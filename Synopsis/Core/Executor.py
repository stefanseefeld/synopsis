"""
Executors are the implementation of the various actions. The actual Action
objects themselves just contain the data needed to perform the actions, and
are minimal on actual code so that they can be easily serialized. The code and
data needed for the execution of an Action is implemented in the matching
Executor class.
"""

import string, re, os, stat, sys

from Action import ActionVisitor
from Synopsis.Core import Util
import AST

try: import gc
except ImportError: gc = None


class Executor:
    """Base class for executor classes, defining the common interface between
    each executor instance."""
    def get_output_names(self):
	"""Returns a list of (name, timestamp) tuples, representing the output
	from this executor. The names must be given to get_output in turn to
	retrieve the AST objects, and the timestamp may be used for build
	control."""
	pass

    def prepare_output(self, name, keep):
	"""Prepares an AST object for returning. For most objects, this does
	nothing. In the case of a cacher, this causes it to process each input
	in turn and store the results to disk. This is as opposed to keeping
	each previous input in memory while the next is parsed!
	Returns the AST if keep is set, else None."""
	if keep: return get_output(name)

    def get_output(self, name):
	"""Returns the AST object for the given name. Name must one returned
	from the 'get_output_names' method."""
	pass

    
class ExecutorCreator (ActionVisitor):
    """Creates Executor instances for Action objects"""
    def __init__(self, project, verbose=0):
	self.__project = project
	self.verbose = verbose or project.verbose()

    def project(self):
	"""Returns the project for this creator"""
	return self.__project

    def create(self, action):
	"""Creates an executor for the given action"""
	self.__executor = None
	action.accept(self)
	return self.__executor

    def visitAction(self, action):
	"""This is an unknown or incomplete Action: ignore."""
	print "Warning: Unknown action '%s'"%action.name()

    def visitSource(self, action):
	self.__executor = SourceExecutor(self, action)
    def visitParser(self, action):
	self.__executor = ParserExecutor(self, action)
    def visitLinker(self, action):
	self.__executor = LinkerExecutor(self, action)
    def visitCacher(self, action):
	self.__executor = CacherExecutor(self, action)
    def visitFormat(self, action):
	self.__executor = FormatExecutor(self, action)

class SourceExecutor (Executor):
    glob_cache = {}

    def __init__(self, executor, action):
	self.__executor = executor
	self.__project = executor.project()
	self.__action = action

    def compile_glob(self, globstr):
	"""Returns a compiled regular expression for the given glob string. A
	glob string is something like "*.?pp" which gets translated into
	"^.*\..pp$". Compiled regular expressions are cached in a class
	variable"""
	if self.glob_cache.has_key(globstr):
	    return self.glob_cache[globstr]
	glob = string.replace(globstr, '.', '\.')
	glob = string.replace(glob, '?', '.')
	glob = string.replace(glob, '*', '.*')
	glob = re.compile('^%s$'%glob)
	self.glob_cache[globstr] = glob
	return glob
    def get_output_names(self):
	"""Expands the paths into a list of filenames, and return those"""
	# Use an 'open' list which contains 3-tuples of 'recurse?', 'path' and
	# 'glob'
	def path_to_tuple(path_obj):
	    if path_obj.type == 'Simple':
		if path_obj.dir.find('/') == -1:
		    return (0, '.', path_obj.dir)
		return (0,)+os.path.split(path_obj.dir)
	    elif path_obj.type == 'Dir':
		return (0, path_obj.dir, path_obj.glob)
	    else:
		return (1, path_obj.dir, path_obj.glob)

	names = []
	for rule in self.__action.rules():
	    if rule.type == 'Simple':
		# Add the specified files if they exist
		for file in rule.files:
		    try:
			filepath = os.path.abspath(file)
			stats = os.stat(filepath)
			if stat.S_ISREG(stats[stat.ST_MODE]):
			    names.append((file, stats[stat.ST_MTIME]))
		    except OSError, e:
			print "Warning:",e
	    elif rule.type == 'Glob':
		glob = self.compile_glob(rule.glob)
		dirs = list(rule.dirs)
		while len(dirs):
		    dir = dirs.pop(0)
		    # Get list of files in this dir
		    for file in os.listdir(dir):
			# Stat file
			filepath = os.path.join(dir, file)
			stats = os.stat(filepath)
			if stat.S_ISDIR(stats[stat.ST_MODE]) and rule.recursive:
			    # Add to list of dirs to check
			    dirs.append(filepath)
			elif stat.S_ISREG(stats[stat.ST_MODE]):
			    # Check if matches glob
			    if glob.match(file):
				# Strip any "./" from the start of the name
				if len(filepath) > 2 and filepath[:2] == "./":
				    filepath = filepath[2:]
				names.append((filepath, stats[stat.ST_MTIME]))
	    elif rule.type == 'Exclude':
		glob = self.compile_glob(rule.glob)
		old_names = names
		names = []
		for name in old_names:
		    # Only re-add ones that don't match
		    if not glob.match(name[0]):
			names.append(name)
	   
	return names
    def get_output(self, name):
	"""Raises an exception, since the SourceAction is only used to
	identify files -- the loading is done by the parsers themselves"""
	raise 'ParseError', "SourceAction doesn't support get_output method."

class ParserExecutor (Executor):
    """Parses the input files given by its input SourceActions"""
    def __init__(self, executor, action):
	self.__executor = executor
	self.__project = executor.project()
	self.__action = action
	self.__name_map = {}
	self.__is_multi = None

    def is_multi(self):
	"""Returns true if this parser parses multiple source files at once.
	This is determined by the parser type and config options."""
	if self.__is_multi is not None: return self.__is_multi
	config = self.__action.config()
	module = config.name
	if module == "C++":
	    if hasattr(config, 'multiple_files'):
		self.__is_multi = config.multiple_files
	    else:
		self.__is_multi = 0
	else:
	    self.__is_multi = 0
	return self.__is_multi

    def get_output_names(self):
	"""Returns the names from all connected SourceActions, and caches
	which source action they came from"""
	names = []
	# for each input source action...
	for source_action in self.__action.inputs():
	    source = self.__executor.create(source_action)
	    source_names = source.get_output_names()
	    names.extend(source_names)
	    for name, timestamp in source_names:
		self.__name_map[name] = source
	# Check multi-file
	if self.is_multi():
	    # Only return first name
	    return names[0:1]
	return names

    def get_output(self, name):
	if self.__executor.verbose:
	    print self.__action.name()+": Parsing "+name
	    sys.stdout.flush()
	config = self.__action.config()
	parser = self.get_parser()
	# Do the parse
	extra_files = None
	if self.is_multi():
	    # Find all source files
	    extra_files = self.__name_map.keys()
	ast = parser.parse(name, extra_files, [], config)
	# Return the parsed AST
	return ast

    def get_parser(self):
	"""Returns the parser module, using the module name stored in the
	Action object. If the module cannot be loaded, this method will raise
	an exception."""
	module = self.__action.config().name
	try:
	    parser = Util._import("Synopsis.Parser." + module)
	except ImportError:
	    # TODO: invent some exception to pass up
	    sys.stderr.write(cmdname + ": Could not import parser `" + name + "'\n")
	    sys.exit(1)
	return parser


		
class LinkerExecutor (Executor):
    def __init__(self, executor, action):
	self.__executor = executor
	self.__project = executor.project()
	self.__action = action
	self.__inputs = {}
	self.__names = {}
    def get_output_names(self):
	"""Links multiple ASTs together, and/or performs other manipulative
	actions on a single AST."""
	# Figure out the output name
	myname = self.__action.name()
	if not myname: myname = 'LinkerOutput'
	myname = myname.replace(' ', '_')
	# Figure out the timestamp
	ts = 0
	for input in self.__action.inputs():
	    exec_obj = self.__executor.create(input)
	    self.__inputs[input] = exec_obj
	    names = exec_obj.get_output_names()
	    self.__names[input] = names
	    for name, timestamp in names:
		if timestamp > ts:
		    ts = timestamp
	return [ (myname, ts) ]

    def get_output(self, name):
	# Get input AST(s), probably from a cacher, source or other linker
	# Prepare the inputs
	for input in self.__action.inputs():
	    exec_obj = self.__inputs[input]
	    names = self.__names[input]
	    for iname, timestamp in names:
		exec_obj.prepare_output(iname, 0)
	# Merge the inputs into one AST
	if self.__executor.verbose:
	    print self.__action.name()+": Linking "+name
	    sys.stdout.flush()
	ast = AST.AST()
	for input in self.__action.inputs():
	    exec_obj = self.__inputs[input]
	    names = self.__names[input]
	    for iname, timestamp in names:
		input_ast = exec_obj.get_output(iname)
		ast.merge(input_ast)
	# Pass merged AST to linker
	module = self.get_linker()
	module.resolve([], ast, self.__action.config())
	# Return linked AST
	return ast

    def get_linker(self):
	"""Returns the linker module, using the module name stored in the
	Action object. If the module cannot be loaded, this method will raise
	an exception."""
	module = self.__action.config().name
	try:
	    linker = Util._import("Synopsis.Linker." + module)
	except ImportError:
	    # TODO: invent some exception to pass up
	    sys.stderr.write(cmdname + ": Could not import linker `" + name + "'\n")
	    sys.exit(1)
	return linker


class CacherExecutor (Executor):
    def __init__(self, executor, action):
	self.__executor = executor
	self.__project = executor.project()
	self.__action = action
	self.__execs = {}
	self.__timestamps = {}
	self.__input_map = {}
	self.__names = []
    def get_output_names(self):
	action = self.__action
	if action.file:
	    # Find file
	    stats = os.stat(action.file)
	    return action.file, stats[stat.ST_MTIME]
	names = self.__names
	# TODO: add logic here to check timestamps, etc
	for input in action.inputs():
	    exec_obj = self.__executor.create(input)
	    self.__execs[input] = exec_obj
	    in_names = exec_obj.get_output_names()
	    names.extend(in_names)
	    # Remember which input for each name
	    for name, timestamp in in_names:
		self.__input_map[name] = exec_obj
		self.__timestamps[name] = timestamp
	return names
    def get_cache_filename(self, name):
	"""Returns the filename of the cache for the input with the given
	name"""
	jname = str(name)
	if jname[0] == '/': jname = jname[1:]
	cache_filename = os.path.join(self.__action.dir, jname)
	if cache_filename[-4:] != ".syn":
	    cache_filename = cache_filename + ".syn"
	return cache_filename
    def _get_timestamp(self, filename):
	"""Returns the timestamp of the given file, or 0 if not found"""
	try:
	    stats = os.stat(filename)
	    return stats[stat.ST_MTIME]
	except OSError:
	    # NB: will catch any type of error caused by the stat call, not
	    # just Not Found
	    return 0
    def prepare_output(self, name, keep):
	"""Prepares the output, which means that it parses it, saves it to
	disk, and forgets about it. If keep is set, return the AST anyway"""
	action = self.__action
	# Check if is a single-file loader (not cache)
	if action.file: return
	cache_filename = self.get_cache_filename(name)
	# Check timestamp on cache
	cache_ts = self._get_timestamp(cache_filename)
	if cache_ts > 0 and cache_ts >= self.__timestamps[name]:
	    # Cache is up to date
	    return
	# Need to regenerate. Find input
	exec_obj = self.__input_map[name]
	ast = exec_obj.get_output(name)
	# Save to cache file
	try:
	    # Create dir for file
	    dir = os.path.dirname(cache_filename)
	    if not os.path.exists(dir):
		print "Warning: creating directory",dir
		os.makedirs(dir)
	    AST.save(cache_filename, ast)
	except:
	    exc, msg = sys.exc_info()[0:2]
	    print "Warning: %s: %s"%(exc, msg)
	if keep: return ast
	elif gc:
	    # Try to free up mem
	    ast = None
	    #gc.set_debug(gc.DEBUG_STATS)
	    gc.collect()

    def get_output(self, name):
	"""Gets the output"""
	action = self.__action
	# Check if is a single-file loader (not cache)
	if action.file:
	    return AST.load(action.file)
	# Double-check preparedness (may generate output)
	ast = self.prepare_output(name, 1)
	if ast: return ast
	# Should now be able to just load from cache file
	return AST.load(self.get_cache_filename(name))
	
class FormatExecutor (Executor):
    """Formats the input AST given by its single input"""
    def __init__(self, executor, action):
	self.__executor = executor
	self.__project = executor.project()
	self.__action = action
	self.__input_exec = None

    def get_output_names(self):
	inputs = self.__action.inputs()
	if len(inputs) != 1:
	    raise 'Error', 'Formatter takes exactly one input AST'
	self.__input_exec = self.__executor.create(inputs[0])
	names = self.__input_exec.get_output_names()
	if len(names) != 1:
	    raise 'Error', 'Formatter takes exactly one input AST'
	return names

    def get_output(self, name):
	# Get input AST, probably from a cache or linker
	ast = self.__input_exec.get_output(name)
	module = self.__action.config().name
	# Pass AST to formatter
	if self.__executor.verbose:
	    print self.__action.name()+": Formatting "+name
	    sys.stdout.flush()
	try:
	    formatter = Util._import("Synopsis.Formatter." + module)
	except ImportError:
	    sys.stderr.write(cmdname + ": Could not import formatter `" + module + "'\n")
	    sys.exit(1)
	formatter.format([], ast, self.__action.config())
	# Finalize AST (incl. maybe write to disk with timestamp info)
	return None
    


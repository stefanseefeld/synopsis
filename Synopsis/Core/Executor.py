"""
Executors are the implementation of the various actions. The actual Action
objects themselves just contain the data needed to perform the actions, and
are minimal on actual code so that they can be easily serialized. The code and
data needed for the execution of an Action is implemented in the matching
Executor class.
"""

import string, re, os, stat

from Action import ActionVisitor
import AST


class Executor:
    """Base class for executor classes"""
    def get_output_names(self): pass # --> list of (name, timestamp)'s

    def get_output(self, name): pass # --> AST

    
class ExecutorCreator (ActionVisitor):
    def __init__(self, project):
	self.__project = project

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
		return (0,)+os.path.split(path_obj.dir)
	    elif path_obj.type == 'Dir':
		return (0, path_obj.dir, path_obj.glob)
	    else:
		return (1, path_obj.dir, path_obj.glob)

	names = []
	paths = map(path_to_tuple, self.__action.paths())
	while len(paths):
	    recurse, path, globpath = paths.pop()
	    glob = self.compile_glob(globpath)
	    # Get list of files matching this path
	    for file in os.listdir(path):
		# Stat file
		filepath = os.path.join(path, file)
		stats = os.stat(filepath)
		if stat.S_ISDIR(stats[stat.ST_MODE]) and recurse:
		    # Add to list of paths to check
		    paths.append( (1, filepath, globpath) )
		elif stat.S_ISREG(stats[stat.ST_MODE]):
		    # Check if matches glob
		    if glob.match(file):
			names.append((filepath, stats[stat.ST_MTIME]))
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
	return names

    def get_output(self, name):
	print "parsing "+name
		
class LinkerExecutor (Executor):
    def __init__(self, executor, action):
	self.__executor = executor
	self.__project = executor.project()
	self.__action = action
    def get_output_names(self):
	"""Links multiple ASTs together, and/or performs other manipulative
	actions on a single AST."""
	# Get input AST(s), probably from a cacher, source or other linker
	# Pass AST(s) to linker
	# Output AST
	pass

    def get_output(self, name):
	pass

class CacherExecutor (Executor):
    def __init__(self, executor, action):
	self.__executor = executor
	self.__project = executor.project()
	self.__action = action
	self.__execs = {}
    def get_output_names(self):
	action = self.__action
	if action.file:
	    return action.file
	names = []
	# TODO: add logic here to check timestamps, etc
	for input in action.inputs():
	    exec_obj = self.__executor.create(input)
	    self.__execs[input] = exec_obj
	    names.extend(exec_obj.get_out_names())
	return map(lambda x: (os.join(action.dir, x[0]), x[1]), names)
    def get_output(self, name):
	action = self.__action
	if action.file:
	    # TODO: unpickle file
	    return AST.load(action.file)
	else:
	    # Call inputs
	    pass

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
	return self.__input_exec.get_output_names()

    def get_output(self, name):
	# Get input AST, probably from a cache or linker
	ast = self.__input_exec.get_output(name)
	# Pass AST to formatter
	print "Formatting ",name
	# Finalize AST (incl. maybe write to disk with timestamp info)
	return None
    


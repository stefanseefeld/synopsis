"""
FileNamer module. This is the base class for the FileNamer interface. The
default implementation stores everything in the same directory.
"""

# System modules
import os, stat, string

# Synopsis modules
from Synopsis.Core import AST

# HTML modules
import core
from core import config

class FileLayout:
    """Base class for naming files.
    You may derive from this class an
    reimplement any methods you like. The default implementation stores
    everything in the same directory (specified by -o)."""
    def __init__(self):
	basename = config.basename
	stylesheet = config.stylesheet
	stylesheet_file = config.stylesheet_file
	if basename is None:
	    print "ERROR: No output directory specified."
	    print "You must specify a base directory with the -o option."
	    sys.exit(1)
	import os.path
	if not os.path.exists(basename):
	    print "Warning: Output directory does not exist. Creating."
	    try:
		os.makedirs(basename, 0755)
	    except EnvironmentError, reason:
		print "ERROR: Creating directory:",reason
		sys.exit(2)
	if stylesheet_file:
	    try:
		# Copy stylesheet in
		stylesheet_new = basename+"/"+stylesheet
		filetime = os.stat(stylesheet_file)[stat.ST_MTIME]
		if not os.path.exists(stylesheet_new) or \
		    filetime > os.stat(stylesheet_new)[stat.ST_MTIME]:
		    fin = open(stylesheet_file,'r')
		    fout = open(stylesheet_new, 'w')
		    fout.write(fin.read())
		    fin.close()
		    fout.close()
	    except EnvironmentError, reason:
		print "ERROR: ", reason
		sys.exit(2)
	if not os.path.isdir(basename):
	    print "ERROR: Output must be a directory."
	    sys.exit(1)
    def nameOfScope(self, scope):
	"""Return the filename of a scoped name (class or module).
	The default implementation is to join the names with '-' and append
	".html" """
	if not len(scope): return self.nameOfSpecial('global')
	return string.join(scope,'-') + ".html"
    def nameOfFile(self, filetuple):
	"""Return the filename of an input file. The file is specified as a
	tuple (generally it is processed this way beforehand so this is okay).
	Default implementation is to join the path with '-', prepend "_file-"
	and append ".html" """
	return "_file-"+string.join(filetuple,'-')+".html"
    def nameOfIndex(self):
	"""Return the name of the main index file. Default is index.html"""
	return "index.html"
    def nameOfSpecial(self, name):
	"""Return the name of a special file (tree, etc). Default is
	_name.html"""
	return "_" + name + ".html"
    def nameOfModuleTree(self):
	"""Return the name of the module tree index. Default is
	_modules.html"""
	return "_modules.html"
    def nameOfModuleIndex(self, scope):
	"""Return the name of the index of the given module. Default is to
	join the name with '-', prepend "_module_" and append ".html" """
	return "_module_" + string.join(scope, '-') + ".html"
    def chdirBase(self):
	"""Change to the base dir. The old current directory is stored for
	restoration by chdirRestore"""
	import os
	self.__old_dir = os.getcwd()
	os.chdir(config.basename)
    def chdirRestore(self):
	"""Change back to the stored directory"""
	import os
	os.chdir(self.__old_dir)
    def link(self, decl):
	"""Create a link to the named declaration. This method may have to
	deal with the directory layout."""
	if isinstance(decl, AST.Scope):
	    # This is a class or module, so it has its own file
	    return self.nameOfScope(decl.name())
	# Assume parent scope is class or module, and this is a <A> name in it
	filename = self.nameOfScope(decl.name()[:-1])
	return filename + "#" + decl.name()[-1]



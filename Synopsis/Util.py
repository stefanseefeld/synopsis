# $Id: Util.py,v 1.19 2002/09/20 10:35:32 chalky Exp $
#
# This file is a part of Synopsis.
# Copyright (C) 2000, 2001 Stefan Seefeld
# Copyright (C) 2000, 2001 Stephen Davies
#
# Synopsis is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA.
#
# $Log: Util.py,v $
# Revision 1.19  2002/09/20 10:35:32  chalky
# Allow writing a comment to top of file
#
# Revision 1.18  2002/08/23 04:37:26  chalky
# Huge refactoring of Linker to make it modular, and use a config system similar
# to the HTML package
#
# Revision 1.17  2002/06/22 06:56:31  chalky
# Fixes to PyWriter for nested classes
#
# Revision 1.16  2002/04/26 01:21:13  chalky
# Bugs and cleanups
#
# Revision 1.15  2001/11/05 06:52:11  chalky
# Major backside ui changes
#
# Revision 1.14  2001/07/19 04:03:05  chalky
# New .syn file format.
#
# Revision 1.13  2001/07/10 14:41:22  chalky
# Make treeformatter config nicer
#
# Revision 1.12  2001/06/28 07:22:18  stefan
# more refactoring/cleanup in the HTML formatter
#
# Revision 1.11  2001/06/26 04:32:15  stefan
# A whole slew of changes mostly to fix the HTML formatter's output generation,
# i.e. to make the output more robust towards changes in the layout of files.
#
# the rpm script now works, i.e. it generates source and binary packages.
#
# Revision 1.10  2001/06/21 01:17:27  chalky
# Fixed some paths for the new dir structure
#
# Revision 1.9  2001/04/05 09:54:00  chalky
# More robust _import()
#
# Revision 1.8  2001/03/29 14:03:36  chalky
# Cache current working dir, and use it for file imports in _import()
#
# Revision 1.7  2001/01/24 18:33:38  stefan
# more cleanup
#
# Revision 1.6  2001/01/24 12:48:10  chalky
# Improved error reporting in _import if __import__ fails for some other reason
# than file not found
#
# Revision 1.5  2001/01/22 17:06:15  stefan
# added copyright notice, and switched on logging
#
# Revision 1.4  2001/01/22 06:04:25  stefan
# some advances on cross referencing
#
# Revision 1.3  2001/01/21 19:31:03  stefan
# new and improved import function that accepts file names or modules
#
# Revision 1.2  2001/01/21 06:22:51  chalky
# Added Util.getopt_spec for --spec=file.spec support
#
# Revision 1.1  2001/01/08 19:48:41  stefan
# changes to allow synopsis to be installed
#
# Revision 1.2  2000/12/19 23:44:16  chalky
# Chalky's Big Commit. Loads of changes and new stuff:
# Rewrote HTML formatter so that it creates a page for each module and class,
# with summaries and details and sections on each page. It also creates indexes
# of modules, and for each module, and a frames index to organise them. Oh and
# an inheritance tree. Bug fixes to some other files. The C++ parser now also
# recognises class and functions to some extent, but support is not complete.
# Also wrote a DUMP formatter that verbosely dumps the AST and Types. Renamed
# the original HTML formatter to HTML_Simple. The ASCII formatter was rewritten
# to follow what looked like major changes to the AST ;) It now outputs
# something that should be the same as the input file, modulo whitespace and
# comments.
#
# Revision 1.1  2000/08/01 03:28:26  stefan
# a major rewrite, hopefully much more robust
#

"""Utility functions for IDL compilers

escapifyString() -- return a string with non-printing characters escaped.
slashName()      -- format a scoped name with '/' separating components.
dotName()        -- format a scoped name with '.' separating components.
ccolonName()     -- format a scoped name with '::' separating components.
pruneScope()     -- remove common prefix from a scoped name.
getopt_spec(args,options,longlist) -- version of getopt that adds transparent --spec= suppport"""

import string, getopt, sys, os, os.path, cStringIO, types

# Store the current working directory here, since during output it is
# sometimes changed, and imports should be relative to the current WD
_workdir = os.getcwd()

def slashName(scopedName, our_scope=[]):
    """slashName(list, [list]) -> string

Return a scoped name given as a list of strings as a single string
with the components separated by '/' characters. If a second list is
given, remove a common prefix using pruneScope()."""
    
    pscope = pruneScope(scopedName, our_scope)
    return string.join(pscope, "/")

def dotName(scopedName, our_scope=[]):
    """dotName(list, [list]) -> string

Return a scoped name given as a list of strings as a single string
with the components separated by '.' characters. If a second list is
given, remove a common prefix using pruneScope()."""
    
    pscope = pruneScope(scopedName, our_scope)
    return string.join(pscope, ".")

def ccolonName(scopedName, our_scope=[]):
    """ccolonName(list, [list]) -> string

Return a scoped name given as a list of strings as a single string
with the components separated by '::' strings. If a second list is
given, remove a common prefix using pruneScope()."""
    
    pscope = pruneScope(scopedName, our_scope)
    try: return string.join(pscope, "::")
    except TypeError, e:
	import pprint
	pprint.pprint(scopedName)
	pprint.pprint(our_scope)
	if type(pscope) in (type([]), type(())):
	    raise TypeError, str(e) + " ..not: list of " + str(type(pscope[0]))
	raise TypeError, str(e) + " ..not: " + str(type(pscope))

def pruneScope(target_scope, our_scope):
    """pruneScope(list A, list B) -> list

Given two lists of strings (scoped names), return a copy of list A
with any prefix it shares with B removed.

  e.g. pruneScope(['A', 'B', 'C', 'D'], ['A', 'B', 'D']) -> ['C', 'D']"""

    tscope = list(target_scope)
    i = 0
    while len(tscope) > 1 and \
          i < len(our_scope) and \
          tscope[0] == our_scope[i]:
        del tscope[0]
        i = i + 1
    return tscope

def escapifyString(str):
    """escapifyString(string) -> string

Return the given string with any non-printing characters escaped."""
    
    l = list(str)
    vis = string.letters + string.digits + " _!$%^&*()-=+[]{};'#:@~,./<>?|`"
    for i in range(len(l)):
        if l[i] not in vis:
            l[i] = "\\%03o" % ord(l[i])
    return string.join(l, "")

def _import(name):
    """import either a module, or a file."""
    arg_name = name #backup for error reporting
    # if name contains slashes, interpret it as a file
    as_file = string.find(name, "/") != -1
    as_file = as_file or name[-3:] == '.py'
    if not as_file:
        components = string.split(name, ".")
        # if one of the components is empty, name is interpreted as a file ('.foo' for example)
        for comp in components:
            if not comp:
                as_file = 1
                break
    mod = None
    error_messages = []
    # try as module
    if not as_file:
	import_name = list(components)
	while len(import_name):
	    try:
		mod = __import__(string.join(import_name, '.'))
		for comp in components[1:]:
		    try:
			mod = getattr(mod, comp)
		    except AttributeError, msg:
			print "Error: Unable to find %s in:\n%s"%(
			    comp,repr(mod))
			print "Error: Importing '%s'\n"%arg_name
			print "Collected import messages (may not all be errors):"
			for message in error_messages: print message
			sys.exit(1)
		return mod
	    except ImportError, msg:
		# Remove last component and try again
		del import_name[-1]
		msg = "  %s:\n    %s"%(string.join(import_name, '.'), msg)
		error_messages.append(msg)
	    except SystemExit, msg: raise
	    except:
		print "Unknown error occurred importing",name
		import traceback
		traceback.print_exc()
		sys.exit(1)

    # try as file
    try:
	if not name[0] == '/': name = _workdir+os.sep+name
	if not os.access(name, os.R_OK): raise ImportError, "Cannot access file %s"%name
        dir = os.path.abspath(os.path.dirname(name))
        name = os.path.basename(name)
        modname = name[:]
        if modname[-3:] == ".py": modname = modname[0:-3]
        if dir not in sys.path: sys.path.insert(0, dir)
        mod = __import__(modname)
    except ImportError, msg:
        sys.path = sys.path[1:]
        sys.stderr.write("Error: Could not find module %s: %s\n"%(name,msg))
	sys.stderr.write("Error: Importing '%s'\n"%arg_name)
        sys.stderr.flush()
        sys.exit(-1)
    return mod

def import_object(spec, defaultAttr = None, basePackage = ''):
    """Imports an object according to 'spec'. spec must be either a
    string or a tuple of two strings. A tuple of two strings means load the
    module from the first string, and look for an attribute using the second
    string. One string is interpreted according to the optional arguments. The
    default is just to load the named module. 'defaultAttr' means to look for
    the named attribute in the module and return that. 'basePackage' means to
    prepend the named string to the spec before importing. Note that you may
    pass a list instead of a tuple, and it will have the same effect.
    
    This is used by the HTML formatter for example, to specify page classes.
    Each class is in a separate module, and each module has a htmlPageAttr
    attribute that references the class of the Page for that module. This
    avoids the need to specify a list of default pages, easing
    maintainability."""
    if type(spec) == types.ListType: spec = tuple(spec)
    if type(spec) == types.TupleType:
	# Tuple means (module-name, attribute-name)
	if len(spec) != 2:
	    raise TypeError, "Import tuple must have two strings"
	name, attr = spec
	if type(name) != types.StringType or type(attr) != types.StringType:
	    raise TypeError, "Import tuple must have two strings"
	module = _import(name)
	if not hasattr(module, attr):
	    raise ImportError, "Module %s has no %s attribute."%spec
	return getattr(module, attr)
    elif type(spec) == types.StringType:
	# String means HTML module with htmlPageClass attribute
	module = _import(basePackage+spec)
	if defaultAttr is not None:
	    if not hasattr(module, defaultAttr):
		raise ImportError, "Module %s has no %s attribute."%(spec, defaultAttr)
	    return getattr(module, defaultAttr)
	return module
    else:
	raise TypeError, "Import spec must be a string or tuple of two strings."


def splitAndStrip(line):
    """Splits a line at the first space, then strips the second argument"""
    pair = string.split(line, ' ', 1)
    return pair[0], string.strip(pair[1])

def open(filename):
    """open a file, generating all intermediate directories if needed"""
    import __builtin__
    dir, file = os.path.split(filename)
    if dir and not os.path.isdir(dir): os.makedirs(dir)
    return __builtin__.open(filename, 'w+')

def getopt_spec(args, options, long_options=[]):
    """Transparently add --spec=file support to getopt"""
    long_options.append('spec=')
    opts, remainder = getopt.getopt(args, options, long_options)
    ret = []
    for pair in opts:
	if pair[0] == '--spec':
	    f = open(pair[1], 'rt')
	    spec_opts = map(splitAndStrip, f.readlines())
	    f.close()
	    ret.extend(spec_opts)
	else:
	    ret.append(pair)
    return ret, remainder

class PyWriter:
    """A class that allows writing data in such a way that it can be read in
    by just 'exec'ing the file. You should extend it and override write_item()"""
    def __init__(self, ostream):
	self.os = ostream
	self.buffer = cStringIO.StringIO()
	self.imports = {}
	self.__indent = '\n'
	self.__prepend = ''
	self.__class_funcs = {}
	self.__long_lists = {}
	self.__done_struct = 0
    def indent(self):
	self.__indent = self.__indent+'  '
    def outdent(self):
	self.__indent = self.__indent[:-2]
    def ensure_import(self, module, names):
	key = module+names
	if self.imports.has_key(key): return
	self.imports[key] = None
	self.os.write('from %s import %s\n'%(module, names))
    def ensure_struct(self):
	if self.__done_struct == 1: return
	self.os.write('class struct:\n def __init__(self,**args):\n  for k,v in args.items(): setattr(self, k, v)\n\n')
	self.__done_struct = 1
    def write_top(self, str):
	"""Writes a string to the top of the file"""
	self.os.write(str)
    def write(self, str):
	# Get cached '\n' if any
	prefix = self.__prepend
	self.__prepend = ''
	# Cache '\n' if any
	if len(str) and str[-1] == '\n':
	    self.__prepend = '\n'
	    str = str[:-1]
	# Indent any remaining \n's, including cached one
	str = string.replace(prefix + str, '\n', self.__indent)
	self.buffer.write(str)
    def write_item(self, item):
	"""Writes arbitrary items by looking up write_Foo functions where Foo
	is the class name of the item"""
	# Use repr() for non-instance types
	if type(item) is types.ListType:
	    return self.write_list(item)
	if type(item) is not types.InstanceType:
	    return self.write(repr(item))
    
	# Check for class in cache
	class_obj = item.__class__
	if self.__class_funcs.has_key(class_obj):
	    return self.__class_funcs[class_obj](item)
	# Check for write_Foo method
	func_name = 'write_'+class_obj.__name__
	if not hasattr(self, func_name):
	    return self.write(repr(item))
	# Cache method and call it
	func = getattr(self, func_name)
	self.__class_funcs[class_obj] = func
	func(item)
    def flush(self):
	"Writes the buffer to the stream and closes the buffer"
	# Needed to flush the cached '\n'
	if self.__prepend: self.write('')
	self.os.write(self.buffer.getvalue())
	if 1: # for debugging
	    sys.stdout.write(self.buffer.getvalue())
	self.buffer.close()
    def long(self, list):
	"Remembers list as wanting 'long' representation (an item per line)"
	self.__long_lists[id(list)] = None
	return list
    def write_list(self, list):
	"""Writes a list on one line. If long(list) was previously called, the
	list from its cache and calls write_long_list"""
	if self.__long_lists.has_key(id(list)):
	    del self.__long_lists[id(list)]
	    return self.write_long_list(list)
	self.write('[')
	comma = 0
	for item in list:
	    if comma: self.write(', ')
	    else: comma = 1
	    self.write_item(item)
	self.write(']')
    def write_long_list(self, list):
	"Writes a list with each item on a new line"
	if not list:
	    self.write('[]')
	    return
	self.write('[\n')
	self.indent()
	comma = 0
	for item in list:
	    if comma:
		self.__prepend = ''
		self.write(',\n')
	    else: comma = 1
	    self.write_item(item)
	self.outdent()
	self.write('\n]')
    def write_attr(self, name, value):
	self.write(name + ' = ')
	self.write_item(value)
	self.write('\n')
    def flatten_struct(self, struct):
	"""Flattens a struct into a (possibly nested) list. A struct is an
	object with only the following members: numbers, strings, sub-structs,
	lists and tuples."""
	t = type(struct)
	if t is types.TupleType:
	    return tuple(map(self.flatten_struct, struct))
	if t is types.ListType:
	    return map(self.flatten_struct, struct)
	if t in (types.ClassType, types.InstanceType):
	    self.ensure_struct()
	    flatten_item = lambda kv, self=self: (kv[0], self.flatten_struct(kv[1]))
	    items = lambda kv: kv[0] not in ('__init__', '__doc__', '__module__')
	    return PyWriterStruct(map(flatten_item, filter(items, struct.__dict__.items()) ))
	return struct
    def write_PyWriterStruct(self, struct):
	if not len(struct.dict): return self.write('struct()')
	self.write('struct(')
	self.indent()
	# Write one attribute per line, being sure to allow nested structs
	prefix = '\n'
	for key, val in struct.dict:
	    self.write(prefix+str(key)+'=')
	    self.write_item(val)
	    prefix = ',\n'
	self.outdent()
	self.write('\n)')
	
class PyWriterStruct:
    """A utility class that PyWriter uses to dump class objects. Dict is the
    dictionary of the class being dumped."""
    def __init__(self, dict): self.dict = dict

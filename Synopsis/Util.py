# $Id: Util.py,v 1.11 2001/06/26 04:32:15 stefan Exp $
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

import string, getopt, sys, os, os.path

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
			sys.exit(1)
		return mod
	    except ImportError, msg:
		# Remove last component and try again
		del import_name[-1]
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
        sys.stderr.flush()
        sys.exit(-1)
    return mod

def splitAndStrip(line):
    """Splits a line at the first space, then strips the second argument"""
    pair = string.split(line, ' ', 1)
    return pair[0], string.strip(pair[1])

def open(filename):
    """open a file, generating all intermediate directories if needed"""
    import __builtin__
    dir, file = os.path.split(filename)
    if dir and not os.path.isdir(dir): os.makedirs(dir)
    return __builtin__.open(filename, 'w')

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

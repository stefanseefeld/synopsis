# $Id: XRefCompiler.py,v 1.4 2002/12/09 04:00:59 chalky Exp $
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
# $Log: XRefCompiler.py,v $
# Revision 1.4  2002/12/09 04:00:59  chalky
# Added multiple file support to parsers, changed AST datastructure to handle
# new information, added a demo to demo/C++. AST Declarations now have a
# reference to a SourceFile (which includes a filename) instead of a filename.
#
# Revision 1.3  2002/11/03 06:08:56  chalky
# Tolerate file not found errors
#
# Revision 1.2  2002/10/29 12:43:25  chalky
# Added no_locals support which is on by default, and strips local variables
# from cross reference tables
#
# Revision 1.1  2002/10/29 07:34:29  chalky
# New linker module, compiles xref files into data structure
#
#

import string, cPickle, urllib

from Synopsis.Core import AST, Type, Util

from Linker import config, Operation

def do_compile(input_files, output_file, no_locals):
    # Init data structures
    data = {}
    index = {}
    file_data = (data, index)

    # Read input files
    for file in input_files:
	if config.verbose: print "XRefCompiler: Reading",file
	try:
	    f = open(file, 'rt')
	except IOError, e:
	    print "Error opening %s: %s"%(file, e)
	    continue

	lines = f.readlines()
	f.close()

	for line in lines:
	    target, file, line, scope, context = string.split(line)
	    target = tuple(map(intern, string.split(urllib.unquote(target), '\t')))
	    scope = map(intern, string.split(urllib.unquote(scope), '\t'))
	    line = int(line)
	    file = intern(file)

	    if no_locals:
		bad = 0
		for i in range(len(target)):
		    if len(target[i]) > 0 and target[i][0] == '`':
			# Don't store local function variables
			bad = 1
			break
		if bad: continue

	    for i in range(len(scope)):
		if len(scope[i]) > 0 and scope[i][0] == '`':
		    # Function scope, truncate here
		    del scope[i+1:]
		    scope[i] = scope[i][1:]
		    break
	    scope = tuple(scope)
	    
	    target_data = data.setdefault(target, [[],[],[]])
	    if context == 'DEF':
		target_data[0].append( (file, line, scope) )
	    elif context == 'CALL':
		target_data[1].append( (file, line, scope) )
	    elif context == 'REF':
		target_data[2].append( (file, line, scope) )
	    else:
		print "Warning: Unknown context:",context

    # Sort the data
    for target, target_data in data.items():
	target_data[1].sort()
	target_data[2].sort()

	name = target[-1]
	index.setdefault(name,[]).append(target)
	# If it's a function name, also add without the parameters
	paren = name.find('(')
	if paren != -1:
	    index.setdefault(name[:paren],[]).append(target)
	    
    if config.verbose: print "XRefCompiler: Writing",output_file
    f = open(output_file, 'wb')
    cPickle.dump(file_data, f)
    f.close()
 
class XRefCompiler(Operation):
    """This class compiles a set of text-based xref files from the C++ parser
    into a cPickled data structure with a name index.
    
    The format of the data structure is:
    <pre>
     (data, index) = cPickle.load()
     data = dict<scoped targetname, target_data>
     index = dict<name, list<scoped targetname>>
     target_data = (definitions = list<target_info>,
		    func calls = list<target_info>,
		    references = list<target_info>)
     target_info = (filename, int(line number), scoped context name)
    </pre>
    The scoped targetnames in the index are guaranteed to exist in the data
    dictionary.
    """
    def __init__(self):
	self.__xref_path = './%s-xref'
	self.__xref_file = 'compiled.xref'
	self.__no_locals = 1
	if hasattr(config.obj, 'XRefCompiler'):
	    obj = config.obj.XRefCompiler
	    if hasattr(obj, 'xref_path'):
		self.__xref_path = obj.xref_path
	    if hasattr(obj, 'xref_file'):
		self.__xref_file = obj.xref_file
	    if hasattr(obj, 'no_locals'):
		self.__no_locals = obj.no_locals
    def execute(self, ast):
	filenames = map(lambda x: x[0], 
	    filter(lambda x: x[1].is_main(), 
		ast.files().items()))
	filenames = map(lambda x:self.__xref_path%x, filenames)
	do_compile(filenames, self.__xref_file, self.__no_locals)

linkerOperation = XRefCompiler

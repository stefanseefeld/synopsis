# $Id: DUMP.py,v 1.14 2002/12/12 17:25:32 chalky Exp $
#
# This file is a part of Synopsis.
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
# $Log: DUMP.py,v $
# Revision 1.14  2002/12/12 17:25:32  chalky
# Implemented Include support for C++ parser. A few other minor fixes.
#
# Revision 1.13  2002/12/09 04:00:58  chalky
# Added multiple file support to parsers, changed AST datastructure to handle
# new information, added a demo to demo/C++. AST Declarations now have a
# reference to a SourceFile (which includes a filename) instead of a filename.
#
# Revision 1.12  2002/04/26 01:21:13  chalky
# Bugs and cleanups
#
# Revision 1.11  2001/07/19 04:00:39  chalky
# New .syn file format. Added -d, -t and -f flags
#
# Revision 1.10  2001/05/25 13:45:49  stefan
# fix problem with getopt error reporting
#
# Revision 1.9  2001/02/13 06:55:23  chalky
# Made synopsis -l work again
#
# Revision 1.8  2001/01/31 06:51:24  stefan
# add support for '-v' to all modules; modified toc lookup to use additional url as prefix
#
# Revision 1.7  2001/01/24 01:38:36  chalky
# Added docstrings to all modules
#
# Revision 1.6  2001/01/22 19:54:41  stefan
# better support for help message
#
# Revision 1.5  2001/01/22 17:06:15  stefan
# added copyright notice, and switched on logging
#
"""
Verbose attribute-oriented dump of AST. Pipe into less -r
"""

# THIS-IS-A-FORMATTER
import sys, getopt, os, os.path, string, types
from Synopsis.Core import Type, AST

verbose = 0

class Dumper:
    def __init__(self):
	self.handlers = {
	    types.NoneType : self.visitNone,
	    types.TypeType : self.visitType,
	    types.IntType : self.visitInt,
	    types.LongType : self.visitLong,
	    types.FloatType : self.visitFloat,
	    types.StringType : self.visitString,
	    types.TupleType : self.visitTuple,
	    types.ListType : self.visitList,
	    types.DictType : self.visitDict,
	    types.InstanceType : self.visitInstance,
	}
	self.clear()
    def clear(self):
	self.indent_level = 0
	self.indent_string = ""
	self.newline = 1
	self.visited = {}
    def writeln(self, text):
	self.write(text)
	self.newline = 1
    def lnwrite(self, text):
	self.newline = 1
	self.write(text)
    def write(self, text):
	if self.newline:
	    sys.stdout.write("\n")
	    sys.stdout.write(self.indent_string)
	    self.newline = 0
	sys.stdout.write(str(text))
    def indent(self, str="  "):
	self.indent_level = self.indent_level + 1
	self.indent_string = self.indent_string+str
    def outdent(self):
	self.indent_level = self.indent_level - 1
	self.indent_string = self.indent_string[:-2]

    def visit(self, obj):
	i,t = id(obj), type(obj)
	if self.visited.has_key(i):
	    if t == types.InstanceType: t = obj.__class__.__name__+" instance"
	    if hasattr(obj, 'name'):
		self.write("<already visited %s ( %d ) '%s'>"%(t,i,string.join(obj.name(),"::")))
	    elif hasattr(obj, 'filename'):
		self.write("<already visited %s ( %d ) '%s'>"%(t,i,obj.filename()))
	    else:
		self.write("<already visited %s ( %d )>"%(t,i))
	    return
	if self.handlers.has_key(t):
	    self.handlers[t](obj)
	else:
	    self.write("Unknown type %s for object: '%s'"%(t,obj))

    def visitNone(self, obj):
        self.write(obj)
    def visitType(self, obj):
        self.write(obj)
    def visitInt(self, obj):
        self.write(obj)
    def visitLong(self, obj):
        self.write(obj)
    def visitFloat(self, obj):
        self.write(obj)
    def visitString(self, obj):
	import string
        self.write('"%s"'%string.replace(obj,"\n","\\n"))
    def visitTuple(self, obj):
	if len(obj) == 0:
	    self.write("()")
	    return
	# Check if all strings
	strings = 1
	for elem in obj:
	    if type(elem) != types.StringType:
		strings = 0
		break
	if strings:
	    self.write("( "+string.join(obj,"::")+" )")
	    return
	# Else write one per line
        self.writeln("( ")
	self.indent()
	if len(obj): self.visit(obj[0])
	for elem in obj[1:]:
	    self.write(", ")
	    self.visit(elem)
	self.outdent()
	self.lnwrite(")")
    def visitList(self, obj):
	if len(obj) == 0:
	    self.write("[]")
	    return
	# Check if all strings
	strings = 1
	for elem in obj:
	    if type(elem) != types.StringType:
		strings = 0
		break
	if strings:
	    self.write("[ "+string.join(obj,"::")+" ]")
	    return
        self.writeln("[ ")
	self.indent()
	if len(obj): self.visit(obj[0])
	for elem in obj[1:]:
	    self.writeln(", ")
	    self.visit(elem)
	self.outdent()
	self.lnwrite("]")
    def _dictKey(self, key):
	self.visit(key)
	self.write(":")
    def visitDict(self, dict, namefunc=None):
	items = dict.items()
	if len(items) == 0:
	    self.write("{}")
	    return
	items.sort()
        self.writeln("{ ")
	if namefunc is None:
	    self.indent()
	    namefunc = self._dictKey
	else:
	    self.indent(". ")
	if len(items):
	    namefunc(items[0][0])
	    self.visit(items[0][1])
	for item in items[1:]:
	    self.writeln(", ")
	    namefunc(item[0])
	    self.visit(item[1])
	self.outdent()
	self.lnwrite("}")
    def _instAttr(self, name):
	if type(name) != types.StringType:
	    return self.visit(name)
	if name[0] != '_':
	    return self.write(name+" = ")
	index = string.find(name, '__')
	if index < 0:
	    return self.write(name+" = ")
	self.write("%s::%s = "%(
	    name[1:index],
	    name[index+2:]
	))
	    
    def visitInstance(self, obj):
	global show_forwards
	if isinstance(obj, AST.Forward) and not show_forwards: return
	if isinstance(obj, AST.SourceFile) and not show_sourcefiles:
	    self.write("SourceFile: '%s'"%obj.filename())
	    return
	if isinstance(obj, AST.Include):
	    self.write("Include: (macro:%d, next:%d) '%s'"%(obj.is_macro(),
		obj.is_next(), obj.target().filename()))
	    return
	self.visited[id(obj)] = None
        self.write("[1m%s.%s[m = "%(
	    obj.__class__.__module__,
	    obj.__class__.__name__
	))
	self.visitDict(obj.__dict__, self._instAttr)

def usage():
    """Print usage to stdout"""
    print \
"""
  -o <file>                            Output file
  -d                                   Show declarations
  -t                                   Show types
  -f                                   Show forwards also
 (If none of -d, -t or -f specified, will default to -d, -t)
"""

def __parseArgs(args):
    global output, verbose, show_decls, show_types, show_forwards, show_files
    # Set defaults
    output = sys.stdout
    show_decls = 0
    show_types = 0
    show_forwards = 0
    show_files = 1

    try:
        opts,remainder = getopt.getopt(args, "o:vdtf")
    except getopt.error, e:
        sys.stderr.write("Error in arguments: " + str(e) + "\n")
        sys.exit(1)

    for opt in opts:
        o,a = opt

        if o == "-o": output = open(a, "w")
        elif o == "-v": verbose = 1
	elif o == "-d": show_decls = 1
	elif o == "-t": show_types = 1
	elif o == "-f": show_forwards = 1

    # Consolidate - if no show_ selected, show decls and types
    if not (show_decls or show_types or show_forwards):
	show_decls = 1
	show_types = 1

def format(args, ast, config_obj):
    global output, show_sourcefiles
    __parseArgs(args)
    #formatter = ASCIIFormatter(output)
    #for type in dictionary:
    #    type.output(formatter)
    dumper = Dumper()
    show_sourcefiles = 0
    if show_decls:
	print "*** Declarations:"
	dumper.visit(ast.declarations())
    if show_types:
	if show_decls: print "\n\n\n"
	print "*** Types:"
	dumper.visit(ast.types())
    if show_files:
	show_sourcefiles = 1
	print "\n\n\n"
	print "*** Files:"
	dumper.visit(ast.files())

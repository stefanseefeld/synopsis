# $Id: Dot.py,v 1.1 2001/01/23 19:50:42 stefan Exp $
#
# This file is a part of Synopsis.
# Copyright (C) 2000, 2001 Stefan Seefeld
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
# $Log: Dot.py,v $
# Revision 1.1  2001/01/23 19:50:42  stefan
# Dot: an inheritance/collaboration graph generator
#
#

import sys, tempfile, getopt, os, os.path, string, types, errno
from Synopsis.Core import AST, Type, Util

class InheritanceFormatter(AST.Visitor):
    """A Formatter that generates an inheritance graph"""
    def __init__(self, output):
        self.__output = output
    def write(self, text): self.__output.write(text)
    def visitClass(self, node):
        name = node.name()
        for inheritance in node.parents():
            parent = inheritance.parent()
	    if hasattr(parent, 'name'):
                self.write(parent.name()[-1] + " -> " + name[-1])
            elif "private" in inheritance.attributes(): self.write("[style=dotted];\n")
            else: self.write(";\n");
        for d in node.declarations(): d.accept(self)

class CollaborationFormatter(AST.Visitor, Type.Visitor):
    """A Formatter that generates a collaboration graph"""
    def __init__(self, output):
        self.__output = output
    def write(self, text): self.__output.write(text)
    def visitClass(self, node):
        name = node.name()
        for inheritance in node.parents():
            parent = inheritance.parent()
	    if hasattr(parent, 'name'):
                self.write(parent.name()[-1] + " -> " + name[-1] + ";\n")
        for d in node.declarations(): d.accept(self)
    def visitVariable(self, node):
        node.vtype().accept(self)
        print node.name(), self.__type_label

    def visitBaseType(self, type):
        self.__type_label = type.name()
    def visitUnknown(self, type):
        self.__type_label = type.name()
    def visitDeclared(self, type):
        self.__type_label = type.name()
    def visitModifier(self, type):
        alias = type.alias()
	pre = string.join(map(lambda x:x+"&nbsp;", type.premod()), '')
	post = string.join(type.postmod(), '')
        self.__type_label = "%s%s%s"%(pre,alias,post)
    def visitTemplate(self, type):
        self.__type_label = "<template>"
    def visitParametrized(self, type):
        self.__type_label = "<parametrized>"
    def visitFunctionType(self, type):
        self.__type_label = "<function>"


def usage():
    """Print usage to stdout"""
    print \
"""
  -o <filename>                        Output directory, created if it doesn't exist.
  -t <title>                           Associate <title> with the generated graph
  -i                                   Generate an inheritance graph
  -c                                   Generate a collaboration graph"""

def __parseArgs(args):
    global output, title, type
    output = ''
    title = ''
    type = ''
    try:
        opts,remainder = Util.getopt_spec(args, "o:t:ic")
    except Util.getopt.error, e:
        sys.stderr.write("Error in arguments: " + e + "\n")
        sys.exit(1)

    for opt in opts:
        o,a = opt
        if o == "-o": output = a
        elif o == "-t": title = a
        elif o == "-i": type = "inheritance"
        elif o == "-c":
            type = "collaboration"
            sys.stderr.write("sorry, collaboration diagrams not yet implemented\n");
            sys.exit(-1)
def format(types, declarations, args):
    global output, title, type
    __parseArgs(args)
    tmpfile = output + ".dot"
    dotfile = open(tmpfile, 'w+')
    dotfile.write("digraph \"%s\" {\n"%(title))
    dotfile.write("node[shape=box, fontsize=10, height=0.2, width=0.4]\n")
    if type == "inheritance":
        generator = InheritanceFormatter(dotfile)
    else:
        generator = CollaborationFormatter(dotfile)
    for d in declarations:
        d.accept(generator)
    dotfile.write("}\n")
    dotfile.close()
    os.system("dot -Tps -o %s %s"%(output, tmpfile))
#    os.remove(tmpfile)
    

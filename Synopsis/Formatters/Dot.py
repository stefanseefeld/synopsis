# $Id: Dot.py,v 1.7 2001/02/02 02:01:01 stefan Exp $
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
# Revision 1.7  2001/02/02 02:01:01  stefan
# synopsis now supports inlined inheritance tree generation
#
# Revision 1.6  2001/02/01 04:04:23  stefan
# added support for html page generation
#
# Revision 1.5  2001/01/31 21:53:11  stefan
# some more work on the dot formatter
#
# Revision 1.4  2001/01/31 06:51:24  stefan
# add support for '-v' to all modules; modified toc lookup to use additional url as prefix
#
# Revision 1.3  2001/01/24 01:38:36  chalky
# Added docstrings to all modules
#
# Revision 1.2  2001/01/23 21:31:36  stefan
# bug fixes
#
# Revision 1.1  2001/01/23 19:50:42  stefan
# Dot: an inheritance/collaboration graph generator
#
#

"""
Uses 'dot' from graphviz to generate various graphs.
"""

import sys, tempfile, getopt, os, os.path, string, types, errno
from Synopsis.Core import AST, Type, Util
from Synopsis.Formatter import TOC

verbose = 0
toc = None
nodes = {}

class InheritanceFormatter(AST.Visitor, Type.Visitor):
    """A Formatter that generates an inheritance graph"""
    def __init__(self, os):
        self.__os = os
        self.__scope = []
        self.__type_ref = None
        self.__type_label = ''
    def scope(self): return self.__scope
    def write(self, text): self.__os.write(text)
    def type_ref(self): return self.__type_ref
    def type_label(self): return self.__type_label
    def parameter(self): return self.__parameter

    def formatType(self, typeObj):
	"Returns a reference string for the given type object"
	if typeObj is None: return "(unknown)"
        typeObj.accept(self)
        return self.type_label()

    def clearType(self):
        self.__type_ref = None
        self.__type_label = ''
        
    def writeNode(self, ref, label):
        """helper method to generate output for a given node"""
        if nodes.has_key(label): return
        nodes[label] = len(nodes)
        number = nodes[label]
        if ref: color = "black"
        else: color = "gray75"
        self.write("Node" + str(number) + " [shape=\"box\", label=\"" + label + "\"")
        self.write(", fontSize = 10, height = 0.2, width = 0.4")
        self.write(", color=\"" + color + "\"")
        if ref: self.write(", URL=\"" + ref.link + "\"")
        self.write("];\n")

    def writeEdge(self, parent, child, label, attr):
        self.write("Node" + str(nodes[parent]) + " -> ")
        self.write("Node" + str(nodes[child]))
        self.write("[ color=\"black\", fontsize=10, style=\"")
        #if "private" in attr: self.write("dotted")
        #else:
        self.write("solid")
        self.write("\"];\n")        

    #################### Type Visitor ###########################################
    def visitUnknown(self, type):
        self.__type_ref = toc[type.link()]
        self.__type_label = Util.ccolonName(type.name(), self.scope())
        
    def visitDeclared(self, type):
        self.__type_ref = toc[type.name()]
        self.__type_label = Util.ccolonName(type.name(), self.scope())

    def visitParametrized(self, type):
	if type.template():
            self.__type_ref = toc[type.template().name()]
	    type_label = Util.ccolonName(type.template().name(), self.scope())
	else:
            self.__type_ref = None
	    type_label = "(unknown)"
        parameters_label = []
        for p in type.parameters():
            parameters_label.append(self.formatType(p))
        self.__type_label = type_label + "&lt;" + string.join(parameters_label, ", ") + "&gt;"

    def visitTemplate(self, type):
        self.__type_ref = None
	self.__type_label = "template&lt;%s&gt;"%(
            string.join(map(lambda x:"typename "+x, map(self.formatType, type.parameters())), ","))

    #################### AST Visitor ############################################
        
    def visitInheritance(self, node):
        self.formatType(node.parent())
        self.writeNode(self.type_ref(), self.type_label())
        
    def visitClass(self, node):
        label = Util.ccolonName(node.name(), self.scope())
        self.writeNode(toc[node.name()], label)
        for inheritance in node.parents():
            inheritance.accept(self)
            self.writeEdge(self.type_label(), label, None, inheritance.attributes())
        for d in node.declarations(): d.accept(self)

class SingleInheritanceFormatter(InheritanceFormatter):
    """A Formatter that generates an inheritance graph for a specific class.
    This Visitor visits the AST upwards, i.e. following the inheritance links, instead of
    the declarations contained in a given scope."""
    #base = InheritanceFormatter
    def __init__(self, os, levels, types):
        InheritanceFormatter.__init__(self, os)
        self.__levels = levels
        self.__types = types
        self.__current = 0

    #################### Type Visitor ###########################################

    def visitDeclared(self, type):
        type.declaration().accept(self)
        # to restore the ref/label...
        InheritanceFormatter.visitDeclared(self, type)
    #################### AST Visitor ############################################
        
    def visitInheritance(self, node):
        self.__current = self.__current + 1
        node.parent().accept(self)
        self.__current = self.__current - 1
        if self.type_label():
            self.writeNode(self.type_ref(), self.type_label())
        
    def visitClass(self, node):
        label = Util.ccolonName(node.name(), self.scope())
        self.writeNode(toc[node.name()], label)
        if self.__current == self.__levels or self.__levels != -1: return
        for inheritance in node.parents():
            inheritance.accept(self)
            if nodes.has_key(self.type_label()):
                self.writeEdge(self.type_label(), label, None, inheritance.attributes())
        # if this is the main class and if there is a type dictionary,
        # look for classes that are derived from this class

        # this part doesn't work yet...
        return
        if not self.__current and self.__types:
            for t in self.__types.values():
                if isinstance(t, Type.Declared):
                    if isinstance(t.declaration(), AST.Class):
                        for inheritance in t.declaration().parents():
                            type = inheritance.parent()
                            type.accept(self)
                            if self.type_ref() == node.name():
                                child = t.declaration()
                                child_label = Util.ccolonName(child.name(), self.scope())
                                self.writeNode(toc[child.name()], child_label)
                                self.writeEdge(label, child_label, None, inheritance.attributes())

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
  -s                                   Generate an inheritance graph for a single class
  -c                                   Generate a collaboration graph
  -f <format>                          Generate output in format 'dot', 'ps' (default), 'png', 'gif', 'map', 'html'
  -r <filename>                        Read in toc for an external data base that is to be referenced"""

formats = {
    'dot' : 'dot',
    'ps' : 'ps',
    'png' : 'png',
    'gif' : 'gif',
    'map' : 'imap',
    'html' : 'html',
}


def __parseArgs(args):
    global output, title, type, oformat, verbose, toc_in
    output = ''
    title = 'NoTitle'
    type = ''
    oformat = 'ps'
    toc_in = []
    try:
        opts,remainder = Util.getopt_spec(args, "o:t:f:r:icsv")
    except Util.getopt.error, e:
        sys.stderr.write("Error in arguments: " + e + "\n")
        sys.exit(1)

    for opt in opts:
        o,a = opt
        if o == "-o": output = a
        elif o == "-t": title = a
        elif o == "-i": type = "inheritance"
        elif o == "-s": type = "single"
        elif o == "-c":
            type = "collaboration"
            sys.stderr.write("sorry, collaboration diagrams not yet implemented\n");
            sys.exit(-1)
        elif o == "-f":
            if formats.has_key(a): oformat = formats[a]
	    else:
		print "Error: Unknown format. Available formats are:",string.join(formats.keys(), ', ')
		sys.exit(1)
        elif o == "-r": toc_in.append(a)
        elif o == "-v": verbose = 1

def _convert_map(input, output):
    line = input.readline()
    while line:
        line = line[:-1]
        if line[0:4] == "rect":
            url, x1y2, x2y1 = string.split(line[4:])
            x1, y2 = string.split(x1y2, ",")
            x2, y1 = string.split(x2y1, ",")
            output.write("<area href=\"" + url + "\" shape=\"rect\" coords=\"")
            output.write(str(x1) + ", " + str(y1) + ", " + str(x2) + ", " + str(y2) + "\">\n")
        line = input.readline()

def _format(input, output, format):
    command = "dot -T%s -o %s %s"%(format, output, input)
    if verbose: print "Dot Formatter: running command '" + command + "'"
    os.system(command)

def _format_png(input, output):
    tmpout = output + ".gif"
    _format(input, tmpout, "gif")
    command = "gif2png -O -d %s"%(tmpout)
    if verbose: print "Dot Formatter: running command '" + command + "'"
    os.system(command)
    if verbose: print "Dot Formatter: renaming '" + tmpout[:-4] + ".png'", "to '" + output + "'"
    os.rename(tmpout[:-4] + ".png", output)

def _format_html(input, output):
    if output[-5:] == ".html": output = output[:-5]
    label = string.join(string.split(title), "_")
    _format_png(input, label + ".png")
    _format(input, output + ".map", "imap")
    html = open(output + ".html", "w+")
    html.write("<img src=\"" + label + ".png\" border=\"0\" usemap=\"#")
    html.write(label + "_map\">\n")
    html.write("<map name=\"" + label + "_map\">")
    dotmap = open(output + ".map", "r+")
    _convert_map(dotmap, html)
    dotmap.close()
    os.remove(output + ".map")
    html.write("</map>\n")

def format(types, declarations, args):
    global output, title, type, oformat, verbose, toc, toc_in
    __parseArgs(args)

    # Create table of contents index
    if not toc: toc = TOC.TableOfContents(TOC.Linker())
    for t in toc_in: toc.load(t)

    tmpfile = output + ".dot"
    if verbose: print "Dot Formatter: Writing dot file..."
    dotfile = open(tmpfile, 'w+')
    dotfile.write("digraph \"%s\" {\n"%(title))
    dotfile.write("node[shape=box, fontsize=10, height=0.2, width=0.4, color=red]\n")
    if type == "inheritance":
        generator = InheritanceFormatter(dotfile)
    elif type == "single":
        generator = SingleInheritanceFormatter(dotfile, -1, types)
    else:
        generator = CollaborationFormatter(dotfile)
    for d in declarations:
        d.accept(generator)
    dotfile.write("}\n")
    dotfile.close()
    if oformat == "dot":
        os.rename(tmpfile, output)
    elif oformat == "png":
        _format_png(tmpfile, output)
        os.remove(tmpfile)
    elif oformat == "html":
        _format_html(tmpfile, output)
        os.remove(tmpfile)
    else:
        _format(tmpfile, output, oformat)
        os.remove(tmpfile)

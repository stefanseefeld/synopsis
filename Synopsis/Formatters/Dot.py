# $Id: Dot.py,v 1.20 2001/07/11 01:45:02 stefan Exp $
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
# Revision 1.20  2001/07/11 01:45:02  stefan
# fix Dot and HTML formatters to adjust the references depending on the filename of the output
#
# Revision 1.19  2001/06/28 07:22:18  stefan
# more refactoring/cleanup in the HTML formatter
#
# Revision 1.18  2001/06/26 04:32:15  stefan
# A whole slew of changes mostly to fix the HTML formatter's output generation,
# i.e. to make the output more robust towards changes in the layout of files.
#
# the rpm script now works, i.e. it generates source and binary packages.
#
# Revision 1.17  2001/06/06 04:44:54  uid20151
# Prune names of each class to the base which is the most common of its parents.
# Makes for simpler graphs with short names :)
#
# Revision 1.16  2001/05/25 13:45:49  stefan
# fix problem with getopt error reporting
#
# Revision 1.15  2001/04/06 02:38:14  chalky
# Dont reset toc
#
# Revision 1.14  2001/04/06 01:40:19  chalky
# Fixed the unknown nodes bug! Wasn't clearing the nodes dict on each run!
#
# Revision 1.13  2001/03/19 07:53:45  chalky
# Small fixes.
#
# Revision 1.12  2001/02/13 06:55:23  chalky
# Made synopsis -l work again
#
# Revision 1.11  2001/02/06 16:54:15  chalky
# Added -n to Dot which stops those nested classes
#
# Revision 1.10  2001/02/06 15:00:42  stefan
# Dot.py now references the template, not the last parameter, when displaying a Parametrized; replaced logo with a simple link
#
# Revision 1.9  2001/02/05 05:25:08  chalky
# Added hspace and vspace in _format_html
#
# Revision 1.8  2001/02/02 17:42:50  stefan
# cleanup in the Makefiles, more work on the Dot formatter
#
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
# THIS-IS-A-FORMATTER

import sys, tempfile, getopt, os, os.path, string, types, errno
from Synopsis.Core import AST, Type, Util
from Synopsis.Formatter import TOC

verbose = 0
toc = None
nodes = {}

class SystemError:
    """Error thrown by the system() function. Attributes are 'retval', encoded
    as per os.wait(): low-byte is killing signal number, high-byte is return
    value of command."""
    def __init__(self, retval, command):
	self.retval = retval
	self.command = command
    def __repr__(self):
	return "SystemError: %(retval)x\"%(command)s\" failed."%self.__dict__

def system(command):
    """Run the command. If the command fails, an exception SystemError is
    thrown."""
    ret = os.system(command)
    if (ret>>8) != 0:
	raise SystemError(ret, command)

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
        
    def writeNode(self, ref, label, **attr):
        """helper method to generate output for a given node"""
        if nodes.has_key(label): return
        nodes[label] = len(nodes)
        number = nodes[label]
        self.write("Node" + str(number) + " [shape=\"box\", label=\"" + label + "\"")
        self.write(", fontSize = 10, height = 0.2, width = 0.4")
        self.write(string.join(map(lambda item:', %s="%s"'%item, attr.items())))
        if ref: self.write(", URL=\"" + ref + "\"")
        self.write("];\n")

    def writeEdge(self, parent, child, label, **attr):
        self.write("Node" + str(nodes[parent]) + " -> ")
        self.write("Node" + str(nodes[child]))
        self.write("[ color=\"black\", fontsize=10" + string.join(map(lambda item:', %s="%s"'%item, attr.items())) + "];\n")

    def getClassName(self, node):
	"""Returns the name of the given class node, relative to all its
	parents. This makes the graph simpler by making the names shorter"""
	base = Util.pruneScope(node.name(), self.scope())
	for i in node.parents():
	    try:
		parent = i.parent()
		pname = Util.pruneScope(parent.name(), self.scope())
		for j in range(len(base)):
		    if j > len(pname) or pname[j] != base[j]:
			# Base is longer than parent name, or found a difference
			base[j:] = []
			break
	    except: pass # typedefs etc may cause errors here.. ignore
	if not node.parents(): base = []
	return Util.ccolonName(node.name(), base)

    #################### Type Visitor ###########################################
    def visitUnknown(self, type):
        self.__type_ref = toc[type.link()]
        self.__type_label = Util.ccolonName(type.name(), self.scope())
        
    def visitDeclared(self, type):
        self.__type_ref = toc[type.name()]
	if isinstance(type.declaration(), AST.Class):
	    self.__type_label = self.getClassName(type.declaration())
	else:
	    self.__type_label = Util.ccolonName(type.name(), self.scope())

    def visitParametrized(self, type):
	if type.template():
            type_ref = toc[type.template().name()]
	    type_label = Util.ccolonName(type.template().name(), self.scope())
	else:
            type_ref = None
	    type_label = "(unknown)"
        parameters_label = []
        for p in type.parameters():
            parameters_label.append(self.formatType(p))
        self.__type_ref = type_ref
        self.__type_label = type_label + "<" + string.join(parameters_label, ",") + ">"

    def visitTemplate(self, type):
        self.__type_ref = None
	self.__type_label = "template<%s>"%(string.join(map(lambda x:"typename "+x, map(self.formatType, type.parameters())), ","))

    #################### AST Visitor ############################################
        
    def visitInheritance(self, node):
        self.formatType(node.parent())
        if self.type_ref():
            self.writeNode(self.type_ref().link, self.type_label())
        else:
            self.writeNode('', self.type_label(), color='gray75', fontcolor='gray75')
        
    def visitClass(self, node):
        label = self.getClassName(node)
        ref = toc[node.name()]
        if ref:
            self.writeNode(ref.link, label)
        else:
            self.writeNode('', label, color='gray75', fontcolor='gray75')
        for inheritance in node.parents():
            inheritance.accept(self)
            self.writeEdge(self.type_label(), label, None)
	if no_descend: return
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
        self.__current = 1

    #################### Type Visitor ###########################################

    def visitDeclared(self, type):
        if self.__current < self.__levels or self.__levels == -1:
            self.__current = self.__current + 1
            type.declaration().accept(self)
            self.__current = self.__current - 1
        # to restore the ref/label...
        InheritanceFormatter.visitDeclared(self, type)
    #################### AST Visitor ############################################
        
    def visitInheritance(self, node):
        node.parent().accept(self)
        if self.type_label():
            if self.type_ref():
                self.writeNode(self.type_ref().link, self.type_label())
            else:
                self.writeNode('', self.type_label(), color='gray75', fontcolor='gray75')
        
    def visitClass(self, node):
        label = self.getClassName(node)
        if self.__current == 1:
            self.writeNode('', label, style='filled', color='lightgrey')
        else:
            ref = toc[node.name()]
            if ref:
                self.writeNode(ref.link, label)
            else:
                self.writeNode('', label, color='gray75', fontcolor='gray75')
        for inheritance in node.parents():
            inheritance.accept(self)
            if nodes.has_key(self.type_label()):
                self.writeEdge(self.type_label(), label, None)
        # if this is the main class and if there is a type dictionary,
        # look for classes that are derived from this class

        # if this is the main class
        if self.__current == 1 and self.__types:
            # fool the visitDeclared method to stop walking upwards
            self.__levels = 0
            for t in self.__types.values():
                if isinstance(t, Type.Declared):
                    child = t.declaration()
                    if isinstance(child, AST.Class):
                        for inheritance in child.parents():
                            type = inheritance.parent()
                            type.accept(self)
                            if self.type_ref():
                                if self.type_ref().name == node.name():
                                    child_label = self.getClassName(child)
                                    ref = toc[child.name()]
                                    if ref:
                                        self.writeNode(ref.link, child_label)
                                    else:
                                        self.writeNode('', child_label, color='gray75', fontcolor='gray75')
                                    self.writeEdge(label, child_label, None)

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
  -r <filename>                        Read in toc for an external data base that is to be referenced (for map/html output)
  -R <filename>                        Provide a reference URL which is used to compute relative links from the toc entries (for map/html output)    
  -n                                   Don't descend AST (for internal use)"""

formats = {
    'dot' : 'dot',
    'ps' : 'ps',
    'png' : 'png',
    'gif' : 'gif',
    'map' : 'imap',
    'html' : 'html',
}


def __parseArgs(args):
    global output, title, type, oformat, verbose, toc_in, origin, no_descend, nodes
    output = ''
    title = 'NoTitle'
    type = ''
    oformat = 'ps'
    toc_in = []
    origin = ''
    no_descend = 0
    nodes = {}
    try:
        opts,remainder = Util.getopt_spec(args, "o:t:f:r:R:icsvn")
    except Util.getopt.error, e:
        sys.stderr.write("Error in arguments: " + str(e) + "\n")
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
        elif o == "-R": origin = a
        elif o == "-v": verbose = 1
	elif o == "-n": no_descend = 1

def _rel(frm, to):
    "Find link to to relative to frm"
    print "comparing", frm, "to", to
    frm = string.split(frm, '/'); to = string.split(to, '/')
    for l in range((len(frm)<len(to)) and len(frm)-1 or len(to)-1):
        if to[0] == frm[0]: del to[0]; del frm[0]
        else: break
    if frm: to = ['..'] * (len(frm) - 1) + to
    return string.join(to,'/')

def _convert_map(input, output):
    """convert map generated from Dot to a html region map.
    input and output are (open) streams"""
    line = input.readline()
    while line:
        line = line[:-1]
        if line[0:4] == "rect":
            url, x1y2, x2y1 = string.split(line[4:])
            x1, y2 = string.split(x1y2, ",")
            x2, y1 = string.split(x2y1, ",")
            output.write("<area href=\"" + _rel(origin, url) + "\" shape=\"rect\" coords=\"")
            output.write(str(x1) + ", " + str(y1) + ", " + str(x2) + ", " + str(y2) + "\">\n")
        line = input.readline()

def _format(input, output, format):
    command = 'dot -T%s -o "%s" "%s"'%(format, output, input)
    if verbose: print "Dot Formatter: running command '" + command + "'"
    system(command)

def _format_png(input, output):
    tmpout = output + ".gif"
    _format(input, tmpout, "gif")
    command = 'gif2png -O -d "%s"'%(tmpout)
    if verbose: print "Dot Formatter: running command '" + command + "'"
    system(command)
    if verbose: print "Dot Formatter: renaming '" + tmpout[:-4] + ".png'", "to '" + output + "'"
    os.rename(tmpout[:-4] + ".png", output)

def _format_html(input, output):
    """generate (active) image for html.
    input and output are file names. If output ends
    in '.html', its stem is used with an '.png' suffix for the
    actual image."""
    if output[-5:] == ".html": output = output[:-5]
    _format_png(input, output + ".png")
    _format(input, output + ".map", "imap")
    prefix, reference = os.path.split(output + ".png")
    html = Util.open(output + ".html")
    html.write('<img src="' + reference + '" hspace="8" vspace="8" border="0" usemap="#')
    html.write(reference + "_map\">\n")
    html.write("<map name=\"" + reference + "_map\">")
    dotmap = open(output + ".map", "r+")
    _convert_map(dotmap, html)
    dotmap.close()
    os.remove(output + ".map")
    html.write("</map>\n")

def format(types, declarations, args, config_obj):
    global output, title, type, oformat, verbose, toc, toc_in
    __parseArgs(args)

    # Create table of contents index
    if not toc: toc = TOC.TableOfContents(TOC.Linker())
    for t in toc_in: toc.load(t)

    tmpfile = output + ".dot"
    if verbose: print "Dot Formatter: Writing dot file..."
    dotfile = Util.open(tmpfile)
    dotfile.write("digraph \"%s\" {\n"%(title))
    dotfile.write("node[shape=box, fontsize=10, height=0.2, width=0.4, color=black]\n")
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

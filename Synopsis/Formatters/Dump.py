# $Id: Dump.py,v 1.1 2003/10/09 05:02:39 stefan Exp $
#
# This file is a part of Synopsis.
# Copyright (C) 2003 Stefan Seefeld
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
# $Log: Dump.py,v $
# Revision 1.1  2003/10/09 05:02:39  stefan
# a new Dump formatter using xml for easy validation
#
#
"""
Verbose attribute-oriented xml dump of AST, useful for validation,
introspection, and debugging.
"""

# THIS-IS-A-FORMATTER
import sys, getopt, os, os.path, string, types
import xml
from xml.dom.minidom import getDOMImplementation
dom = getDOMImplementation().createDocument(None, "dump", None)

from Synopsis.Core import Type, AST

verbose = 0
__dom_implementation = xml.dom.minidom.getDOMImplementation()

class Writer:
    def __init__(self):
	self.handlers = {types.NoneType : self.visit_none,
                         types.TypeType : self.visit_type,
                         types.IntType : self.visit_string,
                         types.LongType : self.visit_string,
                         types.FloatType : self.visit_string,
                         types.StringType : self.visit_string,
                         types.TupleType : self.visit_tuple,
                         types.ListType : self.visit_list,
                         types.DictType : self.visit_dict,
                         types.InstanceType : self.visit_instance}
	self.visited = {}

    def push(self, name):
        element = dom.createElement(name)
        self.node.appendChild(element)
        self.node = element
    def pop(self):
        self.node = self.node.parentNode
    def add_text(self, text):
        node = dom.createTextNode(text)
        self.node.appendChild(node)

    def visit(self, obj):
        i,t = id(obj), type(obj)
        if self.visited.has_key(i):
            self.node.setAttribute('xref', str(i))
            return
        if self.handlers.has_key(t):
            self.handlers[t](obj)
        else:
            print "Unknown type %s for object: '%s'"%(t,obj)

    def visit_none(self, obj): pass
    def visit_string(self, obj): self.add_text(str(obj))
    def visit_type(self, obj): self.write(obj) # where is that used ??

    def visit_tuple(self, obj):
	if len(obj) == 0: return
	for i in obj:
            self.push('item')
	    self.visit(i)
            self.pop()

    def visit_list(self, obj):
	if len(obj) == 0: return
	for i in obj:
            self.push('item')
	    self.visit(i)
            self.pop()

    def visit_dict(self, dict):
	items = dict.items()
	if len(items) == 0: return
	items.sort()
	for i in items:
            self.push("key")
	    self.visit(i[0])
            self.pop()
            self.push("value")
	    self.visit(i[1])
            self.pop()

    def visit_instance(self, obj):
        if isinstance(obj, AST.SourceFile): # just write down the filename
            self.add_text(obj.filename())
	    return
        if isinstance(obj, AST.Include):
            self.write("Include: (macro:%d, next:%d) '%s'"%(obj.is_macro(),
                                                            obj.is_next(),
                                                            obj.target().filename()))
            return
        self.visited[id(obj)] = None
        self.push("%s.%s"%(obj.__class__.__module__,obj.__class__.__name__))
        self.node.setAttribute('id', str(id(obj)))
	attrs = obj.__dict__.items()
	attrs.sort()
	for name, value in attrs:
            # ignore None values
            if (value == None
                or value == []
                or value == ()):
                continue
            # special case for some known attributes...
            if name == '_Named__name':
                self.node.setAttribute('name', string.join(value, '.'))
                continue
            if name == '_Declaration__name':
                self.node.setAttribute('name', string.join(value, '.'))
                continue
            if name == '_Declaration__file':
                if value:
                    self.node.setAttribute('file', value.filename())
                continue
            if name[0] == '_':
                index = string.find(name, '__')
                if index >= 0:
                    #name = "%s.%s"%(name[1:index],name[index+2:])
                    name = name[index+2:]
            if (self.handlers[type(value)] == self.visit_string
                and not (obj.__class__.__name__ == 'Comment'
                         and (name == 'summary' or name == 'text'))):
                self.node.setAttribute(name, str(value))
            else:
                self.push(name)
                self.visit(value)
                self.pop()
        self.pop()

    def write_declarations(self, declarations):
        self.node = dom.createElement("declarations")
        for d in declarations: self.visit(d)
        self.node.writexml(output, indent=" ", addindent=" ", newl="\n")
        self.node.unlink()
        del self.node

    def write_types(self, types):
        self.node = dom.createElement("types")
        for t in types.values(): self.visit(t)
        self.node.writexml(output, indent=" ", addindent=" ", newl="\n")
        self.node.unlink()
        del self.node

    def write_files(self, files):
        self.node = dom.createElement("files")
        for f in files: self.visit(f)
        self.node.writexml(output, indent=" ", addindent=" ", newl="\n")
        self.node.unlink()
        del self.node

def usage():
    """Print usage to stdout"""
    print \
"""
  -o <file>                            Output file
  -d                                   Show declarations
  -t                                   Show types
  -f                                   Show files
 (If none of -d, -t or -f specified, will default to -d, -t)
"""

def __parseArgs(args):
    global output, verbose, show_decls, show_types, show_files
    # Set defaults
    output = sys.stdout
    show_decls = 0
    show_types = 0
    show_files = 0

    try:
        opts,remainder = getopt.getopt(args, "o:vdtfip")
    except getopt.error, e:
        sys.stderr.write("Error in arguments: " + str(e) + "\n")
        sys.exit(1)

    for opt in opts:
        o,a = opt
        if o == "-o": output = open(a, "w")
        elif o == "-v": verbose = 1
        elif o == "-d": show_decls = 1
        elif o == "-t": show_types = 1
        elif o == "-f": show_files = 1

    # Consolidate - if no show_ selected, show decls and types
    if not (show_decls or show_types or show_files):
        show_decls = 1
        show_types = 1
        show_files = 1

def format(args, ast, config_obj):
    global output, show_files
    __parseArgs(args)

    # Write out the prologue.
    output.write("<?xml version='1.0' encoding='ISO-8859-1'?>\n")
    output.write("<content>\n")
    writer = Writer()
    if show_decls:
        writer.write_declarations(ast.declarations())
    if show_types:
        writer.write_types(ast.types())
    if show_files:
        writer.write_files(ast.files())
    output.write("</content>\n")

#  $Id: HTML_Simple.py,v 1.13 2001/06/08 04:50:13 stefan Exp $
#
#  This file is a part of Synopsis.
#  Copyright (C) 2000, 2001 Stefan Seefeld
#
#  Synopsis is free software; you can redistribute it and/or modify it
#  under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#  General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
#  02111-1307, USA.
#
# $Log: HTML_Simple.py,v $
# Revision 1.13  2001/06/08 04:50:13  stefan
# add grouping support
#
# Revision 1.12  2001/05/25 13:45:49  stefan
# fix problem with getopt error reporting
#
# Revision 1.11  2001/05/13 02:46:41  stefan
# more fixes from oxygene
#
# Revision 1.10  2001/02/13 06:55:23  chalky
# Made synopsis -l work again
#
# Revision 1.9  2001/01/31 06:51:24  stefan
# add support for '-v' to all modules; modified toc lookup to use additional url as prefix
#
# Revision 1.8  2001/01/25 01:10:59  chalky
# Fixed html_simple
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
Simpler one-page HTML output
"""
# THIS-IS-A-FORMATTER

import sys, getopt, os, os.path, string
from Synopsis.Core import AST, Type, Util

verbose = 0

def entity(type, body): return "<" + type + "> " + body + "</" + type + ">"
def href(ref, label): return "<a href=" + ref + ">" + label + "</a>"
def name(ref, label): return "<a name=" + ref + ">" + label + "</a>"
def span(clas, body): return "<span class=\"" + clas + "\">" + body + "</span>"
def div(clas, body): return "<div class=\"" + clas + "\">" + body + "</div>"
def desc(text):
    block = filter(lambda s: s.text()[0:3] == "//.", text)
    if not len(block): return ""
    block = map(lambda s: s.text()[3:], block)
    return "<div class=\"desc\">" + string.join(block, '\n') + "</div>"

class TableOfContents(AST.Visitor):
    """
    generate a dictionary of all declarations which can be looked up to create
    cross references. Names are fully scoped."""
    def __init__(self): self.__toc__ = {}
    def write(self, os):
        keys = self.__toc__.keys()
        keys.sort()
        for key in keys:
            os.write(href("#" + self.__toc__[key], key) + "<br>\n")

    def lookup(self, name):
        if self.__toc__.has_key(name): return self.__toc__[name]
        else: return ""

    def insert(self, name):
        self.__toc__[Util.ccolonName(name)] = Util.dotName(name)

    def visitAST(self, node):
        for d in node.declarations: d.accept(this)
        
    def visitDeclarator(self, node):
        self.insert(node.name())

    def visitGroup(self, group):
        for declaration in group.declarations():
            declaration.accept(self)

    def visitModule(self, module):
        self.insert(module.name())
        for declaration in module.declarations():
            declaration.accept(self)

    def visitClass(self, clas):
        self.insert(clas.name())
        for declaration in clas.declarations():
            declaration.accept(self)

    def visitTypedef(self, typedef):
	self.insert(typedef.name())
    def visitEnumerator(self, enumerator):
        self.insert(enumerator.name())
    def visitEnum(self, enum):
        self.insert(enum.name())
        for e in enum.enumerators(): e.accept(self)
        
    def visitVariable(self, variable):
	self.insert(variable.name())
    def visitConst(self, const):
        self.insert(const.name())
    def visitParameter(self, parameter): pass
    def visitFunction(self, function):
        self.insert(function.name())

    def visitOperation(self, operation):
        self.insert(operation.name())
        for parameter in operation.parameters(): parameter.accept(self)
    

class HTMLFormatter (Type.Visitor, AST.Visitor):
    """
    The type visitors should generate names relative to the current scope.
    The generated references however are fully scoped names
    """
    def __init__(self, os, toc):
        self.__os = os
        self.__toc = toc
        self.__scope = []

    def scope(self): return self.__scope
    def write(self, text): self.__os.write(text)

    def reference(self, ref, label):
        """reference takes two strings, a reference (used to look up the symbol and generated the reference),
        and the label (used to actually write it)"""
        location = self.__toc.lookup(ref)
        if location != "": return href("#" + location, label)
        else: return span("type", str(label))
        
    def label(self, ref):
        location = self.__toc.lookup(Util.ccolonName(ref))
        ref = Util.ccolonName(ref, self.scope())
        if location != "": return name("\"" + location + "\"", ref)
        else: return ref

    #################### Type Visitor ###########################################

    def visitBaseType(self, type):
        self.__type_ref = Util.ccolonName(type.name())
        self.__type_label = Util.ccolonName(type.name())
        
    def visitUnknown(self, type):
        self.__type_ref = Util.ccolonName(type.name())
        self.__type_label = Util.ccolonName(type.name(), self.scope())
        
    def visitDeclared(self, type):
        self.__type_label = Util.ccolonName(type.name(), self.scope())
        self.__type_ref = Util.ccolonName(type.name())
        
    def visitModifier(self, type):
        type.alias().accept(self)
        self.__type_ref = string.join(type.premod()) + " " + self.__type_ref + " " + string.join(type.postmod())
        self.__type_label = string.join(type.premod()) + " " + self.__type_label + " " + string.join(type.postmod())
            
    def visitParametrized(self, type):
        type.template().accept(self)
        type_ref = self.__type_ref + "&lt;"
        type_label = self.__type_label + "&lt;"
        parameters_ref = []
        parameters_label = []
        for p in type.parameters():
            p.accept(self)
            parameters_ref.append(self.__type_ref)
            parameters_label.append(self.reference(self.__type_ref, self.__type_label))
        self.__type_ref = type_ref + string.join(parameters_ref, ", ") + "&gt;"
        self.__type_label = type_label + string.join(parameters_label, ", ") + "&gt;"

    def visitFunctionType(self, type):
	# TODO: this needs to be implemented
	self.__type_ref = 'function_type'
	self.__type_label = 'function_type'


    #################### AST Visitor ############################################
            
    def visitDeclarator(self, node):
        self.__declarator = node.name()
        for i in node.sizes():
            self.__declarator[-1] = self.__declarator[-1] + "[" + str(i) + "]"

    def visitTypedef(self, typedef):
        self.write(span("keyword", typedef.type()) + " ")
        typedef.alias().accept(self)
        self.write(self.reference(self.__type_ref, self.__type_label) + " ")
	self.write(self.label(typedef.name()))
        if len(typedef.comments()): self.write("\n" + desc(typedef.comments()) + "\n")            

    def visitVariable(self, variable):
        variable.vtype().accept(self)
        self.write(self.reference(self.__type_ref, self.__type_label) + " ")
	self.write(self.label(variable.name()))
        if len(variable.comments()): self.write("\n" + desc(variable.comments()) + "\n")            

    def visitConst(self, const):
        const.ctype().accept(self)
        self.write(span("keyword", const.type()) + " " + self.reference(self.__type_ref, self.__type_label) + " ")
        self.write(self.label(const.name()) + " = " + const.value())
        if len(const.comments()): self.write("\n" + desc(const.comments()) + "\n")            

    def visitGroup(self, group):
        self.write(span("keyword", group.type()) + "\n")
        if len(group.comments()): self.write("\n" + desc(group.comments()) + "\n")            
        self.write("<div class=\"group\">\n")
        for declaration in group.declarations():
            declaration.accept(self)
            self.write("<br>\n")
        self.write("</div>\n")

    def visitModule(self, module):
        self.write(span("keyword", module.type()) + " " + self.label(module.name()) + "\n")
        if len(module.comments()): self.write("\n" + desc(module.comments()) + "\n")            
        self.write("<div class=\"module\">\n")
        self.scope().append(module.name()[-1])
        for declaration in module.declarations():
            declaration.accept(self)
            self.write("<br>\n")
        self.scope().pop()
        self.write("</div>\n")

    def visitClass(self, clas):
        self.write(span("keyword", clas.type()) + " " + self.label(clas.name()) + "\n")
        if len(clas.parents()):
            self.write("<div class=\"parents\">\n")
            self.write(entity("b", "parents:") + "\n")
            self.write("<div class=\"inheritance\">\n")
            for parent in clas.parents():
                parent.accept(self)
                self.write("<br>\n")
            self.write("</div>\n")
            self.write("</div>\n")
        self.scope().append(clas.name()[-1])
        if len(clas.comments()): self.write("\n" + desc(clas.comments()) + "\n")            
        if len(clas.declarations()):
            self.write("<div class=\"declarations\">\n")
            for d in clas.declarations():
                d.accept(self)
                self.write("<br>\n")
            self.write("</div>\n")
        self.scope().pop()

    def visitInheritance(self, inheritance):
        for attribute in inheritance.attributes(): self.write(span("keywords", attribute) + " ")
	inheritance.parent().accept(self)
        self.write(self.reference(self.__type_ref, self.__type_label))

    def visitParameter(self, parameter):
        for m in parameter.premodifier(): self.write(span("keyword", m) + " ")
        parameter.type().accept(self)
        self.write(self.reference(self.__type_ref, self.__type_label) + " ")
        for m in parameter.postmodifier(): self.write(span("keyword", m) + " ")
        if len(parameter.identifier()) != 0:
            self.write(span("variable", parameter.identifier()))
            if len(parameter.value()) != 0:
                self.write(" = " + span("value", parameter.value()))

    def visitFunction(self, function):
        for modifier in operation.premodifier(): self.write(span("keyword", modifier) + " ")
        operation.returnType().accept(self)
        self.write(self.reference(self.__type_ref, self.__type_label) + " ")
        self.write(self.label(function.realname()) + "(")
        parameters = function.parameters()
        if len(parameters): parameters[0].accept(self)
        for parameter in parameters[1:]:
            self.write(", ")
            parameter.accept(self)
        self.write(")")
        for modifier in operation.postmodifier(): self.write(span("keyword", modifier) + " ")
        self.write("\n")
        if len(operation.exceptions()):
            self.write(span("keyword", "raises") + "\n")
            exceptions = []
            for exception in operation.exceptions():
                exceptions.append(self.reference(Util.ccolonName(exception.name()), Util.ccolonName(exception.name(), self.scope())))
            self.write("(" + span("raises", string.join(exceptions, ", ")) + ")")
        self.write("\n")
        if len(function.comments()): self.write("\n" + desc(function.comments()) + "\n")            

    def visitOperation(self, operation):
        for modifier in operation.premodifier(): self.write(span("keyword", modifier) + " ")
        if operation.returnType():
	    operation.returnType().accept(self)
	    self.write(self.reference(self.__type_ref, self.__type_label) + " ")
        if operation.language() == "IDL" and operation.type() == "attribute":
            self.write(span("keyword", "attribute") + " ")
        self.write(self.label(operation.realname()) + "(")
        parameters = operation.parameters()
        if len(parameters): parameters[0].accept(self)
        for parameter in parameters[1:]:
            self.write(", ")
            parameter.accept(self)
        self.write(")")
        for modifier in operation.postmodifier(): self.write(span("keyword", modifier) + " ")
        self.write("\n")
        if len(operation.exceptions()):
            self.write(span("keyword", "raises") + "\n")
            exceptions = []
            for exception in operation.exceptions():
                exceptions.append(self.reference(Util.ccolonName(exception.name()), Util.ccolonName(exception.name(), self.scope())))
            self.write("(" + span("raises", string.join(exceptions, ", ")) + ")")
        self.write("\n")
        if len(operation.comments()): self.write("\n" + desc(operation.comments()) + "\n")            

    def visitEnumerator(self, enumerator):
        self.write(self.label(enumerator.name()))
        if len(enumerator.value()):
            self.write(" = " + span("value", enumerator.value()))

    def visitEnum(self, enum):
        self.write(span("keyword", "enum ") + self.label(enum.name()) + "\n")
        self.write("<div class=\"enum\">\n")
        for enumerator in enum.enumerators():
            enumerator.accept(self)
            self.write("<br>\n")
        self.write("</div>\n")
        if len(enum.comments()): self.write("\n" + desc(enum.comments()) + "\n")            

def usage():
    """Print usage to stdout"""
    print \
"""
  -o <filename>                        Output filename
  -s <filename>                        Filename of stylesheet"""

def __parseArgs(args):
    global output, stylesheet, verbose
    output = sys.stdout
    stylesheet = ""
    try:
        opts,remainder = getopt.getopt(args, "o:s:v")
    except getopt.error, e:
        sys.stderr.write("Error in arguments: " + str(e) + "\n")
        sys.exit(1)

    for opt in opts:
        o,a = opt
        if o == "-o": output = open(a, "w")
        elif o == "-s": stylesheet = a
        elif o == "-v": verbose = 1

def format(types, declarations, args, config_obj):
    global output, stylesheet
    __parseArgs(args)
    output.write("<html>\n")
    output.write("<head>\n")
    if len(stylesheet):
        output.write("<link  rel=\"stylesheet\" href=\"" + stylesheet + "\">\n")
    output.write("</head>\n")
    output.write("<body>\n")
    toc = TableOfContents()
    for d in declarations:
        d.accept(toc)
    output.write(entity("h1", "Reference") + "\n")
    formatter = HTMLFormatter(output, toc)
    for d in declarations:
        d.accept(formatter)
    output.write(entity("h1", "Index") + "\n")
    toc.write(output)
    output.write("</body>\n")
    output.write("</html>\n")

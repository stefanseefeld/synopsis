#  $Id: Docbook.py,v 1.1 2001/01/27 06:01:21 stefan Exp $
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
# $Log: Docbook.py,v $
# Revision 1.1  2001/01/27 06:01:21  stefan
# a first take at a docbook formatter
#
#
"""a DocBook formatter (producing Docbook 4.2 XML output"""

import sys, getopt, os, os.path, string
from Synopsis.Core import AST, Type, Util

languages = {
    'C++': 'cxx',
    'IDL': 'idl'
    }

def start_entity(type, params=None):
    text = "<" + type
    if params: text = text + " " + string.join(map(lambda p:p[0]+"=\""+p[1]+"\"", params), ' ')
    text = text + ">"
    return text
def end_entity(type): return "</" + type + ">"
def entity(type, body, params=None):
    return start_entity(type, params) + "\n" + body + "\n" + end_entity(type)

class Formatter (Type.Visitor, AST.Visitor):
    """
    The type visitors should generate names relative to the current scope.
    The generated references however are fully scoped names
    """
    def __init__(self, os):
        self.__os = os
        self.__scope = []

    def scope(self): return self.__scope
    def write(self, text): self.__os.write(text + "\n")

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

    def type_label(self): return self.__type_label
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
        print "sorry, not implemented"

    def visitVariable(self, variable):
        self.write(start_entity("fieldsynopsis"))
        variable.vtype().accept(self)
        self.write(entity("type", self.type_label()))
        self.write(entity("varname", variable.name()[-1]))
        self.write(end_entity("fieldsynopsis"))

    def visitConst(self, const):
        print "sorry, not implemented"

    def visitModule(self, module):
        print "sorry, not implemented"

    def visitClass(self, clas):
        self.write(start_entity("classsynopsis", [("class", clas.type()), ("language", languages[clas.language()])]))
        self.write(entity("classname", Util.ccolonName(clas.name(), self.scope())))
        if len(clas.parents()):
            for parent in clas.parents(): parent.accept(self)
        self.scope().append(clas.name()[-1])
        for declaration in clas.declarations(): declaration.accept(self)
        self.scope().pop()
        self.write(end_entity("classsynopsis"))

    def visitInheritance(self, inheritance):
        self.write(map(lambda a:entity("modifier", a)+"\n", inheritance.attributes()))
        self.write(entity("classname", inheritance.parent().name()))

    def visitParameter(self, parameter):
        self.write(start_entity("methodparam"))
        self.write(map(lambda m:entity("modifier", a)+"\n", parameter.premodifiers()))
        parameter.type().accept(self)
        self.write(entity("type", self.type_label()))
        self.write(entity("parameter", parameter.name()))
        self.write(map(lambda m:entity("modifier", a)+"\n", parameter.postmodifiers()))
        self.write(end_entity("methodparam"))

    def visitFunction(self, function):
        print "sorry, not implemented"

    def visitOperation(self, operation):
        if operation.language() == "IDL" and operation.type() == "attribute":
            self.write(start_entity("fieldsynopsis"))
            self.write(map(lambda m:entity("modifier", m), operation.premodifiers()))
            self.write(entity("modifier", "attribute"))
            operation.returnType().accept(self)
            self.write(entity("type", self.type_label()))
            self.write(entity("varname", operation.realname()))
            self.write(end_entity("fieldsynopsis"))
        else:
            self.write(start_entity("methodsynopsis"))
            operation.returnType().accept(self)
            self.write(entity("type", self.type_label()))
            self.write(entity("methodname", operation.realname()))
            for parameter in operation.parameters(): parameter.accept(self)
            self.write(map(lambda e:entity("exceptionname", e), operation.exceptions()))
            self.write(end_entity("methodsynopsis"))

    def visitEnumerator(self, enumerator):
        print "sorry, not implemented"
    def visitEnum(self, enum):
        print "sorry, not implemented"

def usage():
    """Print usage to stdout"""
    print \
"""
  -o <filename>                        Output filename"""

def __parseArgs(args):
    global output, stylesheet
    output = sys.stdout
    stylesheet = ""
    try:
        opts,remainder = getopt.getopt(args, "o:")
    except getopt.error, e:
        sys.stderr.write("Error in arguments: " + e + "\n")
        sys.exit(1)

    for opt in opts:
        o,a = opt
        if o == "-o":
            output = open(a, "w")

def format(types, declarations, args):
    global output, stylesheet
    __parseArgs(args)
    output.write("<!DOCTYPE classsynopsis PUBLIC \"-//OASIS//DTD DocBook V4.2//EN\">\n")
    formatter = Formatter(output)
    for d in declarations:
        d.accept(formatter)

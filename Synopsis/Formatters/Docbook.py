#  $Id: Docbook.py,v 1.5 2001/02/13 06:55:23 chalky Exp $
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
# Revision 1.5  2001/02/13 06:55:23  chalky
# Made synopsis -l work again
#
# Revision 1.4  2001/02/11 19:33:32  stefan
# made the C++ parser formally accept the config object; synopsis now correctly formats if a formatter is present, and only dumps a syn file otherwise; some minor fixes to DocBook
#
# Revision 1.3  2001/02/11 05:39:33  stefan
# first try at a more powerful config framework for synopsis
#
# Revision 1.2  2001/01/31 06:51:24  stefan
# add support for '-v' to all modules; modified toc lookup to use additional url as prefix
#
# Revision 1.1  2001/01/27 06:01:21  stefan
# a first take at a docbook formatter
#
#
"""a DocBook formatter (producing Docbook 4.2 XML output"""
# THIS-IS-A-FORMATTER

import sys, getopt, os, os.path, string
from Synopsis.Core import AST, Type, Util

verbose = 0

languages = {
    'IDL': 'idl',
    'C++': 'cxx',
    'Python': 'python'
    }

class Formatter (Type.Visitor, AST.Visitor):
    """
    The type visitors should generate names relative to the current scope.
    The generated references however are fully scoped names
    """
    def __init__(self, os):
        self.__os = os
        self.__scope = []
        self.__indent = 0

    def scope(self): return self.__scope
    def write(self, text): self.__os.write(' ' * self.__indent + text + "\n")
    def start_entity(self, type, params=None):
        text = "<" + type
        if params: text = text + " " + string.join(map(lambda p:p[0]+"=\""+p[1]+"\"", params), ' ')
        text = text + ">"
        self.write(text)
        self.__indent = self.__indent + 2
    def end_entity(self, type):
        self.__indent = self.__indent - 2
        self.write("</" + type + ">")
    def entity(self, type, body, params=None):
        self.start_entity(type, params)
        self.write(body)
        self.end_entity(type)

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
        type_label = self.__type_label + "&lt;"
        parameters_label = []
        for p in type.parameters():
            p.accept(self)
            parameters_label.append(self.__type_label)
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
        print "sorry, <typedef> not implemented"

    def visitVariable(self, variable):
        self.start_entity("fieldsynopsis")
        variable.vtype().accept(self)
        self.entity("type", self.type_label())
        self.entity("varname", variable.name()[-1])
        self.end_entity("fieldsynopsis")

    def visitConst(self, const):
        print "sorry, <const> not implemented"

    def visitModule(self, module):
        self.scope().append(module.name()[-1])
        for declaration in module.declarations(): declaration.accept(self)
        self.scope().pop()

    def visitClass(self, clas):
        self.start_entity("classsynopsis", [("class", clas.type()), ("language", languages[clas.language()])])
        self.entity("classname", Util.ccolonName(clas.name(), self.scope()))
        if len(clas.parents()):
            for parent in clas.parents(): parent.accept(self)
        self.scope().append(clas.name()[-1])
        for declaration in clas.declarations(): declaration.accept(self)
        self.scope().pop()
        self.end_entity("classsynopsis")

    def visitInheritance(self, inheritance):
        map(lambda a, this=self: this.entity("modifier", a), inheritance.attributes())
        self.entity("classname", Util.ccolonName(inheritance.parent().name(), self.scope()))

    def visitParameter(self, parameter):
        self.start_entity("methodparam")
        map(lambda m, this=self: this.entity("modifier", m), parameter.premodifier())
        parameter.type().accept(self)
        self.entity("type", self.type_label())
        self.entity("parameter", parameter.identifier())
        map(lambda m, this=self: this.entity("modifier", m), parameter.postmodifier())
        self.end_entity("methodparam")

    def visitFunction(self, function):
        print "sorry, <function> not implemented"

    def visitOperation(self, operation):
        if operation.language() == "IDL" and operation.type() == "attribute":
            self.start_entity("fieldsynopsis")
            map(lambda m, this=self: this.entity("modifier", m), operation.premodifiers())
            self.entity("modifier", "attribute")
            operation.returnType().accept(self)
            self.entity("type", self.type_label())
            self.entity("varname", operation.realname())
            self.end_entity("fieldsynopsis")
        else:
            self.start_entity("methodsynopsis")
            ret = operation.returnType()
            if ret:
                ret.accept(self)
                self.entity("type", self.type_label())
            self.entity("methodname", Util.ccolonName(operation.realname(), self.scope()))
            for parameter in operation.parameters(): parameter.accept(self)
            map(lambda e, this=self: this.entity("exceptionname", e), operation.exceptions())
            self.end_entity("methodsynopsis")

    def visitEnumerator(self, enumerator):
        print "sorry, <enumerator> not implemented"
    def visitEnum(self, enum):
        print "sorry, <enum> not implemented"

def usage():
    """Print usage to stdout"""
    print \
"""
  -o <filename>                        Output filename"""

def __parseArgs(args):
    global output, verbose
    output = sys.stdout
    try:
        opts,remainder = getopt.getopt(args, "o:v")
    except getopt.error, e:
        sys.stderr.write("Error in arguments: " + e + "\n")
        sys.exit(1)

    for opt in opts:
        o,a = opt
        if o == "-o": output = open(a, "w")
        elif o == "-v": verbose = 1

def format(types, declarations, args, config):
    global output
    __parseArgs(args)
    output.write("<!DOCTYPE classsynopsis PUBLIC \"-//OASIS//DTD DocBook V4.2//EN\">\n")
    formatter = Formatter(output)
    for d in declarations:
        d.accept(formatter)

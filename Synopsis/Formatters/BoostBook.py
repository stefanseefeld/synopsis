#  $Id: BoostBook.py,v 1.2 2003/11/11 12:50:56 stefan Exp $
#
#  This file is a part of Synopsis.
#  Copyright (C) 2003 by Stephen Davies
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
# $Log: BoostBook.py,v $
# Revision 1.2  2003/11/11 12:50:56  stefan
# remove 'Core' module
#
# Revision 1.1  2003/03/06 10:02:28  chalky
# First cut at a BoostBook formatter
#

"""a BoostBook formatter"""
# THIS-IS-A-FORMATTER

import sys, getopt, os, os.path, string, re
from Synopsis import AST, Type, Util

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
        self.__scope = ()
	self.__scopestack = []
        self.__indent = 0

    def scope(self): return self.__scope
    def push_scope(self, newscope):
	self.__scopestack.append(self.__scope)
	self.__scope = newscope
    def pop_scope(self):
	self.__scope = self.__scopestack[-1]
	del self.__scopestack[-1]
    def write(self, text):
	"""Write some text to the output stream, replacing \n's with \n's and
	indents."""
	indent = ' ' * self.__indent
	self.__os.write(text.replace('\n', '\n'+indent))
    def start_entity(self, __type, **__params):
	"""Write the start of an entity, ending with a newline"""
	param_text = ""
        if __params: param_text = " " + string.join(map(lambda p:'%s="%s"'%(p[0].lower(), p[1]), __params.items()))
        self.write("<" + __type + param_text + ">")
        self.__indent = self.__indent + 2
	self.write("\n")
    def end_entity(self, type):
	"""Write the end of an entity, starting with a newline"""
        self.__indent = self.__indent - 2
        self.write("\n</" + type + ">")
    def write_entity(self, __type, __body, **__params):
	"""Write a single entity on one line (though body may contain
	newlines)"""
	param_text = ""
        if __params: param_text = " " + string.join(map(lambda p:'%s="%s"'%(p[0].lower(), p[1]), __params.items()))
        self.write("<" + __type + param_text + ">")
        self.__indent = self.__indent + 2
        self.write(__body)
        self.__indent = self.__indent - 2
        self.write("</" + __type + ">")
    def entity(self, __type, __body, **__params):
	"""Return but do not write the text for an entity on one line"""
	param_text = ""
        if __params: param_text = " " + string.join(map(lambda p:'%s="%s"'%(p[0].lower(), p[1]), __params.items()))
        return "<%s%s>%s</%s>"%(__type, param_text, __body, __type)

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

    def formatType(self, type):
	type.accept(self)
	return self.__type_label

    def visitFunctionType(self, type):
        # TODO: this needs to be implemented
        self.__type_ref = 'function_type'
        self.__type_label = 'function_type'

    def visitComment(self, comment):
        text = comment.text()
        text = text.replace('\n\n', '</para><para>')
        self.write(self.entity("para", text)+'\n')

    #################### AST Visitor ############################################
            
    def visitDeclarator(self, node):
        self.__declarator = node.name()
        for i in node.sizes():
            self.__declarator[-1] = self.__declarator[-1] + "[" + str(i) + "]"

    def visitTypedef(self, typedef):
	self.start_entity("typedef", name=Util.ccolonName(self.scope(), typedef.name()))
	self.write_entity("type", self.formatType(typedef.alias()))
	self.end_entity("typedef")

    def visitVariable(self, variable):
        self.start_entity("fieldsynopsis")
        variable.vtype().accept(self)
        self.entity("type", self.type_label())
        self.entity("varname", variable.name()[-1])
        self.end_entity("fieldsynopsis")

    def visitConst(self, const):
        print "sorry, <const> not implemented"

    def visitModule(self, module):
        self.start_entity("namespace", name=Util.ccolonName(self.scope(), module.name()))
	self.write("\n")
        map(self.visitComment, module.comments())
        self.push_scope(module.name())
        for declaration in module.declarations(): declaration.accept(self)
        self.pop_scope()
        self.end_entity("namespace")

    def visitClass(self, clas):
	self.start_entity("class", name=Util.ccolonName(self.scope(), clas.name()))
	# clas.type()
        if len(clas.parents()):
            for parent in clas.parents(): parent.accept(self)
	self.push_scope(clas.name())
        if clas.comments():
            self.start_entity("purpose")
            map(self.visitComment, clas.comments())
            self.end_entity("purpose")
        for declaration in clas.declarations():
	    declaration.accept(self)
        self.pop_scope()
        self.end_entity("class")

    def visitInheritance(self, inheritance):
	if len(inheritance.attributes()):
	    self.start_entity("inherit", access=inheritance.attributes()[0])
	else:
	    self.start_entity("inherit")
        self.write_entity("classname", self.formatType(inheritance.parent()))
	self.end_entity("inherit")

    def visitParameter(self, parameter):
        self.start_entity("parameter", name=parameter.identifier())
	self.write_entity("param_type", self.formatType(parameter.type()))
        #map(lambda m, this=self: this.write_entity("modifier", m), parameter.premodifier())
        #map(lambda m, this=self: this.write_entity("modifier", m), parameter.postmodifier())
        self.end_entity("parameter")
	self.write("\n")

    def visitFunction(self, function):
	self.start_entity("function", name=Util.ccolonName(self.scope(), function.realname()))
	self.do_function(function)
	self.end_entity("function")
	self.write("\n")

    def visitOperation(self, operation):
	name = operation.name()
	tag = None
	if len(name) > 1:
	    if name[-1] == name[-2]:
		tag = "constructor"
		self.start_entity(tag)
	    elif name[-1] == "~"+name[-2]:
		tag = "destructor"
		self.start_entity(tag)
	if tag is None:
	    tag = "method"
	    self.start_entity(tag, name=Util.ccolonName(self.scope(), operation.realname()))
	self.do_function(operation)
	self.end_entity(tag)
	self.write("\n")

    def do_function(self, func):
	"""Stuff common to functions and methods, contructors, destructors"""
	for parameter in func.parameters(): parameter.accept(self)
	if func.returnType():
	    self.write_entity("type", self.formatType(func.returnType()))
	    self.write("\n")
	if func.comments():
	    self.start_entity("purpose")
	    map(self.visitComment, func.comments())
	    self.end_entity("purpose")
	    self.write("\n")
	
	if func.exceptions():
	    self.start_entity("throws")
	    for ex in func.exceptions():
		self.write_entity("simpara", ex)
	    self.end_entity("throws")
	    self.write("\n")

    def visitEnumerator(self, enumerator):
        print "sorry, <enumerator> not implemented"
    def visitEnum(self, enum):
        print "sorry, <enum> not implemented"

def usage():
    """Print usage to stdout"""
    print \
"""
  -v                         Verbose mode
  -o <filename>              Output filename
  -d			     Don't ouput a DOCTYPE tag"""

def __parseArgs(args):
    global output, verbose, no_doctype, is_manual
    output = sys.stdout
    no_doctype = 0
    is_manual = 0
    try:
        opts,remainder = getopt.getopt(args, "o:vdm")
    except getopt.error, e:
        sys.stderr.write("Error in arguments: " + str(e) + "\n")
        sys.exit(1)

    for opt in opts:
        o,a = opt
        if o == "-o": output = open(a, "w")
        elif o == "-v": verbose = 1
        elif o == "-d": no_doctype = 1
        elif o == "-m": is_manual = 1

def format(args, ast, config):
    global output
    __parseArgs(args)
    #if not no_doctype:
    #	output.write("<!DOCTYPE classsynopsis PUBLIC \"-//OASIS//DTD DocBook V4.2//EN\">\n")
    formatter = Formatter(output)
    for d in ast.declarations():
        d.accept(formatter)

    if output is not sys.stdout:
	output.close()

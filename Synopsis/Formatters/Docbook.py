#  $Id: Docbook.py,v 1.8 2002/11/18 11:49:29 chalky Exp $
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
# Revision 1.8  2002/11/18 11:49:29  chalky
# Tried to fix it up, but the *synopsis docbook entities aren't what we need.
# Shortcut to what I needed and just wrote a DocFormatter for the Synopsis
# DocBook manual's Config section.
#
# Revision 1.7  2001/07/19 04:03:05  chalky
# New .syn file format.
#
# Revision 1.6  2001/05/25 13:45:49  stefan
# fix problem with getopt error reporting
#
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

import sys, getopt, os, os.path, string, re
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
        self.start_entity("section")
        self.write_entity("title", module.type()+" "+Util.ccolonName(module.name()))
	self.write("\n")
        map(self.visitComment, module.comments())
        self.push_scope(module.name())
        for declaration in module.declarations(): declaration.accept(self)
        self.pop_scope()
        self.end_entity("section")

    def visitClass(self, clas):
        self.start_entity("classsynopsis", Class=clas.type(), language=languages[clas.language()])
	classname = self.entity("classname", Util.ccolonName(clas.name()))
        self.write_entity("ooclass", classname)
        self.start_entity("classsynopsisinfo")
        if len(clas.parents()):
            for parent in clas.parents(): parent.accept(self)
	self.push_scope(clas.name())
        if clas.comments():
            self.start_entity("textobject")
            map(self.visitComment, clas.comments())
            self.end_entity("textobject")
        self.end_entity("classsynopsisinfo")
        classes = []
        for declaration in clas.declarations():
            # Store classes for later
            if isinstance(declaration, AST.Class):
                classes.append(declaration)
            else:
                declaration.accept(self)
        self.pop_scope()
        self.end_entity("classsynopsis")
        # Classes can't be nested (in docbook 4.2), so do them after
        for clas in classes: clas.accept(self)

    def visitInheritance(self, inheritance):
        map(lambda a, this=self: this.entity("modifier", a), inheritance.attributes())
        self.entity("classname", Util.ccolonName(inheritance.parent().name(), self.scope()))

    def visitParameter(self, parameter):
        self.start_entity("methodparam")
        map(lambda m, this=self: this.write_entity("modifier", m), parameter.premodifier())
        parameter.type().accept(self)
        self.write_entity("type", self.type_label())
        self.write_entity("parameter", parameter.identifier())
        map(lambda m, this=self: this.write_entity("modifier", m), parameter.postmodifier())
        self.end_entity("methodparam")

    def visitFunction(self, function):
        print "sorry, <function> not implemented"

    def visitOperation(self, operation):
        if operation.language() == "IDL" and operation.type() == "attribute":
            self.start_entity("fieldsynopsis")
            map(lambda m, this=self: this.entity("modifier", m), operation.premodifiers())
            self.write_entity("modifier", "attribute")
            operation.returnType().accept(self)
            self.write_entity("type", self.type_label())
            self.write_entity("varname", operation.realname())
            self.end_entity("fieldsynopsis")
        else:
            self.start_entity("methodsynopsis")
	    if operation.language() != "Python":
		ret = operation.returnType()
		if ret:
		    ret.accept(self)
		    self.write_entity("type", self.type_label())
	    else:
		self.write_entity("modifier", "def")
            self.write_entity("methodname", Util.ccolonName(operation.realname(), self.scope()))
            for parameter in operation.parameters(): parameter.accept(self)
            map(lambda e, this=self: this.entity("exceptionname", e), operation.exceptions())
            self.end_entity("methodsynopsis")

    def visitEnumerator(self, enumerator):
        print "sorry, <enumerator> not implemented"
    def visitEnum(self, enum):
        print "sorry, <enum> not implemented"

class DocFormatter (Formatter):
    """A specialized version that just caters for the needs of the DocBook
    manual's Config section. Only modules and classes are printed, and the
    docbook elements classsynopsis etc are not used."""
    _re_tags = '(?P<text>.*?)\n[ \t]*(?P<tags>@[a-zA-Z]+[ \t]+.*)'
    def __init__(self, os):
	Formatter.__init__(self, os)
	self.re_tags = re.compile(self._re_tags, re.M|re.S)
    def parseTags(self, str, joiner):
	"""Returns text, tags"""
	# Find tags
	mo = self.re_tags.search(str)
	if not mo: return str, ''
	str, tags = mo.group('text'), mo.group('tags')
	# Split the tag section into lines
	tags = map(string.strip, string.split(tags,'\n'))
	# Join non-tag lines to the previous tag
	tags = reduce(joiner, tags, [])
	return str, tags

    def visitComment(self, comment):
        text = comment.text()
	see_tags, attr_tags = [], []
	joiner = lambda x,y: len(y) and y[0]=='@' and x+[y] or x[:-1]+[x[-1]+' '+y]
	text, tags = self.parseTags(text, joiner)
	# Parse each of the tags
	for line in tags:
	    tag, rest = string.split(line,' ',1)
	    if tag == '@see':
		see_tags.append(rest)
	    elif tag == '@attr':
		attr_tags.append(string.split(rest,' ',1))
	# Do the body of the comment
        text = text.replace('\n\n', '</para><para>')
        text = text.replace('<heading>', '<emphasis>')
        text = text.replace('</heading>', '</emphasis>')
        text = text.replace('<example>', '<programlisting>')
        text = text.replace('</example>', '</programlisting>')
        self.write(self.entity("para", text)+'\n')
	# Do the attributes
	if len(attr_tags):
	    self.start_entity('variablelist')
	    self.write_entity('title', 'Attributes:')
	    for attr, desc in attr_tags:
		self.start_entity('varlistentry')
		self.write_entity('term', attr)
		self.write_entity('listitem', self.entity('para', desc))
		self.end_entity('varlistentry')
	    self.end_entity('variablelist')
	# Do the see-also
	if len(see_tags):
	    self.start_entity('itemizedlist')
	    self.write_entity('title', 'See also:')
	    for text in see_tags:
		self.write_entity('listitem', self.entity('para', text))
	    self.end_entity('itemizedlist')

    def visitModule(self, module):
        self.start_entity("section")
        self.write_entity("title", module.type()+" "+Util.ccolonName(module.name()))
	self.write("\n")
        map(self.visitComment, module.comments())
        self.end_entity("section")
        self.push_scope(module.name())
        for declaration in module.declarations():
	    if isinstance(declaration, AST.Class):
                self.visitClass(declaration)
        self.pop_scope()

    def visitClass(self, clas):
        self.start_entity("section")
	if len(clas.name()) > 3 and clas.name()[:2] == ("Config.py", "Base"):
	    self.write_entity("title", clas.name()[2]+" class "+Util.ccolonName(clas.name()[3:], self.scope()))
	else:
	    self.write_entity("title", "Class "+Util.ccolonName(clas.name(), self.scope()))
        if len(clas.parents()):
            for parent in clas.parents():
		self.visitInheritance(parent)
	map(self.visitComment, clas.comments())
        self.end_entity("section")
        for declaration in clas.declarations():
            if isinstance(declaration, AST.Class):
                self.visitClass(declaration)

    def visitInheritance(self, inheritance):
        self.write_entity("para", "Inherits "+Util.ccolonName(inheritance.parent().name(), self.scope()))


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
    if not no_doctype:
	output.write("<!DOCTYPE classsynopsis PUBLIC \"-//OASIS//DTD DocBook V4.2//EN\">\n")
    if is_manual:
	formatter = DocFormatter(output)
    else:
	formatter = Formatter(output)
    for d in ast.declarations():
        d.accept(formatter)

    if output is not sys.stdout:
	output.close()

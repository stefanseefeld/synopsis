#  $Id: Texinfo.py,v 1.1 2001/06/11 19:45:15 stefan Exp $
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
# $Log: Texinfo.py,v $
# Revision 1.1  2001/06/11 19:45:15  stefan
# initial work on a TexInfo formatter
#
#
#
"""a TexInfo formatter """
# THIS-IS-A-FORMATTER

import sys, getopt, os, os.path, string, re
from Synopsis.Core import AST, Type, Util

verbose = 0

class Formatter (Type.Visitor, AST.Visitor):
    """
    The type visitors should generate names relative to the current scope.
    The generated references however are fully scoped names
    """
    __doc_hierarchy = ['chapter', 'section', 'subsection', 'subsubsection']
    __re_ssd = r'^[ \t]*//\. ?(.*)$'
    def __init__(self, os):
        self.__os = os
        self.__scope = []
        self.__indent = 0
        self.__current_entity = 0
	self.re_ssd = re.compile(Formatter.__re_ssd, re.M)
    def push_doc(self):
        self.__current_entity = self.__current_entity + 1
        if (self.__current_entity > len(Formatter.__doc_hierarchy)):
            print "error: texinfo only supports four levels in the document hierarchy. No more sublevels available"
            sys.exit(-1)
    def pop_doc(self):
        self.__current_entity = self.__current_entity - 1
        if (self.__current_entity < 0):
            print "error: can't get up higher than that"
            sys.exit(-1)
    def new_section(self, label):
        self.write("@" + Formatter.__doc_hierarchy[self.__current_entity] + " " + label + "\n")
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

    def type_label(self): return self.__type_label

    def formatType(self, type):
	"Returns a reference string for the given type object"
	if type is None: return "(unknown)"
        type.accept(self)
        return self.__type_label

    def formatComments(self, decl):
        for c in decl.comments(): self.write(c.text() + '\n')

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
        type_label = self.__type_label + "<"
        parameters_label = []
        for p in type.parameters():
            p.accept(self)
            parameters_label.append(self.__type_label)
        self.__type_label = type_label + string.join(parameters_label, ", ") + ">"

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
        self.write('@deftp ' + typedef.type() + ' {' + self.formatType(typedef.alias()) + '} {' + Util.ccolonName(typedef.name(), self.scope()) + '}\n')
        self.formatComments(typedef)
        self.write('@end deftp\n')
    def visitVariable(self, variable):
        self.write('@deftypevr {' + variable.type() + '} {' + self.formatType(variable.vtype()) + '} {' + Util.ccolonName(variable.name(), self.scope()) + '}\n')
        self.formatComments(variable)
        self.write('@end deftypevr\n')

    def visitConst(self, const):
        print "sorry, <const> not implemented"

    def visitModule(self, module):
        self.new_section(module.name()[-1])
        self.push_doc()
        self.formatComments(module)
        self.scope().append(module.name()[-1])
        for declaration in module.declarations(): declaration.accept(self)
        self.scope().pop()
        self.pop_doc()

    def visitClass(self, clas):
        self.new_section(clas.name()[-1])
        self.push_doc()
        if len(clas.parents()):
            self.write('parents:')
            first = 1
            for parent in clas.parents():
                if not first: self.write(', ')
                else: self.write(' ')
                parent.accept(self)
            self.write('\n')
        self.formatComments(clas)
        self.scope().append(clas.name()[-1])
        for declaration in clas.declarations(): declaration.accept(self)
        self.scope().pop()
        self.pop_doc()

    def visitInheritance(self, inheritance):
        #map(lambda a, this=self: this.entity("modifier", a), inheritance.attributes())
        #self.entity("classname", Util.ccolonName(inheritance.parent().name(), self.scope()))
        self.write('parent class')

    def visitParameter(self, parameter):
        #map(lambda m, this=self: this.entity("modifier", m), parameter.premodifier())
        parameter.type().accept(self)
        label = self.write('{' + self.type_label() + '}')
        label = self.write(' @var{' + parameter.identifier() + '}')
        #map(lambda m, this=self: this.entity("modifier", m), parameter.postmodifier())

    def visitFunction(self, function):
        ret = function.returnType()
        if ret:
            ret.accept(self)
            ret_label = '{' + self.type_label() + '}'
        else:
            ret_label = '{}'
        self.write('@deftypefn ' + function.type() + ' ' + ret_label + ' ' + Util.ccolonName(function.realname(), self.scope()) + '(')
        first = 1
        for parameter in function.parameters():
            if not first: self.write(', ')
            else: self.write(' ')
            parameter.accept(self)
            first = 0
        self.write(')\n')
        #    map(lambda e, this=self: this.entity("exceptionname", e), operation.exceptions())
        self.formatComments(operation)
        self.write('@end deftypefn\n')

    def visitOperation(self, operation):
        ret = operation.returnType()
        if ret:
            ret.accept(self)
            ret_label = '{' + self.type_label() + '}'
        else:
            ret_label = '{}'
        self.write('@deftypefn ' + operation.type() + ' ' + ret_label + ' ' + Util.ccolonName(operation.realname(), self.scope()) + '(')
        first = 1
        for parameter in operation.parameters():
            if not first: self.write(', ')
            else: self.write(' ')
            parameter.accept(self)
            first = 0
        self.write(')\n')
        #    map(lambda e, this=self: this.entity("exceptionname", e), operation.exceptions())
        self.formatComments(operation)
        self.write('@end deftypefn\n')

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
        sys.stderr.write("Error in arguments: " + str(e) + "\n")
        sys.exit(1)

    for opt in opts:
        o,a = opt
        if o == "-o": output = open(a, "w")
        elif o == "-v": verbose = 1

def format(types, declarations, args, config):
    global output
    __parseArgs(args)
    formatter = Formatter(output)
    for d in declarations:
        d.accept(formatter)

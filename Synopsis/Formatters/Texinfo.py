#
# Copyright (C) 2000 Stefan Seefeld
# Copyright (C) 2000 Stephen Davies
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

"""a TexInfo formatter """

from Synopsis.Processor import Processor, Parameter
from Synopsis import AST, Type, Util
from Synopsis.DocString import DocString
import sys, getopt, os, os.path, string, re


_tags = re.compile(r'@(?!(table|item|samp|end))')
def escape(text):

   return _tags.sub('@@', text).replace('{', '@{').replace('}', '@}')


class MenuMaker(AST.Visitor):
   """generate a texinfo menu for the declarations of a given scope"""

   def __init__(self, scope, os):

      self.__scope = scope
      self.__os = os

   def write(self, text): self.__os.write(text)
   def start(self): self.write('@menu\n')
   def end(self): self.write('@end menu\n')
   def visitDeclaration(self, node):

      name = escape(Util.dotName(node.name(), self.__scope))
      self.write('* ' + name + '::\t' + node.type() + '\n')

   visitGroup = visitDeclaration
   visitEnum = visitDeclaration

class Formatter(Processor, Type.Visitor, AST.Visitor):
   """The type visitors should generate names relative to the current scope.
   The generated references however are fully scoped names."""

   def process(self, ir, **kwds):

      self.set_parameters(kwds)
      self.ir = self.merge_input(ir)

      self.__os = open(self.output, 'w+')
      self.__scope = []
      self.__indent = 0

      for d in self.ir.declarations:
         d.accept(self)

      return self.ir

   def scope(self): return self.__scope
   def write(self, text): self.__os.write(text)

   def type_label(self): return escape(self.__type_label)

   def decl_label(self, decl): return escape(decl[-1])

   def format_type(self, type):
      "Returns a reference string for the given type object"

      if type is None: return "(unknown)"
      type.accept(self)
      return self.type_label()

   def format_comments(self, decl):

      doc = decl.annotations.get('doc', DocString('', ''))
      # TODO: implement markup formatters for e.g. javadoc and rst
      self.write(escape(doc.text) + '\n')

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

      if type.template():
         type.template().accept(self)
         type_label = self.__type_label + "<"
      else: type_label = "(unknown)<"
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

      #self.write('@node ' + self.decl_label(typedef.name()) + '\n')
      self.write('@deftp ' + typedef.type() + ' {' + self.format_type(typedef.alias()) + '} {' + self.decl_label(typedef.name()) + '}\n')
      self.format_comments(typedef)
      self.write('@end deftp\n')

   def visitVariable(self, variable):

      #self.write('@node ' + self.decl_label(variable.name()) + '\n')
      self.write('@deftypevr {' + variable.type() + '} {' + self.format_type(variable.vtype()) + '} {' + self.decl_label(variable.name()) + '}\n')
      #FIXME !: how can values be represented in texinfo ?
      self.format_comments(variable)
      self.write('@end deftypevr\n')

   def visitConst(self, const):

      print "sorry, <const> not implemented"

   def visitModule(self, module):

      #self.write('@node ' + self.decl_label(module.name()) + '\n')
      self.write('@deftp ' + module.type() + ' ' + self.decl_label(module.name()) + '\n')
      self.format_comments(module)
      self.scope().append(module.name()[-1])
      #menu = MenuMaker(self.scope(), self.__os)
      #menu.start()
      #for declaration in module.declarations(): declaration.accept(menu)
      #menu.end()
      for declaration in module.declarations(): declaration.accept(self)
      self.scope().pop()
      self.write('@end deftp\n')

   def visitClass(self, clas):

      #self.write('@node ' + self.decl_label(clas.name()) + '\n')
      self.write('@deftp ' + clas.type() + ' ' + self.decl_label(clas.name()) + '\n')
      if len(clas.parents()):
         self.write('parents:')
         first = 1
         for parent in clas.parents():
            if not first: self.write(', ')
            else: self.write(' ')
            parent.accept(self)
         self.write('\n')
      self.format_comments(clas)
      self.scope().append(clas.name()[-1])
      #menu = MenuMaker(self.scope(), self.__os)
      #menu.start()
      #for declaration in clas.declarations(): declaration.accept(menu)
      #menu.end()
      for declaration in clas.declarations(): declaration.accept(self)
      self.scope().pop()
      self.write('@end deftp\n')

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
      #self.write('@node ' + self.decl_label(function.realname()) + '\n')
      self.write('@deftypefn ' + function.type() + ' ' + ret_label + ' ' + self.decl_label(function.realname()) + ' (')
      first = 1
      for parameter in function.parameters():
         if not first: self.write(', ')
         else: self.write(' ')
         parameter.accept(self)
         first = 0
      self.write(')\n')
      #    map(lambda e, this=self: this.entity("exceptionname", e), operation.exceptions())
      self.format_comments(function)
      self.write('@end deftypefn\n')

   def visitOperation(self, operation):

      ret = operation.returnType()
      if ret:
         ret.accept(self)
         ret_label = '{' + self.type_label() + '}'
      else:
         ret_label = '{}'
      try:
         #self.write('@node ' + self.decl_label(operation.name()) + '\n')
         self.write('@deftypeop ' + operation.type() + ' ' + self.decl_label(self.scope()) + ' ' + ret_label + ' ' + self.decl_label(operation.realname()) + ' (')
      except:
         print operation.realname()
         sys.exit(0)
      first = 1
      for parameter in operation.parameters():
         if not first: self.write(', ')
         else: self.write(' ')
         parameter.accept(self)
         first = 0
      self.write(')\n')
      #    map(lambda e, this=self: this.entity("exceptionname", e), operation.exceptions())
      self.format_comments(operation)
      self.write('@end deftypeop\n')

   def visitEnumerator(self, enumerator):

      self.write('@deftypevr {' + enumerator.type() + '} {} {' + self.decl_label(enumerator.name()) + '}')
      #FIXME !: how can values be represented in texinfo ?
      if enumerator.value(): self.write('\n')
      else: self.write('\n')
      self.format_comments(enumerator)
      self.write('@end deftypevr\n')

   def visitEnum(self, enum):
      #self.write('@node ' + self.decl_label(enum.name()) + '\n')
      self.write('@deftp ' + enum.type() + ' ' + self.decl_label(enum.name()) + '\n')
      self.format_comments(enum)
      for enumerator in enum.enumerators(): enumerator.accept(self)
      self.write('@end deftp\n')

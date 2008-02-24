#
# Copyright (C) 2000 Stefan Seefeld
# Copyright (C) 2000 Stephen Davies
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

"""a TexInfo formatter """

from Synopsis.Processor import *
from Synopsis import ASG
from Synopsis.DocString import DocString
import sys, os, re


_tags = re.compile(r'@(?!(table|item|samp|end))')
def escape(text):

   return _tags.sub('@@', text).replace('{', '@{').replace('}', '@}')


class MenuMaker(ASG.Visitor):
   """generate a texinfo menu for the declarations of a given scope"""

   def __init__(self, scope, os):

      self.scope = scope
      self.os = os

   def write(self, text): self.os.write(text)
   def start(self): self.write('@menu\n')
   def end(self): self.write('@end menu\n')
   def visit_declaration(self, node):

      name = escape(str(self.scope.prune(node.name)))
      self.write('* ' + name + '::\t' + node.type + '\n')

   visit_group = visit_declaration
   visit_enum = visit_declaration

class Formatter(Processor, ASG.Visitor):
   """The type visitors should generate names relative to the current scope.
   The generated references however are fully scoped names."""

   def process(self, ir, **kwds):

      self.set_parameters(kwds)
      if not self.output: raise MissingArgument('output')
      self.ir = self.merge_input(ir)

      self.os = open(self.output, 'w+')
      self.scope = ()
      for d in self.ir.declarations:
         d.accept(self)

      return self.ir

   def write(self, text): self.os.write(text)

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

   def visit_base_type(self, type):

      self.__type_ref = str(type.name)
      self.__type_label = str(type.name)
        
   def visit_unknown_type(self, type):

      self.__type_ref = str(type.name)
      self.__type_label = str(self.scope.prune(type.name))
        
   def visit_declared_type(self, type):

      self.__type_label = str(self.scope.prune(type.name))
      self.__type_ref = str(type.name)
        
   def visit_modifier_type(self, type):

      type.alias.accept(self)
      self.__type_ref = ''.join(type.premod) + ' ' + self.__type_ref + ' ' + ''.join(type.postmod)
      self.__type_label = ''.join(type.premod) + ' ' + self.__type_label + ' ' + ''.join(type.postmod)
            
   def visit_parametrized(self, type):

      if type.template:
         type.template.accept(self)
         type_label = self.__type_label + '<'
      else: type_label = '(unknown)<'
      parameters_label = []
      for p in type.parameters:
         p.accept(self)
         parameters_label.append(self.__type_label)
      self.__type_label = type_label + ', '.join(parameters_label) + '>'

   def visit_function_type(self, type):

      # TODO: this needs to be implemented
      self.__type_ref = 'function_type'
      self.__type_label = 'function_type'

   #################### ASG Visitor ############################################
      
   def visit_declarator(self, node):

      self.__declarator = node.name
      for i in node.sizes:
         self.__declarator[-1] = self.__declarator[-1] + '[%d]'%i

   def visit_typedef(self, typedef):

      #self.write('@node ' + self.decl_label(typedef.name) + '\n')
      self.write('@deftp ' + typedef.type + ' {' + self.format_type(typedef.alias) + '} {' + self.decl_label(typedef.name) + '}\n')
      self.format_comments(typedef)
      self.write('@end deftp\n')

   def visit_variable(self, variable):

      #self.write('@node ' + self.decl_label(variable.name) + '\n')
      self.write('@deftypevr {' + variable.type + '} {' + self.format_type(variable.vtype) + '} {' + self.decl_label(variable.name) + '}\n')
      #FIXME !: how can values be represented in texinfo ?
      self.format_comments(variable)
      self.write('@end deftypevr\n')

   def visit_const(self, const):

      print "sorry, <const> not implemented"

   def visit_module(self, module):

      #self.write('@node ' + self.decl_label(module.name) + '\n')
      self.write('@deftp ' + module.type + ' ' + self.decl_label(module.name) + '\n')
      self.format_comments(module)
      old_scope = self.scope
      self.scope = module.name
      #menu = MenuMaker(str(self.scope), self.os)
      #menu.start()
      #for declaration in module.declarations: declaration.accept(menu)
      #menu.end()
      for declaration in module.declarations: declaration.accept(self)
      self.scope = old_scope
      self.write('@end deftp\n')

   def visit_class(self, class_):

      #self.write('@node ' + self.decl_label(clas.name) + '\n')
      self.write('@deftp ' + class_.type + ' ' + self.decl_label(class_.name) + '\n')
      if len(class_.parents):
         self.write('parents:')
         first = 1
         for parent in class_.parents:
            if not first: self.write(', ')
            else: self.write(' ')
            parent.accept(self)
         self.write('\n')
      self.format_comments(class_)
      old_scope = self.scope
      self.scope = class_.name
      #menu = MenuMaker(str(self.scope), self.os)
      #menu.start()
      #for d in class_.declarations: d.accept(menu)
      #menu.end()
      for d in class_.declarations:
         d.accept(self)
      self.scope = old_scope
      self.write('@end deftp\n')

   def visit_inheritance(self, inheritance):

      self.write('parent class')

   def visit_parameter(self, parameter):

      parameter.type.accept(self)
      label = self.write('{' + self.type_label() + '}')
      label = self.write(' @var{' + parameter.name + '}')

   def visit_function(self, function):

      ret = function.return_type
      if ret:
         ret.accept(self)
         ret_label = '{' + self.type_label() + '}'
      else:
         ret_label = '{}'
      self.write('@deftypefn ' + function.type + ' ' + ret_label + ' ' + self.decl_label(function.real_name) + ' (')
      first = 1
      for parameter in function.parameters:
         if not first: self.write(', ')
         else: self.write(' ')
         parameter.accept(self)
         first = 0
      self.write(')\n')
      #    map(lambda e, this=self: this.entity("exceptionname", e), operation.exceptions)
      self.format_comments(function)
      self.write('@end deftypefn\n')

   def visit_operation(self, operation):

      ret = operation.return_type
      if ret:
         ret.accept(self)
         ret_label = '{' + self.type_label() + '}'
      else:
         ret_label = '{}'
      try:
         #self.write('@node ' + self.decl_label(operation.name) + '\n')
         self.write('@deftypeop ' + operation.type + ' ' + self.decl_label(self.scope) + ' ' + ret_label + ' ' + self.decl_label(operation.real_name) + ' (')
      except:
         print operation.real_name
         sys.exit(0)
      first = 1
      for parameter in operation.parameters:
         if not first: self.write(', ')
         else: self.write(' ')
         parameter.accept(self)
         first = 0
      self.write(')\n')
      #    map(lambda e, this=self: this.entity("exceptionname", e), operation.exceptions)
      self.format_comments(operation)
      self.write('@end deftypeop\n')

   def visit_enumerator(self, enumerator):

      self.write('@deftypevr {' + enumerator.type + '} {} {' + self.decl_label(enumerator.name) + '}')
      #FIXME !: how can values be represented in texinfo ?
      if enumerator.value: self.write('\n')
      else: self.write('\n')
      self.format_comments(enumerator)
      self.write('@end deftypevr\n')

   def visit_enum(self, enum):
      #self.write('@node ' + self.decl_label(enum.name) + '\n')
      self.write('@deftp ' + enum.type + ' ' + self.decl_label(enum.name) + '\n')
      self.format_comments(enum)
      for enumerator in enum.enumerators:
         enumerator.accept(self)
      self.write('@end deftp\n')

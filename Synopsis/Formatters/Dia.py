# $Id: Dia.py,v 1.14 2003/11/13 20:40:09 stefan Exp $
#
# Copyright (C) 2000 Stefan Seefeld
# Copyright (C) 2000 Stephen Davies
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

"""Generates a .dia file of unpositioned classes and generalizations."""

from Synopsis.Processor import Processor, Parameter
from Synopsis import Type, AST, Util

import sys, getopt, os, os.path, string, re

def k2a(keys):
   "Convert a keys dict to a string of attributes"
   return string.join(map(lambda item:' %s="%s"'%item, keys.items()), '')

def quote(str):
   "Remove HTML chars from str"
   str = re.sub('&', '&amp;', str)
   str = re.sub('<','&lt;', str)
   str = re.sub('>','&gt;', str)
   #str = re.sub('<','lt', str)
   #str = re.sub('>','gt', str)
   str = re.sub("'", '&#39;', str)
   str = re.sub('"', '&quot;', str)
   return str

class Formatter(Processor, AST.Visitor, Type.Visitor):
   """Outputs a Dia file"""

   hide_operations = Parameter(False, 'hide operations')
   hide_attributes = Parameter(False, 'hide attributes')
   hide_parameters = Parameter(False, 'hide parameters')

   def process(self, ast, **kwds):

      self.set_parameters(kwds)
      self.ast = self.merge_input(ast)
      self.__indent = 0
      self.__istring = "  "
      self.__os = None
      self.__oidcount = 0
      self.__inherits = [] # list of from,to tuples
      self.__objects = {} #maps tuple names to object ids

      self.__os = open(self.output, "w")

      self.write('<?xml version="1.0"?>\n')
      self.start_tag_dict('diagram', {'xmlns:dia':"http://www.lysator.liu.se/~alla/dia/"})

      self.doDiagramData()
      self.start_tag('layer', name='Background', visible='true')
      for decl in self.ast.declarations():
         decl.accept(self)
      for inheritance in self.__inherits:
         self.do_inheritance(inheritance)
      self.end_tag('layer')
      self.end_tag('diagram')

      self.__os.close()

      return self.ast

   def indent(self): self.__os.write(self.__istring * self.__indent)
   def incr(self): self.__indent = self.__indent + 1
   def decr(self): self.__indent = self.__indent - 1
   def write(self, text): self.__os.write(text)
   def scope(self): return ()

   def start_tag(self, tagname, **keys):
      "Starts a tag and indents, attributes using keyword arguments"

      self.indent()
      self.write("<%s%s>\n"%(tagname,k2a(keys)))
      self.incr()

   def start_tag_dict(self, tagname, attrs):
      "Starts a tag and indents, attributes using dictionary argument"

      self.indent()
      self.write("<%s%s>\n"%(tagname,k2a(attrs)))
      self.incr()

   def end_tag(self, tagname):
      "Un-indents and closes tag"

      self.decr()
      self.indent()
      self.write("</%s>\n"%tagname)

   def solo_tag(self, tagname, **keys):
      "Writes a solo tag with attributes from keyword arguments"

      self.indent()
      self.write("<%s%s/>\n"%(tagname,k2a(keys)))

   def attribute(self, name, type, value, allow_solo=1):
      "Writes an attribute with given name, type and value"

      self.start_tag('attribute', name=name)
      if not value and allow_solo:
         self.solo_tag(type)
      elif type == 'string':
         self.indent()
         self.write('<string>#%s#</string>\n'%value)
      else:
         self.solo_tag(type, val=value)
      self.end_tag('attribute')

   def doDiagramData(self):
      "Write the stock diagramdata stuff"

      self.start_tag('diagramdata')
      self.attribute('background', 'color', '#ffffff')
      self.start_tag('attribute', name='paper')
      self.start_tag('composite', type='paper')
      self.attribute('name','string','A4')
      self.attribute('tmargin','real','2.82')
      self.attribute('bmargin','real','2.82')
      self.attribute('lmargin','real','2.82')
      self.attribute('rmargin','real','2.82')
      self.attribute('is_portrait','boolean','true')
      self.attribute('scaling','real','1')
      self.attribute('fitto','boolean','false')
      self.end_tag('composite')
      self.end_tag('attribute')
      self.start_tag('attribute', name='grid')
      self.start_tag('composite', type='grid')
      self.attribute('width_x', 'real', '1')
      self.attribute('width_y', 'real', '1')
      self.attribute('visible_x', 'int', '1')
      self.attribute('visible_y', 'int', '1')
      self.end_tag('composite')
      self.end_tag('attribute')
      self.start_tag('attribute', name='guides')
      self.start_tag('composite', type='guides')
      self.solo_tag('attribute', name='hguides')
      self.solo_tag('attribute', name='vguides')
      self.end_tag('composite')
      self.end_tag('attribute')
      self.end_tag('diagramdata')

   def do_inheritance(self, inherit):
      "Create a generalization object for one inheritance"

      from_id, to_id = map(self.get_object_id, inherit)
      id = self.create_object_id(None)
      self.start_tag('object', type='UML - Generalization', version='0', id=id)
      self.attribute('obj_pos', 'point', '1,0')
      self.attribute('obj_bb', 'rectangle', '0,0;2,2')
      self.start_tag('attribute', name='orth_points')
      self.solo_tag('point', val='1,0')
      self.solo_tag('point', val='1,1')
      self.solo_tag('point', val='0,1')
      self.solo_tag('point', val='0,2')
      self.end_tag('attribute')
      self.start_tag('attribute', name='orth_orient')
      self.solo_tag('enum', val='1')
      self.solo_tag('enum', val='0')
      self.solo_tag('enum', val='1')
      self.end_tag('attribute')
      self.attribute('name', 'string', None)
      self.attribute('stereotype', 'string', None)
      self.start_tag('connections')
      self.solo_tag('connection', handle='0', to=from_id, connection='6')
      self.solo_tag('connection', handle='1', to=to_id, connection='1')
      self.end_tag('connections')
      self.end_tag('object')

   def create_object_id(self, decl):
      "Creates a new object identifier, and remembers it with the given declaration"

      idstr = 'O'+str(self.__oidcount)
      if decl: self.__objects[decl.name()] = idstr
      self.__oidcount = self.__oidcount+1
      return idstr

   def get_object_id(self, decl):
      "Returns the stored identifier for the given object"

      print decl, type(decl)
      try:
         return self.__objects[decl.name()]
      except KeyError:
         print "Warning: no ID for",decl.name()
         return 0
    
   def formatType(self, type):
      "Returns a string representation for the given type"

      if type is None: return '(unknown)'
      type.accept(self)
      return self.__type
    
   #################### Type Visitor ##########################################

   def visitBaseType(self, type):

      self.__type = Util.ccolonName(type.name())

   def visitUnknown(self, type):

      self.__type = Util.ccolonName(type.name(), self.scope())

   def visitDeclared(self, type):

      self.__type = Util.ccolonName(type.name(), self.scope())
        
   def visitModifier(self, type):

      aliasStr = self.formatType(type.alias())
      premod = map(lambda x:x+" ", type.premod())
      self.__type = "%s%s%s"%(string.join(premod,''), aliasStr, string.join(type.postmod(),''))

   def visitParametrized(self, type):

      temp = self.formatType(type.template())
      params = map(self.formatType, type.parameters())
      self.__type = "%s<%s>"%(temp,string.join(params, ", "))

   def visitFunctionType(self, type):

      ret = self.formatType(type.returnType())
      params = map(self.formatType, type.parameters())
      self.__type = "%s(%s)(%s)"%(ret,string.join(type.premod(),''),string.join(params,", "))

    ################# AST visitor #################

   def visitDeclaration(self, decl):
      "Default is to not do anything with it"

      #print "Not writing",decl.type(), decl.name()
      pass

   def visitModule(self, decl):
      "Just traverse child declarations"

      # TODO: make a Package UML object and maybe link classes to it?
      for d in decl.declarations():
         d.accept(self)

   def visitClass(self, decl):
      "Creates a Class object for one class, with attributes and operations"

      id = self.create_object_id(decl)
      self.start_tag('object', type='UML - Class', version='0', id=id)
      self.attribute('objpos', 'point', '1,1')
      self.attribute('obj_bb', 'rectangle', '1,1;2,2')
      self.attribute('elem_corner', 'point', '1.05,1.05')
      self.attribute('elem_width', 'real', '1')
      self.attribute('elem_height', 'real', '5')
      self.attribute('name', 'string', Util.ccolonName(decl.name()))
      self.attribute('stereotype', 'string', None)
      self.attribute('abstract', 'boolean', 'false')
      self.attribute('suppress_attributes', 'boolean', 'false')
      self.attribute('suppress_operations', 'boolean', 'false')
      self.attribute('visible_attributes', 'boolean', self.hide_attributes and 'false' or 'true')
      self.attribute('visible_operations', 'boolean', self.hide_operations and 'false' or 'true')
      # Do attributes
      afilt = lambda d: d.type() == 'attribute' or d.type() == 'variable'
      attributes = filter(afilt, decl.declarations())
      if not len(attributes) or self.hide_attributes:
         self.solo_tag('attribute', name='attributes')
      else:
         self.start_tag('attribute', name='attributes')
         for attr in attributes:
            self.start_tag('composite', type='umlattribute')
            self.attribute('name', 'string', attr.name()[-1])
            if attr.type() == 'attribute':
               self.attribute('type', 'string', self.formatType(attr.returnType()))
            else:
               self.attribute('type', 'string', self.formatType(attr.vtype()))
            self.attribute('value', 'string', None)
            self.attribute('visibility', 'enum', '0')
            self.attribute('abstract', 'boolean', 'false')
            self.attribute('class_scope', 'boolean', 'false')
            self.end_tag('composite')
         self.end_tag('attribute')
      # Do operations
      operations = filter(lambda d: d.type() == 'operation', decl.declarations())
      if not len(operations) or self.hide_operations:
         self.solo_tag('attribute', name='operations')
      else:
         self.start_tag('attribute', name='operations')
         for oper in operations:
            self.start_tag('composite', type='umloperation')
            self.attribute('name', 'string', oper.name()[-1])
            self.attribute('type', 'string', self.formatType(oper.returnType()))
            self.attribute('visibility', 'enum', '0')
            self.attribute('abstract', 'boolean', 'false')
            self.attribute('class_scope', 'boolean', 'false')
            if len(oper.parameters()) and not self.hide_parameters:
               self.start_tag('attribute', name='parameters')
               for param in oper.parameters():
                  self.start_tag('composite', name='umlparameter')
                  self.attribute('name', 'string', param.identifier())
                  self.attribute('type', 'string', '', 0)
                  self.attribute('value', 'string', quote(param.value()))
                  self.attribute('kind', 'enum', '0')
                  self.end_tag('composite')
               self.end_tag('attribute')
            else:
               self.solo_tag('attribute', name='parameters')
            self.end_tag('composite')
         self.end_tag('attribute')
      # Finish class object
      self.attribute('template', 'boolean', 'false')
      self.solo_tag('attribute', name='templates')
      self.end_tag('object')

      for inherit in decl.parents():
         self.__inherits.append( (inherit.parent(), decl) )



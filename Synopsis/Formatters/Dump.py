# $Id: Dump.py,v 1.9 2003/12/28 21:46:37 stefan Exp $
#
# Copyright (C) 2003 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

"""
Verbose attribute-oriented xml dump of AST, useful for validation,
introspection, and debugging.
"""

from Synopsis import config
from Synopsis.Processor import Processor, Parameter
from Synopsis import Type, AST

import sys, getopt, os, os.path, string, types
from xml.dom.minidom import getDOMImplementation

dom = getDOMImplementation().createDocument(None, "dump", None)

class Formatter(Processor):

   show_ids = Parameter(True, 'output object ids as attributes')
   show_declarations = Parameter(True, 'output declarations')
   show_types = Parameter(True, 'output types')
   show_files = Parameter(True, 'output files')
   stylesheet = Parameter(config.datadir + '/dump.css', 'stylesheet to be referenced for rendering')

   def process(self, ast, **kwds):
      
      self.set_parameters(kwds)
      self.ast = self.merge_input(ast)

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


      self.os = open(self.output, "w")
      self.os.write("<?xml version='1.0' encoding='ISO-8859-1'?>\n")
      if self.stylesheet:
         self.os.write("<?xml-stylesheet href='%s' type='text/css'?>\n"%self.stylesheet)

      self.os.write("<ast>\n")

      if self.show_declarations:
         self.write_declarations(self.ast.declarations())
         
      if self.show_types:
         self.write_types(self.ast.types())

      if self.show_files:
         self.write_files(self.ast.files())

      self.os.write("</ast>\n")

      return self.ast

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
         if self.show_ids:
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
         #self.push('item')
         self.visit(i)
         #self.pop()

   def visit_list(self, obj):

      if len(obj) == 0: return
      for i in obj:
         #self.push('item')
         self.visit(i)
         #self.pop()

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
      self.push("instance")
      self.node.setAttribute('class', "%s.%s"%(obj.__class__.__module__,obj.__class__.__name__))
      if self.show_ids:
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
      self.node.writexml(self.os, indent=" ", addindent=" ", newl="\n")
      self.node.unlink()
      del self.node

   def write_types(self, types):
      self.node = dom.createElement("types")
      values = types.values()
      values.sort()
      for t in values: self.visit(t)
      self.node.writexml(self.os, indent=" ", addindent=" ", newl="\n")
      self.node.unlink()
      del self.node

   def write_files(self, files):

      self.node = dom.createElement("files")
      for f in files:
         self.push("file")
         self.visit(f)
         self.pop()

      self.node.writexml(self.os, indent=" ", addindent=" ", newl="\n")
      self.node.unlink()
      del self.node


# $Id: Part.py,v 1.39 2003/12/05 22:30:29 stefan Exp $
#
# Copyright (C) 2000 Stephen Davies
# Copyright (C) 2000 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

"""AST Formatting classes.

This module contains classes for formatting parts of a scope page (class,
module, etc with methods, variables etc. The actual formatting of the
declarations is delegated to multiple strategies for each part of the page,
and are defined in the FormatStrategy module.
"""

from Synopsis.Processor import Parametrized, Parameter
from Synopsis import AST, Type, Util
from Fragment import Fragment
import Tags # need both because otherwise 'Tags.name' would be ambiguous
from Tags import *

import string, os

class Part(Parametrized, Type.Visitor, AST.Visitor):
   """Base class for formatting a Part of a Scope Page.
    
   This class contains functionality for modularly formatting an AST node and
   its children for display. It is typically used to contruct Heading,
   Summary and Detail formatters. Strategy objects are added according to
   configuration, and this base class  then checks which format methods each
   strategy implements. For each AST declaration visited, the Part asks all
   Strategies which implement the appropriate format method to generate
   output for that declaration. The final writing of the formatted html is
   delegated to the write_section_start, write_section_end, and write_section_item
   methods, which myst be implemented in a subclass.
   """

   fragments = Parameter([], "list of Fragments")

   def register(self, page):

      self.processor = page.processor
      self.__page = page
      self.__fragments = []
      self.__id_holder = None
      # Lists of format methods for each AST type
      self.__formatdict = {'format_declaration':[],
                           'format_forward':[],
                           'format_group':[],
                           'format_scope':[],
                           'format_module':[],
                           'format_meta_module':[],
                           'format_class':[],
                           'format_typedef':[],
                           'format_enum':[],
                           'format_variable':[],
                           'format_const':[],
                           'format_function':[],
                           'format_operation':[]}

      # Why not just apply all formatters ? is this an optimization ?
      # ask chalky...
      for fragment in self.fragments:
         fragment.register(self)
         for method in self.__formatdict.keys():
            no_func = getattr(Fragment, method).im_func
            method_obj = getattr(fragment, method)
            # If it was overridden in fragment
            if method_obj.im_func is not no_func:
               # Add to the dictionary
               self.__formatdict[method].append(method_obj)
    
   def page(self): return self.__page
   def filename(self): return self.__page.filename()
   def os(self): return self.__page.os()
   def scope(self): return self.__page.scope()
   def write(self, text): self.os().write(text)

   # Access to generated values
   def type_ref(self): return self.__type_ref
   def type_label(self): return self.__type_label
   def declarator(self): return self.__declarator
   def parameter(self): return self.__parameter
    
   def reference(self, name, label=None, **keys):
      """Returns a reference to the given name. The name is a scoped name,
      and the optional label is an alternative name to use as the link text.
      The name is looked up in the TOC so the link may not be local. The
      optional keys are appended as attributes to the A tag."""

      if not label: label = anglebrackets(Util.ccolonName(name, self.scope()))
      entry = self.processor.toc[name]
      if entry: return apply(href, (rel(self.filename(), entry.link), label), keys)
      return label or ''

   def label(self, name, label=None):
      """Create a label for the given name. The label is an anchor so it can
      be referenced by other links. The name of the label is derived by
      looking up the name in the TOC and using the link in the TOC entry.
      The optional label is an alternative name to use as the displayed
      name. If the name is not found in the TOC then the name is not
      anchored and just label is returned (or name if no label is given).
      """

      if label is None: label = name
      # some labels are templates with <>'s
      entry = self.processor.toc[name]
      label = anglebrackets(Util.ccolonName(label, self.scope()))
      if entry is None: return label
      location = entry.link
      index = string.find(location, '#')
      if index >= 0: location = location[index+1:]
      return location and Tags.name(location, label) or label


   def format_declaration(self, decl, method):
      """Format decl using named method of each fragment. Each fragment
      returns two strings - type and name. All the types are joined and all
      the names are joined separately. The consolidated type and name
      strings are then passed to write_section_item."""

      # TODO - investigate quickest way of doing this. I tried.
      # A Lambda that calls the given function with decl
      format = lambda func, decl=decl: func(decl)
      # Get a list of 2tuples [('type','name'),('type','name'),...]
      type_name = map(format, self.__formatdict[method])
      if not len(type_name): return
      # NEW CODE:
      text = string.strip(string.join(type_name))
      self.write_section_item(text)

   def process(self, decl):
      """Formats the given decl, creating the output for this Part of the
      page. This method is implemented in various subclasses in different
      ways, for example Summary and Detail iterate through the children of
      'decl' section by section, whereas Heading only formats decl itself.
      """

      pass
	
   #################### AST Visitor ############################################
   def visitDeclaration(self, decl): self.format_declaration(decl, 'format_declaration')
   def visitForward(self, decl): self.format_declaration(decl, 'format_forward')
   def visitGroup(self, decl): self.format_declaration(decl, 'format_group')
   def visitScope(self, decl): self.format_declaration(decl, 'format_scope')
   def visitModule(self, decl):	self.format_declaration(decl, 'format_module')
   def visitMetaModule(self, decl): self.format_declaration(decl, 'format_meta_module')
   def visitClass(self, decl): self.format_declaration(decl, 'format_class')
   def visitTypedef(self, decl): self.format_declaration(decl, 'format_typedef')
   def visitEnum(self, decl): self.format_declaration(decl, 'format_enum')
   def visitVariable(self, decl): self.format_declaration(decl, 'format_variable')
   def visitConst(self, decl): self.format_declaration(decl, 'format_const')
   def visitFunction(self, decl): self.format_declaration(decl, 'format_function')
   def visitOperation(self, decl): self.format_declaration(decl, 'format_operation')


   #################### Type Formatter/Visitor #################################
   def format_type(self, typeObj, id_holder = None):
      "Returns a reference string for the given type object"

      if typeObj is None: return "(unknown)"
      if id_holder:
         save_id = self.__id_holder
         self.__id_holder = id_holder
      typeObj.accept(self)
      if id_holder:
         self.__id_holder = save_id
      return self.__type_label

   def visitBaseType(self, type):
      "Sets the label to be a reference to the type's name"

      self.__type_label = self.reference(type.name())
        
   def visitUnknown(self, type):
      "Sets the label to be a reference to the type's link"

      self.__type_label = self.reference(type.link())
        
   def visitDeclared(self, type):
      "Sets the label to be a reference to the type's name"

      self.__type_label = self.reference(type.name())

   def visitDependent(self, type):
      "Sets the label to be the type's name (which has no proper scope)"

      self.__type_label = type.name()[-1]
        
   def visitModifier(self, type):
      "Adds modifiers to the formatted label of the modifier's alias"

      alias = self.format_type(type.alias())
      def amp(x):
         if x == '&': return '&amp;'
         return x
      pre = string.join(map(lambda x:x+"&nbsp;", map(amp, type.premod())), '')
      post = string.join(map(amp, type.postmod()), '')
      self.__type_label = "%s%s%s"%(pre,alias,post)
            
   def visitParametrized(self, type):
      "Adds the parameters to the template name in angle brackets"

      if type.template():
         type_label = self.reference(type.template().name())
      else:
         type_label = "(unknown)"
      params = map(self.format_type, type.parameters())
      self.__type_label = "%s&lt;%s&gt;"%(type_label,string.join(params, ", "))

   def visitTemplate(self, type):
      "Labs the template with the parameters"

      self.__type_label = "template&lt;%s&gt;"%(
         string.join(map(lambda x:"typename "+x,
                         map(self.format_type, type.parameters())), ",")
         )

   def visitFunctionType(self, type):
      "Labels the function type with return type, name and parameters"
      
      ret = self.format_type(type.returnType())
      params = map(self.format_type, type.parameters())
      pre = string.join(type.premod(), '')
      if self.__id_holder:
         ident = self.__id_holder[0]
         del self.__id_holder[0]
      else:
         ident = ''
      self.__type_label = "%s(%s%s)(%s)"%(ret,pre,ident,string.join(params,", "))



   # These are overridden in {Summary,Detail}Formatter
   def write_start(self):
      "Abstract method to start the output, eg table headings"

      pass

   def write_section_start(self, heading):
      "Abstract method to start a section of declaration types"

      pass

   def write_section_end(self, heading):
      "Abstract method to end a section of declaration types"

      pass

   def write_section_item(self, text):
      "Abstract method to write the output of one formatted declaration"

      pass

   def write_end(self):
      "Abstract method to end the output, eg close the table"

      pass

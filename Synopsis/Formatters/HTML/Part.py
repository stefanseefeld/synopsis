#
# Copyright (C) 2000 Stephen Davies
# Copyright (C) 2000 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

"""ASG Formatting classes.

This module contains classes for formatting parts of a scope view (class,
module, etc with methods, variables etc. The actual formatting of the
declarations is delegated to multiple strategies for each part of the view,
and are defined in the FormatStrategy module.
"""

from Synopsis.Processor import Parametrized, Parameter
from Synopsis import ASG, Type, Util
from Fragment import Fragment
import Tags # need both because otherwise 'Tags.name' would be ambiguous
from Tags import *

class Part(Parametrized, Type.Visitor, ASG.Visitor):
   """Base class for formatting a Part of a Scope View.
    
   This class contains functionality for modularly formatting an ASG node and
   its children for display. It is typically used to construct Heading,
   Summary and Detail formatters. Strategy objects are added according to
   configuration, and this base class  then checks which format methods each
   strategy implements. For each ASG declaration visited, the Part asks all
   Strategies which implement the appropriate format method to generate
   output for that declaration. The final writing of the formatted html is
   delegated to the write_section_start, write_section_end, and write_section_item
   methods, which must be implemented in a subclass.
   """

   fragments = Parameter([], "list of Fragments")

   def register(self, view):

      self.processor = view.processor
      self.__view = view
      self.__fragments = []
      self.__id_holder = None
      # Lists of format methods for each ASG type
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

      for fragment in self.fragments:
         fragment.register(self)
         for method in self.__formatdict.keys():
            no_func = getattr(Fragment, method).im_func
            method_obj = getattr(fragment, method)
            # If it was overridden in fragment
            if method_obj.im_func is not no_func:
               # Add to the dictionary
               self.__formatdict[method].append(method_obj)
    
   def view(self): return self.__view
   def filename(self): return self.__view.filename()
   def os(self): return self.__view.os()
   def scope(self): return self.__view.scope()
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

      if not label: label = escape(Util.ccolonName(name, self.scope()))
      entry = self.processor.toc[name]
      if entry: return href(rel(self.filename(), entry.link), escape(label), **keys)
      else: return label or ''

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
      label = escape(Util.ccolonName(label, self.scope()))
      if entry is None: return label
      location = entry.link
      index = location.find('#')
      if index >= 0: location = location[index+1:]
      return location and Tags.name(location, label) or label


   def format_declaration(self, decl, method):
      """Format decl using named method of each fragment. Each fragment
      returns two strings - type and name. All the types are joined and all
      the names are joined separately. The consolidated type and name
      strings are then passed to write_section_item."""

      type_name = [f(decl) for f in self.__formatdict[method]]
      if type_name:
         text = ' '.join(type_name).strip()
         self.write_section_item(text)

   def process(self, decl):
      """Formats the given decl, creating the output for this Part of the
      view. This method is implemented in various subclasses in different
      ways, for example Summary and Detail iterate through the children of
      'decl' section by section, whereas Heading only formats decl itself.
      """

      pass
	
   #################### ASG Visitor ############################################
   def visit_declaration(self, decl): self.format_declaration(decl, 'format_declaration')
   def visit_forward(self, decl): self.format_declaration(decl, 'format_forward')
   def visit_group(self, decl): self.format_declaration(decl, 'format_group')
   def visit_scope(self, decl): self.format_declaration(decl, 'format_scope')
   def visit_module(self, decl):	self.format_declaration(decl, 'format_module')
   def visit_meta_module(self, decl): self.format_declaration(decl, 'format_meta_module')
   def visit_class(self, decl): self.format_declaration(decl, 'format_class')
   def visit_typedef(self, decl): self.format_declaration(decl, 'format_typedef')
   def visit_enum(self, decl): self.format_declaration(decl, 'format_enum')
   def visit_variable(self, decl): self.format_declaration(decl, 'format_variable')
   def visit_const(self, decl): self.format_declaration(decl, 'format_const')
   def visit_function(self, decl): self.format_declaration(decl, 'format_function')
   def visit_operation(self, decl): self.format_declaration(decl, 'format_operation')


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

   def visit_base_type(self, type):
      "Sets the label to be a reference to the type's name"

      self.__type_label = self.reference(type.name())
        
   def visit_unknown(self, type):
      "Sets the label to be a reference to the type's link"

      self.__type_label = self.reference(type.link())
        
   def visit_declared(self, type):
      "Sets the label to be a reference to the type's name"

      self.__type_label = self.reference(type.name())

   def visit_dependent(self, type):
      "Sets the label to be the type's name (which has no proper scope)"

      self.__type_label = type.name()[-1]
        
   def visit_modifier(self, type):
      "Adds modifiers to the formatted label of the modifier's alias"

      alias = self.format_type(type.alias())
      def amp(x):
         if x == '&': return '&amp;'
         return x
      pre = ''.join(['%s&#160;'%amp(x) for x in type.premod()])
      post = ''.join([amp(x) for x in type.postmod()])
      self.__type_label = "%s%s%s"%(pre,alias,post)
            
   def visit_parametrized(self, type):
      "Adds the parameters to the template name in angle brackets"

      if type.template():
         type_label = self.reference(type.template().name())
      else:
         type_label = "(unknown)"
      params = map(self.format_type, type.parameters())
      self.__type_label = "%s&lt;%s&gt;"%(type_label,', '.join(params))

   def visit_template(self, type):
      "Labs the template with the parameters"

      self.__type_label = "template&lt;%s&gt;"%(
         ','.join(['typename %s'%self.format_type(p) for p in type.parameters()])
         )

   def visit_function_type(self, type):
      "Labels the function type with return type, name and parameters"
      
      ret = self.format_type(type.returnType())
      params = map(self.format_type, type.parameters())
      pre = ''.join(type.premod())
      if self.__id_holder:
         ident = self.__id_holder[0]
         del self.__id_holder[0]
      else:
         ident = ''
      self.__type_label = "%s(%s%s)(%s)"%(ret,pre,ident,', '.join(params))



   # These are overridden in {Summary,Detail}Formatter
   def write_start(self):
      "Abstract method to start the output, eg table headings"

      self.write('<!-- this part was generated by ' + self.__class__.__name__ + ' -->\n')

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

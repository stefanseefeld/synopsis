#
# Copyright (C) 2000 Stephen Davies
# Copyright (C) 2000 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.Formatters.HTML.Tags import *
from Synopsis.Formatters.HTML.Fragment import Fragment

class ClassHierarchySimple(Fragment):
   "Prints a simple text hierarchy for classes"

   def format_inheritance(self, inheritance):

      return '%s %s'%(self.format_modifiers(inheritance.attributes()),
                      self.format_type(inheritance.parent()))

   def format_class(self, clas):

      # Print out a list of the parents
      super = sub = ''
      if clas.parents():
         parents = map(self.format_inheritance, clas.parents())
         super = string.join(parents, ", ")
         super = div('superclasses', "Superclasses: "+super)

      # Print subclasses
      subs = self.processor.class_tree.subclasses(clas.name())
      if subs:
         refs = map(self.reference, subs)
         sub = string.join(refs, ", ")
         sub = div('subclasses', "Known subclasses: "+sub) 
	
      return super + sub

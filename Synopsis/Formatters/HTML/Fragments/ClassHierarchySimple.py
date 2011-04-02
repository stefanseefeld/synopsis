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

      return '%s %s'%(self.format_modifiers(inheritance.attributes),
                      self.format_type(inheritance.parent))

   def format_class(self, class_):

      # Print out a list of the parents
      super = sub = ''
      if class_.parents:
         parents = [self.format_inheritance(i) for i in class_.parents]
         super = ', '.join(parents)
         super = div('Superclasses: '+super, class_='superclasses')

      # Print subclasses
      subs = self.processor.class_tree.subclasses(class_.name)
      if subs:
         sub = ', '.join([self.reference(s) for s in subs])
         sub = div('Known subclasses: '+sub, class_='subclasses') 
	
      return super + sub

   format_class_template = format_class

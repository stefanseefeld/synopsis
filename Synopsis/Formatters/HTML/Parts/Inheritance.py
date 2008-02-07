#
# Copyright (C) 2000 Stephen Davies
# Copyright (C) 2000 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.Processor import Parameter
from Synopsis import ASG
from Synopsis.Formatters.HTML.Part import Part
from Synopsis.Formatters.HTML.Fragments import *
from Synopsis.Formatters.HTML.Tags import *

class Inheritance(Part):

   fragments = Parameter([InheritanceFormatter()],
                         '')

   def register(self, view):

      Part.register(self, view)
      self.__start_list = 0

   def process(self, decl):
      "Walk the hierarchy to find inherited members to print."

      if not isinstance(decl, ASG.Class): return
      self.write_start()
      names = [self._short_name(n) for n in decl.declarations]
      self._process_superclasses(decl, names)
      self.write_end()

   def _process_class(self, clas, names):
      "Prints info for the given class, and calls _process_superclasses after"

      sorter = self.processor.sorter.clone(clas.declarations)
      child_names = []

      # Iterate through the sections
      for section in sorter:
         # Write a heading
         heading = section+' Inherited from '+ str(self.scope().prune(clas.name))
         started = 0 # Lazy section start incase no details for this section
         # Iterate through the children in this section
         for child in sorter[section]:
            child_name = self._short_name(child)
            if child_name in names:
               continue
            # FIXME: This doesn't account for the inheritance type
            # (private etc)
            if child.accessibility == ASG.PRIVATE:
               continue
            # Don't include constructors and destructors!
            if (isinstance(child, ASG.Function) and
                child.file.annotations['language'] == 'C++' and
                len(child.real_name) > 1):
               if child.real_name[-1] == child.real_name[-2]: continue
               elif child.real_name[-1] == "~"+child.real_name[-2]: continue
            # FIXME: skip overriden declarations
            child_names.append(child_name)
            # Check section heading
            if not started:
               started = 1
               self.write_section_start(heading)
            child.accept(self)
         # Finish the section
         if started: self.write_section_end(heading)
	
      self._process_superclasses(clas, names + child_names)
    
   def _short_name(self, decl):
      if isinstance(decl, ASG.Function):
         return decl.real_name[-1]
      return decl.name[-1]
    
   def _process_superclasses(self, class_, names):
      """Iterates through the superclasses of clas and calls _process_clas for
      each"""

      for inheritance in class_.parents:
         parent = inheritance.parent
         if isinstance(parent, ASG.Declared):
            parent = parent.declaration
            if isinstance(parent, ASG.Class):
               self._process_class(parent, names)
               continue
         #print "Ignoring", parent.__class__.__name__, "parent of", clas.name
         pass #ignore
     
   def write_section_start(self, heading):
      """Creates a table with one row. The row has a td of class 'heading'
      containing the heading string"""

      self.write('<table width="100%%" summary="%s">\n'%heading)
      self.write('<tr><td colspan="2" class="heading">' + heading + '</td></tr>\n')
      self.write('<tr><td class="inherited">')
      self.__start_list = 1

   def write_section_item(self, text):
      """Adds a table row"""

      if self.__start_list:
         self.write(text)
         self.__start_list = 0
      else:
         self.write(',\n'+text)

   def write_section_end(self, heading):

      self.write('</td></tr></table>\n')



# $Id: Inheritance.py,v 1.1 2003/11/15 19:54:05 stefan Exp $
#
# Copyright (C) 2000 Stephen Davies
# Copyright (C) 2000 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.Formatters.HTML.Part import Part

from Synopsis.Formatters.HTML import FormatStrategy
from from Synopsis.Formatters.HTML.Tags import *

class Inheritance(Part):

   def register(self, page):

      Part.register(self, page)
      self._init_formatters('inheritance_formatters', 'inheritance')
      self.__start_list = 0

   def _init_default_formatters(self):

      self.addFormatter(FormatStrategy.Inheritance)

   def process(self, decl):
      "Walk the hierarchy to find inherited members to print."

      if not isinstance(decl, AST.Class): return
      self.write_start()
      names = decl.declarations()
      names = map(self._short_name, names)
      self._process_superclasses(decl, names)
      self.write_end()

   def _process_class(self, clas, names):
      "Prints info for the given class, and calls _process_superclasses after"

      sorter = self.processor.sorter
      sorter.set_scope(clas)
      sorter.sort_section_names()
      child_names = []

      # Iterate through the sections
      for section in sorter.sections():
         # Write a heading
         heading = section+'s Inherited from '+ Util.ccolonName(clas.name(), self.scope())
         started = 0 # Lazy section start incase no details for this section
         # Iterate through the children in this section
         for child in sorter.children(section):
            child_name = self._short_name(child)
            if child_name in names:
               continue
            # FIXME: This doesn't account for the inheritance type
            # (private etc)
            if child.accessibility() == AST.PRIVATE:
               continue
            # Don't include constructors and destructors!
            if isinstance(child, AST.Function) and child.language() == 'C++' and len(child.realname())>1:
               if child.realname()[-1] == child.realname()[-2]: continue
               elif child.realname()[-1] == "~"+child.realname()[-2]: continue
            # FIXME: skip overriden declarations
            child_names.append(child_name)
            # Check section heading
            if not started:
               started = 1
               self.writeSectionStart(heading)
            child.accept(self)
         # Finish the section
         if started: self.writeSectionEnd(heading)
	
      self._process_superclasses(clas, names + child_names)
    
   def _short_name(self, decl):
      if isinstance(decl, AST.Function):
         return decl.realname()[-1]
      return decl.name()[-1]
    
   def _process_superclasses(self, clas, names):
      """Iterates through the superclasses of clas and calls _process_clas for
      each"""

      for inheritance in clas.parents():
         parent = inheritance.parent()
         if isinstance(parent, Type.Declared):
            parent = parent.declaration()
            if isinstance(parent, AST.Class):
               self._process_class(parent, names)
               continue
         #print "Ignoring", parent.__class__.__name__, "parent of", clas.name()
         pass #ignore
     
   def writeSectionStart(self, heading):
      """Creates a table with one row. The row has a td of class 'heading'
      containing the heading string"""

      self.write('<table width="100%%" summary="%s">\n'%heading)
      self.write('<tr><td colspan="2" class="heading">' + heading + '</td></tr>\n')
      self.write('<tr><td class="inherited">')
      self.__start_list = 1

   def writeSectionItem(self, text):
      """Adds a table row"""

      if self.__start_list:
         self.write(text)
         self.__start_list = 0
      else:
         self.write(',\n'+text)

   def writeSectionEnd(self, heading):
      """Closes the table entity and adds a break."""
      self.write('</td></tr></table>\n<br>\n')



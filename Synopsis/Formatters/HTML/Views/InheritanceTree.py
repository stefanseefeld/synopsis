#
# Copyright (C) 2000 Stephen Davies
# Copyright (C) 2000 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.Processor import Parameter
from Synopsis import Util
from Synopsis.Formatters.HTML.View import View
from Synopsis.Formatters.HTML.Tags import *

import os

class InheritanceTree(View):

   def register(self, processor):

      View.register(self, processor)
      self.processor.add_root_view(self.filename(), 'Inheritance Tree', 'main', 1)
 
   def filename(self): return self.processor.file_layout.special('InheritanceTree')

   def title(self): return 'Synopsis - Class Hierarchy'

   def process(self, start):
      """Creates a file with the inheritance tree"""

      roots = self.processor.class_tree.roots()
      self.start_file()
      self.write(self.processor.navigation_bar(self.filename()))
      self.write(entity('h1', "Inheritance Tree"))
      self.write('<ul>')
      map(self.process_class_inheritance, map(lambda a,b=start.name():(a,b), roots))
      self.write('</ul>')
      self.end_file()   

   def process_class_inheritance(self, args):
      name, rel_name = args
      self.write('<li>')
      self.write(self.reference(name, rel_name))
      parents = self.processor.class_tree.superclasses(name)
      if parents:
         self.write(' <i>(%s)</i>'%string.join(map(Util.ccolonName, parents), ", "))
      subs = self.processor.class_tree.subclasses(name)
      if subs:
         self.write('<ul>')
         map(self.process_class_inheritance, map(lambda a,b=name:(a,b), subs))
         self.write('</ul>\n')
      self.write('</li>')

# $Id: InheritanceTree.py,v 1.12 2003/11/12 16:42:05 stefan Exp $
#
# Copyright (C) 2000 Stephen Davies
# Copyright (C) 2000 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.Processor import Parameter
from Synopsis import Util

import core
from Page import Page
from core import config
from Tags import *

import os

class InheritanceTree(Page):

   def register(self, manager):

      Page.register(self, manager)
      self.manager.addRootPage(self.filename(), 'Inheritance Tree', 'main', 1)
 
   def filename(self): return config.files.nameOfSpecial('InheritanceTree')

   def title(self): return 'Synopsis - Class Hierarchy'

   def process(self, start):
      """Creates a file with the inheritance tree"""

      roots = config.classTree.roots()
      self.start_file()
      self.write(self.manager.formatHeader(self.filename()))
      self.write(entity('h1', "Inheritance Tree"))
      self.write('<ul>')
      map(self.process_class_inheritance, map(lambda a,b=start.name():(a,b), roots))
      self.write('</ul>')
      self.end_file()   

   def process_class_inheritance(self, args):
      name, rel_name = args
      self.write('<li>')
      self.write(self.reference(name, rel_name))
      parents = config.classTree.superclasses(name)
      if parents:
         self.write(' <i>(%s)</i>'%string.join(map(Util.ccolonName, parents), ", "))
      subs = config.classTree.subclasses(name)
      if subs:
         self.write('<ul>')
         map(self.process_class_inheritance, map(lambda a,b=name:(a,b), subs))
         self.write('</ul>\n')
      self.write('</li>')

# $Id: Heading.py,v 1.6 2003/12/08 00:39:24 stefan Exp $
#
# Copyright (C) 2000 Stephen Davies
# Copyright (C) 2000 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.Processor import Parameter
from Synopsis.Formatters.HTML.Part import Part
from Synopsis.Formatters.HTML.Fragments import *
from Synopsis.Formatters.HTML.Tags import *

class Heading(Part):
   """Heading view part. Displays a header for the view -- its strategies are
   only passed the object that the view is for; ie a Class or Module"""

   fragments = Parameter([HeadingFormatter(),
                          ClassHierarchyGraph(),
                          DetailCommenter()],
                         '')

   def register(self, view):

      if view.processor.has_view('XRef'):
         self.fragments.append(XRefLinker())
      if view.processor.has_view('Source'):
         self.fragments.append(SourceLinker())

      Part.register(self, view)

   def write_section_item(self, text):
      """Writes text and follows with a horizontal rule"""

      self.write(text + '\n<hr>\n')

   def process(self, decl):
      """Process this Part by formatting only the given decl"""

      decl.accept(self)


# $Id: Heading.py,v 1.5 2003/12/05 22:30:29 stefan Exp $
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
   """Heading page part. Displays a header for the page -- its strategies are
   only passed the object that the page is for; ie a Class or Module"""

   fragments = Parameter([HeadingFormatter(),
                          ClassHierarchyGraph(),
                          DetailCommenter()],
                         '')

   def register(self, page):

      if page.processor.has_page('XRef'):
         self.fragments.append(XRefLinker())
      if page.processor.has_page('FileSource'):
         self.fragments.append(SourceLinker())

      Part.register(self, page)

   def write_section_item(self, text):
      """Writes text and follows with a horizontal rule"""

      self.write(text + '\n<hr>\n')

   def process(self, decl):
      """Process this Part by formatting only the given decl"""

      decl.accept(self)


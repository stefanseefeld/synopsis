# $Id: Heading.py,v 1.1 2003/11/15 19:54:05 stefan Exp $
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

class Heading(Part):
   """Heading page part. Displays a header for the page -- its strategies are
   only passed the object that the page is for; ie a Class or Module"""

   def register(self, page):

      Part.register(self, page)
      self._init_formatters('heading_formatters', 'heading')

   def _init_default_formatters(self):

      self.addFormatter(FormatStrategy.Heading)
      self.addFormatter(FormatStrategy.ClassHierarchyGraph)
      self.addFormatter(FormatStrategy.DetailCommenter)

   def writeSectionItem(self, text):
      """Writes text and follows with a horizontal rule"""

      self.write(text + '\n<hr>\n')

   def process(self, decl):
      """Process this Part by formatting only the given decl"""

      decl.accept(self)


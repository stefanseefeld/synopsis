# $Id: Heading.py,v 1.3 2003/11/16 21:09:45 stefan Exp $
#
# Copyright (C) 2000 Stephen Davies
# Copyright (C) 2000 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.Processor import Parameter
from Synopsis.Formatters.HTML.Part import Part
from Synopsis.Formatters.HTML import FormatStrategy
from Synopsis.Formatters.HTML.Tags import *

class Heading(Part):
   """Heading page part. Displays a header for the page -- its strategies are
   only passed the object that the page is for; ie a Class or Module"""

   formatters = Parameter([FormatStrategy.Heading(),
                           FormatStrategy.ClassHierarchyGraph(),
                           FormatStrategy.DetailCommenter()],
                          '')

   def write_section_item(self, text):
      """Writes text and follows with a horizontal rule"""

      self.write(text + '\n<hr>\n')

   def process(self, decl):
      """Process this Part by formatting only the given decl"""

      decl.accept(self)


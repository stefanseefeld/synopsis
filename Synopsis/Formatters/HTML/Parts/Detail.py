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

class Detail(Part):

   fragments = Parameter([DeclarationDetailFormatter(), DetailCommenter()],
                         '')

   def write_section_start(self, heading):
      """Creates a table with one row. The row has a td of class 'heading'
      containing the heading string"""

      self.write('<table width="100%%" summary="%s">\n'%heading)
      self.write('<tr><td colspan="2" class="heading">' + heading + '</td></tr>\n')
      self.write('</table>')

   def write_section_item(self, text):
      """Writes text and follows with a horizontal rule"""

      self.write(text + '\n')

   def process(self, decl):
      "Print out the details for the children of the given decl"

      doc = self.processor.documentation
      sorter = self.processor.sorter
      sorter.set_scope(decl)
      sorter.sort_section_names()

      # Iterate through the sections with details
      self.write_start()
      for section in sorter.sections():
         # Write a heading
         heading = section+' Details:'
         started = 0 # Lazy section start incase no details for this section
         # Iterate through the children in this section
         for child in sorter.children(section):
            # Check if need to add to detail list
            if not doc.details(child, self.view()):
               continue
            # Check section heading
            if not started:
               started = 1
               self.write_section_start(heading)
            child.accept(self)
         # Finish the section
         if started: self.write_section_end(heading)
      self.write_end()

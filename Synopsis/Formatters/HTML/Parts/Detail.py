# $Id: Detail.py,v 1.1 2003/11/15 19:54:05 stefan Exp $
#
# Copyright (C) 2000 Stephen Davies
# Copyright (C) 2000 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.Formatters.HTML.Part import Part

from Synopsis.Formatters.HTML import FormatStrategy
from Synopsis.Formatters.HTML.Tags import *
from Synopsis.Formatters.HTML.DeclarationStyle import *

class Detail(Part):

   def register(self, page):

      Part.register(self, page)
      self._init_formatters('detail_formatters', 'detail')

   def _init_default_formatters(self):

      self.addFormatter( FormatStrategy.DetailAST )
      #self.addFormatter( ClassHierarchySimple )
      self.addFormatter( FormatStrategy.DetailCommenter )

   def writeSectionStart(self, heading):
      """Creates a table with one row. The row has a td of class 'heading'
      containing the heading string"""

      self.write('<table width="100%%" summary="%s">\n'%heading)
      self.write('<tr><td colspan="2" class="heading">' + heading + '</td></tr>\n')
      self.write('</table>')

   def writeSectionItem(self, text):
      """Writes text and follows with a horizontal rule"""

      self.write(text + '\n<hr>\n')

   def process(self, decl):
      "Print out the details for the children of the given decl"

      decl_style = self.processor.decl_style
      SUMMARY = Style.SUMMARY

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
            if decl_style[child] == SUMMARY:
               continue
            # Check section heading
            if not started:
               started = 1
               self.writeSectionStart(heading)
            child.accept(self)
         # Finish the section
         if started: self.writeSectionEnd(heading)
      self.write_end()

#
# Copyright (C) 2000 Stephen Davies
# Copyright (C) 2000 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.Processor import Parameter
from Synopsis import Util
from Synopsis.Formatters.HTML.Part import Part
from Synopsis.Formatters.HTML.Fragments import *
from Synopsis.Formatters.HTML.Tags import *

class Summary(Part):
   """Formatting summary visitor. This formatter displays a summary for each
   declaration, with links to the details if there is one. All of this is
   controlled by the ASTFormatters."""

   fragments = Parameter([DeclarationSummaryFormatter(), SummaryCommenter()],
                         '')

   def register(self, view):

      if view.processor.has_view('XRef'):
         self.fragments.append(XRefLinker())
      if view.processor.has_view('Source'):
         self.fragments.append(SourceLinker())

      Part.register(self, view)
      self.__link_detail = 0

   def set_link_detail(self, boolean):
      """Sets link_detail flag to given value.
      @see label()"""

      self.__link_detail = boolean

   def label(self, ref, label=None):
      """Override to check link_detail flag. If it's set, returns a reference
      instead - which will be to the detailed info"""

      if label is None: label = ref
      if self.__link_detail:
         # Insert a reference instead
         return span('name',self.reference(ref, Util.ccolonName(label, self.scope())))
      return Part.label(self, ref, label)
	
   def write_section_start(self, heading):
      """Starts a table entity. The heading is placed in a row in a td with
      the class 'heading'."""

      self.write('<table width="100%%" summary="%s">\n'%heading)
      self.write('<col/><col width="100%" />')
      self.write('<tr><td class="heading" colspan="2">' + heading + '</td></tr>\n')

   def write_section_end(self, heading):
      """Closes the table entity and adds a break."""

      self.write('</table>\n<br/>\n')

   def write_section_item(self, text):
      """Adds a table row"""

      # fragment formatters place a <tab> as separator, so we can replace
      # it with proper html markup here
      #text = re.sub('\t', '</td><td class="summ-info">', text)
      self.write('<tr><td/><td class="summ-info">' + text + '</td></tr>\n')
      
   def process(self, scope):
      "Print out the summaries from the given scope"

      doc = self.processor.documentation
      sorter = self.processor.sorter
      sorter.set_scope(scope)
      sorter.sort_section_names()

      self.write_start()
      for section in sorter.sections():
         # Write a header for this section
         if section[-1] == 's': heading = section+'es Summary:'
         else: heading = section+'s Summary:'
         self.write_section_start(heading)
         # Iterate through the children in this section
         for child in sorter.children(section):
            # Check if need to add to detail list
            if doc.details(child, self.view()):
               # Setup the linking stuff
               self.set_link_detail(1)
               child.accept(self)
               self.set_link_detail(0)
            else:
               # Just do it
               child.accept(self)
         # Finish off this section
         self.write_section_end(heading)
      self.write_end()


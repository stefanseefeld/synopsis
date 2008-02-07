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

class Summary(Part):
    """Formatting summary visitor. This formatter displays a summary for each
    declaration, with links to the details if there is one. All of this is
    controlled by the ASGFormatters."""

    fragments = Parameter([DeclarationSummaryFormatter(), SummaryCommenter()],
                         '')

    def register(self, view):

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
            return span('name',self.reference(ref, str(self.scope().prune(label))))
        return Part.label(self, ref, label)
	
    def write_section_start(self, heading):
        """Start a 'summary' section and write an appropriate heading."""

        self.write('<div class="summary">\n')
        self.write(div('heading', heading) + '\n')

    def write_section_end(self, heading):
        """Close the section."""
        
        self.write('</div><!-- summary -->\n')

    def write_section_item(self, text):
        """Add an item."""

        self.write(div('item', text) + '\n')
      
    def process(self, scope):
        "Print out the summaries from the given scope"

        doc = self.processor.documentation
        sorter = self.processor.sorter.clone(scope.declarations)

        self.write_start()
        for section in sorter:
            # Write a header for this section
            heading = section+' Summary:'
            self.write_section_start(heading)
            # Iterate through the children in this section
            for child in sorter[section]:
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


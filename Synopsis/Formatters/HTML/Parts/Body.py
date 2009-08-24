#
# Copyright (C) 2009 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis import ASG
from Synopsis.Processor import Parameter
from Synopsis.Formatters.HTML.Part import Part
from Synopsis.Formatters.HTML.Fragments import *
from Synopsis.Formatters.HTML.Tags import *

class Body(Part):

    fragments = Parameter([DeclarationDetailFormatter(),
                           DeclarationCommenter()],
                         '')

    def write_section_start(self, heading):
        """Start a 'detail' section and write an appropriate heading."""

        c = self.view().generate_id()
        toggle = span('-', class_='toggle', id="toggle_s%d"%c,
                      onclick='return body_section_toggle(\'s%d\');'%c)
        self.write(div(toggle + heading, class_='heading') + '\n')
        self.write('<div class="body expanded" id="s%d">\n'%c)

    def write_section_end(self, heading):
        """Close the section."""
        
        self.write('</div><!-- body -->\n')
        
    def write_section_item(self, text):
        """Add an item."""

        self.write(div(text, class_='item') + '\n')

    def process(self, decl):
        "Print out the details for the children of the given decl"

        if type(decl) == ASG.Forward:
            return

        doc = self.processor.documentation
        sorter = self.processor.sorter.clone(decl.declarations)

        # Iterate through the sections with details
        self.write_start()
        for section in sorter:
            started = 0 # Lazy section start incase no details for this section
            # Iterate through the children in this section
            for child in sorter[section]:
                # Check if need to add to detail list
                if not doc.details(child, self.view()):
                    continue
                # Check section heading
                if not started:
                    started = 1
                    self.write_section_start(section)
                child.accept(self)
            # Finish the section
            if started: self.write_section_end(section)
        self.write_end()

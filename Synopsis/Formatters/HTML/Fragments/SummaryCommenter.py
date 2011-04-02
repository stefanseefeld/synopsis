#
# Copyright (C) 2000 Stephen Davies
# Copyright (C) 2000 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.Formatters.HTML.Tags import *
from Default import Default

class SummaryCommenter(Default):
    """Adds summary annotations to all declarations."""

    def format_declaration(self, decl):
        doc = self.processor.documentation.doc(decl, self.view)
        return div(doc.summary or '', class_='doc')

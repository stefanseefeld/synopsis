#
# Copyright (C) 2000 Stephen Davies
# Copyright (C) 2000 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.Formatters.HTML.Tags import *
from Default import Default

class DetailCommenter(Default):
    """Add annotation details to all declarations."""

    def format_declaration(self, decl):

        details = self.processor.documentation.details(decl, self.view)
        return details and div('doc', details) or ''


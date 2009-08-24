#
# Copyright (C) 2009 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.Formatters.HTML.Tags import *
from Default import Default

class DeclarationCommenter(Default):
    """Add annotation details to all declarations."""

    def format_declaration(self, decl):

        doc = self.processor.documentation.doc(decl, self.view)
        if doc.has_details():
            c = self.view.generate_id()
            more = span(' More...', class_='expand-toggle',
                        onclick='return decl_doc_expand(\'d%d\');'%c)
            summary = div(doc.summary + more, class_='summary collapsed')
            less = para('-', class_='collapse-toggle expanded',
                        onclick='return decl_doc_collapse(\'d%d\');'%c)
            details = div(doc.details, class_='details expanded')
            return div(less + '\n' + summary + '\n' + details,
                       class_='doc collapsible', id='d%d'%c)
        else:
            return div(doc.details or '', class_='doc')
            

#
# Copyright (C) 2000 Stephen Davies
# Copyright (C) 2000 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.Formatters.HTML.Tags import *
from Default import Default

class XRefLinker(Default):
    """Adds an xref link to all declarations"""

    def register(self, part):

        Default.register(self, part)
        self.pager = self.processor.xref
        self.sxr = self.processor.sxr_prefix and self.processor.ir.sxr

    def format_declaration(self, decl):

        entry = self.sxr and self.sxr.get(decl.name)
        if not entry:
            return ''
        page = self.pager.get(decl.name)
        url = self.directory_layout.xref(page)
        url += '#' + quote_as_id(str(decl.name))
        label = img(src=rel(self.view.filename(), 'xref.png'), alt='references')
        return href(rel(self.view.filename(), url), label)


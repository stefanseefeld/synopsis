#
# Copyright (C) 2000 Stephen Davies
# Copyright (C) 2000 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.Formatters.HTML.Tags import *
from Default import Default

_icons = {'C':'src-c.png',
          'C++':'src-c++.png',
          'Python':'src-py.png'}


class SourceLinker(Default):
    """Adds a link to the decl on the file view to all declarations"""

    def register(self, formatter):

        Default.register(self, formatter)
        self.sxr = self.processor.sxr_prefix and True

    def format_declaration(self, decl):

        if not self.sxr or not decl.file: return ''
        language = decl.file.annotations['language']
        icon = _icons[language]
        url = self.directory_layout.file_source(decl.file.name)
        url += '#%d' % decl.line
        label = img(src=rel(self.view.filename(), icon), alt='source code', border='0')
        return href(rel(self.view.filename(), url), label)

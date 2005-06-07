#
# Copyright (C) 2005 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis import AST
from Filter import Filter

import re

class QtFilter(Filter):
    """A class that finds Qt style comments. These have two styles: //! ...
    and /*! ... */. The first means "brief comment" and there must only be
    one. The second type is the detailed comment."""

    __re_brief = r"[ \t]*//!(.*)"
    __re_detail = r"[ \t]*/\*!(.*)\*/[ \t\n]*"
    
    def __init__(self):
        "Compiles the regular expressions"

        self.re_brief = re.compile(self.__re_brief)
        self.re_detail = re.compile(self.__re_detail, re.S)

    def filter_comment(self, comment):
        "Matches either brief or detailed comments"

        text = comment.text()
        mo = self.re_brief.match(text)
        if mo:
            comment.set_text(mo.group(1))
            return
        mo = self.re_detail.match(text)
        if mo:
            comment.set_text(mo.group(1))
            return
        comment.set_text('')


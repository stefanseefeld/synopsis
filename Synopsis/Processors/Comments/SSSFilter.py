#
# Copyright (C) 2005 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis import AST
from Filter import Filter

import re

class SSSFilter(Filter):
    """A class that selects only /// comments."""

    def __init__(self):
        "Compiles the regular expressions"

        self.re_star = re.compile(r'/\*(.*?)\*/', re.S)
        self.re_sss = re.compile(r'^[ \t]*/// ?(.*)$', re.M)

    def filter_comment(self, comment):
        """Replaces the text in the comment. It calls strip_star() first to
        remove all multi-line star comments, then follows with filter_ss().
        """

        text = comment.text()
        text = self.filter_sss(self.strip_star(text))
        comment.set_text(text)

    def strip_star(self, str):
        """Strips all star-format comments from the string"""

        mo = self.re_star.search(str)
        while mo:
            str = str[:mo.start()] + str[mo.end():]
            mo = self.re_star.search(str)
        return str

    def filter_sss(self, str):
        """Filters str and returns just the lines that start with ///"""

        return '\n'.join(self.re_sss.findall(str))


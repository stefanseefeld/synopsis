#
# Copyright (C) 2005 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis import AST
from Filter import Filter

import re

class CFilter(Filter):
    """A class that formats C /* */ style comments"""

    __re_c = r"/[\*]+[ \t]*(?P<text>.*)(?P<lines>(\n[ \t]*.*)*?)(\n[ \t]*)?[\*]+/"
    # match lines with and without asterisk.
    # preserve space after asterisk so empty lines are formatted correctly.
    __re_line = r"\n[ \t]*([ \t]*[\*]+(?=[ \t\n]))*(?P<text>.*)"

    def __init__(self):
        """Compiles the regular expressions"""

        if self.debug:
            print 'CComment Processor requested.\n'
        self.re_c = re.compile(CFilter.__re_c)
        self.re_line = re.compile(CFilter.__re_line)

    def filter_comment(self, comment):
        """Finds comments in the C format. The format is  /* ... */.
        It has to cater for all five line forms: "/* ...", " * ...", " ...",
        " */" and the one-line "/* ... */".
        """

        text = comment.text()
        text_list = []
        mo = self.re_c.search(text)
        while mo:
            text_list.append(mo.group('text'))
            lines = mo.group('lines')
            if lines:
                mol = self.re_line.search(lines)
                while mol:
                    text_list.append(mol.group('text'))
                    mol = self.re_line.search(lines, mol.end())
            mo = self.re_c.search(text, mo.end())
        text = string.join(text_list,'\n')
        comment.set_text(text)



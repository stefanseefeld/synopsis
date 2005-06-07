#
# Copyright (C) 2005 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis import AST
from Filter import Filter

import re

class JavaFilter(Filter):
    """A class that formats java /** style comments"""

    __re_java = r"/\*\*[ \t]*(?P<text>.*)(?P<lines>(\n[ \t]*\*.*)*?)(\n[ \t]*)?\*/"
    __re_line = r"\n[ \t]*\*[ \t]*(?P<text>.*)"

    def __init__(self):
        "Compiles the regular expressions"

        self.re_java = re.compile(JavaFilter.__re_java)
        self.re_line = re.compile(JavaFilter.__re_line)

    def filter_comment(self, comment):
      """Finds comments in the java format. The format is  /** ... */, and
      it has to cater for all four line forms: "/** ...", " * ...", " */" and
      the one-line "/** ... */".
      """

      text = comment.text()
      text_list = []
      mo = self.re_java.search(text)
      while mo:
         text_list.append(mo.group('text'))
         lines = mo.group('lines')
         if lines:
            mol = self.re_line.search(lines)
            while mol:
               text_list.append(mol.group('text'))
               mol = self.re_line.search(lines, mol.end())
         mo = self.re_java.search(text, mo.end())
      text = '\n'.join(text_list)
      comment.set_text(text)


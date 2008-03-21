#
# Copyright (C) 2005 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.Processor import Processor
from Synopsis import ASG
import re

class Filter(Processor, ASG.Visitor):
    """Base class for comment filters."""

    def process(self, ir, **kwds):

        self.set_parameters(kwds)

        self.ir = self.merge_input(ir)

        for d in ir.asg.declarations:
            d.accept(self)

        return self.output_and_return_ir()


    def visit_declaration(self, decl):

        comments = decl.annotations.get('comments', [])
        comments[:] = [c is not None and self.filter_comment(c) or None
                       for c in comments]

    visit_builtin = visit_declaration

    def filter_comment(self, comment):
        """Filter comment."""

        return comment


class CFilter(Filter):
    """A class that filters C-style comments."""

    comment = r'/[\*]+[ \t]*(?P<text>.*)(?P<lines>(\n[ \t]*.*)*?)(\n[ \t]*)?[\*]+/'
    # Match lines with and without asterisk.
    # Preserve space after asterisk so empty lines are formatted correctly.
    line = r'\n[ \t]*([ \t]*[\*]+(?=[ \t\n]))*(?P<text>.*)'

    def __init__(self, **kwds):
        """Compiles the regular expressions"""

        Filter.__init__(self, **kwds)
        self.comment = re.compile(CFilter.comment)
        self.line = re.compile(CFilter.line)

    def filter_comment(self, comment):
        """Finds comments in the C format. The format is  /* ... */.
        It has to cater for all five line forms: "/* ...", " * ...", " ...",
        " */" and the one-line "/* ... */".
        """

        text = []
        mo = self.comment.search(comment)
        while mo:
            text.append(mo.group('text'))
            lines = mo.group('lines')
            if lines:
                mol = self.line.search(lines)
                while mol:
                    text.append(mol.group('text'))
                    mol = self.line.search(lines, mol.end())
            mo = self.comment.search(comment, mo.end())
        return '\n'.join(text)


class SSFilter(Filter):
    """A class that selects only // comments."""

    ss = r'^[ \t]*// ?(.*)$'

    def __init__(self, **kwds):
        "Compiles the regular expressions"

        Filter.__init__(self, **kwds)
        self.ss = re.compile(SSFilter.ss, re.M)


    def filter_comment(self, comment):
        """"""

        return '\n'.join(self.ss.findall(comment))


class SSDFilter(Filter):
    """A class that selects only //. comments."""

    ssd = r'^[ \t]*//\. ?(.*)$'

    def __init__(self, **kwds):
        "Compiles the regular expressions"

        Filter.__init__(self, **kwds)
        self.ssd = re.compile(SSDFilter.ssd, re.M)


    def filter_comment(self, comment):
        """"""

        return '\n'.join(self.ssd.findall(comment))


class SSSFilter(Filter):
    """A class that selects only /// comments."""

    sss = r'^[ \t]*/// ?(.*)$'

    def __init__(self, **kwds):
        "Compiles the regular expressions"

        Filter.__init__(self, **kwds)
        self.sss = re.compile(SSSFilter.sss, re.M)


    def filter_comment(self, comment):
        """"""

        return '\n'.join(self.sss.findall(comment))


class QtFilter(Filter):
    """A class that finds Qt style comments. These have two styles: //! ...
    and /*! ... */. The first means "brief comment" and there must only be
    one. The second type is the detailed comment."""

    brief = r"[ \t]*//!(.*)"
    detail = r"[ \t]*/\*!(.*)\*/[ \t\n]*"
    
    def __init__(self, **kwds):
        "Compiles the regular expressions"

        Filter.__init__(self, **kwds)
        self.brief = re.compile(QtFilter.brief)
        self.detail = re.compile(QtFilter.detail, re.S)


    def filter_comment(self, comment):
        "Matches either brief or detailed comments."

        mo = self.brief.match(comment)
        if mo:
            return mo.group(1)
        else:
            mo = self.detail.match(comment)
            if mo:
                return mo.group(1)
        return ''


class JavaFilter(Filter):
    """A class that selects java /** style comments"""

    java = r'/\*\*[ \t]*(?P<text>.*)(?P<lines>(\n[ \t]*\*.*)*?)(\n[ \t]*)?\*/'
    line = r'\n[ \t]*\*[ \t]*(?P<text>.*)'


    def __init__(self):
        "Compiles the regular expressions"

        self.java = re.compile(JavaFilter.java)
        self.line = re.compile(JavaFilter.line)


    def filter_comment(self, comment):
      """Finds comments in the java format. The format is  /** ... */, and
      it has to cater for all four line forms: "/** ...", " * ...", " */" and
      the one-line "/** ... */".
      """

      text_list = []
      mo = self.java.search(comment)
      while mo:
         text_list.append(mo.group('text'))
         lines = mo.group('lines')
         if lines:
            mol = self.line.search(lines)
            while mol:
               text_list.append(mol.group('text'))
               mol = self.line.search(lines, mol.end())
         mo = self.java.search(comment, mo.end())
      return '\n'.join(text_list)


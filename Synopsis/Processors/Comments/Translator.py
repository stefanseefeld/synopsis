#
# Copyright (C) 2006 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis import ASG
from Synopsis.DocString import DocString
from Synopsis.Processor import Processor, Parameter
from Filter import *

class Translator(Processor, ASG.Visitor):
    """A Translator translates comments into documentation."""

    filter = Parameter(SSFilter(), 'A comment filter to apply.')
    processor = Parameter(None, 'A comment processor to run.')
    markup = Parameter('', 'The markup type for this declaration.')
    concatenate = Parameter(False, 'Whether or not to concatenate adjacent comments.')
    primary_only = Parameter(True, 'Whether or not to preserve secondary comments.')

    def process(self, ir, **kwds):
      
        self.set_parameters(kwds)
        self.ir = self.merge_input(ir)
        if self.filter:
            self.ir = self.filter.process(self.ir)
        if self.processor:
            self.ir = self.processor.process(self.ir)

        for decl in self.ir.declarations:
            decl.accept(self)

        return self.output_and_return_ir()


    def visit_declaration(self, decl):
        """Map comments to a doc string."""

        comments = decl.annotations.get('comments')
        if comments:
            text = None
            if self.primary_only:
                text = comments[-1]
            elif self.combine:
                text = ''.join([c for c in comments if c])
            else:
                comments = comments[:]
                comments.reverse()
                for c in comments:
                    if c is not None:
                        text = c
                        break
            doc = DocString(text or '', self.markup)
            decl.annotations['doc'] = doc

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

        for sf in self.ir.files.values():
            self.visit_sourcefile(sf)

        for decl in self.ir.asg.declarations:
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

    def visit_sourcefile(self, sf):
        """Map comments to a doc string."""

        comments = sf.annotations.get('comments')
        if comments:
            text = None
            if self.primary_only:
                # A 'primary' comment for a file is the first one,
                # not the last.
                text = comments[0]
            elif self.combine:
                text = ''.join([c for c in comments if c])
            else:
                comments = comments[:]
                for c in comments:
                    if c is not None:
                        text = c
                        break
            doc = DocString(text or '', self.markup)
            sf.annotations['doc'] = doc

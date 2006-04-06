#
# Copyright (C) 2006 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis import AST, Type
from Synopsis.Formatters.HTML.Tags import *
from Formatter import Formatter
from docutils import nodes
from docutils.nodes import NodeVisitor, Text
from docutils.frontend import OptionParser
from docutils.utils import new_reporter
import string, re

class RST(Formatter):

    def format_summary(self, view, decl):
        summary = decl.comments()[-1].summary
        print 'this is a summary'
        return summary and self.translate(summary) or ''

    def format(self, view, decl):
        details = decl.comments()[-1].details
        print 'xxx', decl.comments()[-1].text, details
        return details and self.translate(details) or ''

    def translate(self, document, directory=None, docindex=None, context=None):

        print 'translating', document
        if not document.reporter:
            # We unset this before pickling, so we have to restore it now.
            fe = OptionParser()
            settings = fe.get_default_values()
            settings.update({
                'warning_stream': '',
                'halt_level': 10000,
                'report_level': 10000,
                }, fe)
            document.reporter = new_reporter('', settings)

        #linker = Linker()
        #visitor = Translator(document, linker,
        #                     directory, docindex, context)
        #document.walkabout(visitor)
        #return ''.join(visitor.body)

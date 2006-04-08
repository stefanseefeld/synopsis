#
# Copyright (C) 2006 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis import AST, Type
from Synopsis.Formatters.HTML.Tags import *
from Synopsis.Formatters.HTML.Markup import *
from docutils.nodes import NodeVisitor, Text
from docutils.core import *
import string, re

class SummaryExtractor(NodeVisitor):
    """A docutils node visitor that extracts the first sentence from
    the first paragraph in a document."""

    def __init__(self, document):

        NodeVisitor.__init__(self, document)
        self.summary = None
        

    def visit_document(self, node):

        self.summary = None

        
    def visit_paragraph(self, node):

        if self.summary is not None:
            return

        summary_pieces = []

        # Extract the first sentence.
        for child in node:
            if isinstance(child, Text):
                m = re.match(r'(\s*[\w\W]*?\.)(\s|$)', child.data)
                if m:
                    summary_pieces.append(Text(m.group(1)))
                    break
            summary_pieces.append(child)
            
        summary_doc = self.document.copy()
        summary_doc[:] = summary_pieces
        self.summary = summary_doc


    def unknown_visit(self, node):
        'Ignore all unknown nodes'

        pass


class RST(Formatter):


    def format(self, decl, view):

        doc = decl.annotations.get('doc')
        if doc:
            print 'text', doc.text
            parts = publish_parts(doc.text, writer_name='html')
            print parts.keys()
        return Struct('', parts['html_body'])


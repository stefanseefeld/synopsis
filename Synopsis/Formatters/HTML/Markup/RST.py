#
# Copyright (C) 2006 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis import AST, Type
from Synopsis.Formatters.HTML.Tags import *
from Synopsis.Formatters.HTML.Markup import *
from docutils import nodes
from docutils.nodes import NodeVisitor, Text
from docutils.frontend import OptionParser
from docutils.utils import new_reporter
from docutils.core import *
import string, re

class RST(Formatter):

    def format(self, decl, view):

        doc = decl.annotations.get('doc')
        if doc:
            parts = publish_parts(doc.text, writer_name='html')
            print parts.keys()
        return Struct('', parts['html_body'])


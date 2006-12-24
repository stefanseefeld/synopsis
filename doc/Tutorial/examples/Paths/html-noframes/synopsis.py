#
# Copyright (C) 2006 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.process import process
from Synopsis.Processor import Processor, Parameter, Composite
from Synopsis.Parsers import Cxx
from Synopsis.Processors import *
from Synopsis.Processors import Comments
from Synopsis.Formatters import HTML
from Synopsis.Formatters.HTML.DirectoryLayout import *
from Synopsis.Formatters.HTML.Views import *
from Synopsis.Formatters import Dot

cxx = Cxx.Parser(base_path='../src/',
                 syntax_prefix='links',
                 xref_prefix='xref')

ss = Comments.Translator(filter = Comments.SSFilter(),
                         processor = Comments.Grouper())
ssd_prev = Comments.Translator(filter = Comments.SSDFilter(),
                               processor = Composite(Comments.Previous(),
                                                     Comments.Grouper()))
javadoc = Comments.Translator(markup='javadoc',
                              filter = Comments.JavaFilter(),
                              processor = Comments.Grouper())
rst = Comments.Translator(markup='rst',
                          filter = Comments.SSDFilter(),
                          processor = Comments.Grouper())

html = HTML.Formatter(directory_layout=DirectoryLayout(),
                      index = [],
                      detail = [],
                      content = [Scope(),
                                 Source(prefix = 'links'),
                                 XRef(xref_file = 'Paths.xref'),
                                 FileDetails(),
                                 InheritanceTree(),
                                 InheritanceGraph(),
                                 NameIndex()])

process(cxx_ss = Composite(cxx, ss),
        cxx_ssd_prev = Composite(cxx, ssd_prev),
        cxx_javadoc = Composite(cxx, javadoc),
        cxx_rst = Composite(cxx, rst),
        link = Linker(),
        xref = XRefCompiler(prefix='xref'),
        html = html)

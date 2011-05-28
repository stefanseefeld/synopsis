#
# Copyright (C) 2006 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.process import process
from Synopsis.Processor import *
from Synopsis.Parsers import IDL
from Synopsis.Processors import Linker
from Synopsis.Processors import Comments
from Synopsis.Formatters import HTML

parser = Composite(IDL.Parser(cppflags=['-I.'], base_path='.'),
                   Comments.Translator(filter = Comments.SSDFilter(),
                                       processor = Comments.Grouper()))
linker = Linker()

format = HTML.Formatter()

process(parse = parser,
        link = linker,
        format = format)

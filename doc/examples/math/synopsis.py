#
# Copyright (C) 2008 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.process import process
from Synopsis.Processor import Processor, Parameter, Composite
from Synopsis.Parsers import C
from Synopsis.Processors import Linker
from Synopsis.Processors import Comments
from Synopsis.Processors import TypedefFolder
from Synopsis.Formatters import DocBook

parser = C.Parser(base_path='.')

translator = Comments.Translator(filter = Comments.CFilter(),
                                 processor = Composite(Comments.Previous(),
                                                       Comments.Grouper()))

process(parse = Composite(parser, TypedefFolder(), translator),
        docbook = DocBook.Formatter())

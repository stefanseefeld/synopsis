#! /usr/bin/env python

from Synopsis.process import process
from Synopsis.Processor import *
from Synopsis.Parsers import IDL
from Synopsis.Processors import Linker
from Synopsis.Processors import Comments
from Synopsis.Formatters import HTML
from Synopsis.Formatters.HTML.TreeFormatterJS import TreeFormatterJS

parser = Composite(IDL.Parser(cppflags=['-I.']),
                   Comments.Translator(filter = Comments.SSDFilter(),
                                       processor = Comments.Grouper()))
linker = Linker()

format = HTML.Formatter(tree_formatter = TreeFormatterJS())

process(parse = parser,
        link = linker,
        format = format)

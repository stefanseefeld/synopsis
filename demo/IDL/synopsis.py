#! /usr/bin/env python

from Synopsis.process import process
from Synopsis.Processor import *
from Synopsis.Parsers import IDL
from Synopsis.Processors import *
from Synopsis.Formatters import HTML
from Synopsis.Formatters.HTML.TreeFormatterJS import TreeFormatterJS

parser = IDL.Parser(include_paths=['.'])

linker = Linker(SSDComments())      # filter out any non-'//.' comments

format = HTML.Formatter(tree_formatter = TreeFormatterJS())

process(parse = parser,
        link = linker,
        format = format)

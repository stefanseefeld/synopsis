#! /usr/bin/env python

from Synopsis.process import process
from Synopsis.Processor import *
from Synopsis.Parsers import Cxx
from Synopsis.Processors import *
from Synopsis.Formatters import HTML

linker = Linker(Stripper(),         # strip prefix (see Linker.Stripper.Stripper docs)
                NameMapper(),       # apply name mapping if any (prefix adding, etc.)
                JavaComments(),     # only keep javadoc-like comments
                JavaTags(),         # process javadoc-like tags
                Summarizer(),       
                EmptyNS(),          # skip empty namespaces
                AccessRestrictor()) # filter out unwanted ('private', say) declarations

process(parse = Cxx.Parser(),
        link = linker,
        format = HTML.Formatter(stylesheet_file = '../../html.css'))

#! /usr/bin/env python

from Synopsis.process import process
from Synopsis.Processor import *
from Synopsis.Parsers import Cxx
from Synopsis.Formatters import Dot

process(parse = Cxx.Parser(),
        dot = Dot.Formatter(hide_operations=False,
                            hide_attributes=False))


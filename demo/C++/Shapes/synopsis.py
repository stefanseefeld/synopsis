#! /usr/bin/env python

from Synopsis.process import process
from Synopsis.Processor import *
from Synopsis.Parsers import Cxx
from Synopsis.Processors import *
from Synopsis.Formatters import Dot

dot = Dot.Formatter(hide_attributes = False,
                    hide_operations = False)

process(parse = Cxx.Parser(),
        uml = dot)

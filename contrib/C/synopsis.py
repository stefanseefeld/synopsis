#! /usr/bin/env python

from Synopsis.process import process
from Synopsis.Processor import *
from Synopsis.Processors import *
from Synopsis.Formatters import Dump
from Synopsis.Formatters import HTML

from Parser import Parser

from distutils import sysconfig
import sys, os.path

parser = Parser(emulate_compiler = 'gcc')
html = HTML.Formatter(title = 'Test case for C parser')
process(parse = parser,
        html = html,
        all = Composite(parser, html),
        dump = Composite(parser, Dump.Formatter()))

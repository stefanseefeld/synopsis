#! /usr/bin/env python

from Synopsis.process import process
from Synopsis.Processor import *
from Synopsis.Parsers import Cxx
from Synopsis.Processors import *
from Synopsis.Formatters import Dump
from Synopsis.Formatters import HTML

# this is actually only here as a hack for backward compatibility.
import glob
extra_input = glob.glob('boost/boost/python/*.hpp')

# the python include path can be obtained from distutils.sysconfig,
# assuming that the python version used to run synopsis is the same
# boost should be compiled with
parser = Cxx.Parser(cppflags = ['-DPYTHON_INCLUDE=<python2.2/Python.h>',
                                '-DBOOST_PYTHON_SYNOPSIS',
                                '-Iboost',
                                '-I/usr/include/python2.2'],
                    base_path = 'boost/',
                    main_file_only = True,
                    syntax_prefix = 'links/',
                    xref_prefix = 'xref/',
                    extract_tails = True,
                    emulate_compiler = 'g++',
                    # 'extra_files' will go away shortly
                    extra_files = extra_input)

xref = XRefCompiler(prefix='xref/')    # compile xref dictionary


linker = Linker(Stripper(),         # strip prefix (see Linker.Stripper.Stripper docs)
                NameMapper(),       # apply name mapping if any (prefix adding, etc.)
                SSComments(),       # filter out any non-'//' comments
                Grouper2(),         # group declarations according to '@group' tags
                CommentStripper(),  # strip any 'suspicious' comments
                Previous(),         # attach '//<' comments
                AccessRestrictor()) # filter out unwanted ('private', say) declarations

formatter = HTML.Formatter(stylesheet_file = '../../html.css')

process(parse2 = parser,
        parse = Composite(parser, linker),
        xref = xref,
        link = linker,
        format = formatter)

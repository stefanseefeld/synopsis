#! /usr/bin/env python

from Synopsis.process import process
from Synopsis.Processor import *
from Synopsis.Parsers import Cxx
from Synopsis.Processors import *
from Synopsis.Formatters import Dump
from Synopsis.Formatters import HTML
from Synopsis.Formatters import SXR

from distutils import sysconfig
import sys, os.path

# the python include path can be obtained from distutils.sysconfig,
# assuming that the python version used to run synopsis is the same
# boost should be compiled with
parser = Cxx.Parser(cppflags = ['-DPYTHON_INCLUDE=<python%s/Python.h>'%sys.version[0:3],
                                '-DBOOST_PYTHON_SYNOPSIS',
                                '-Iboost',
                                '-I%s'%(sysconfig.get_python_inc())],
                    base_path = 'boost/',
                    primary_file_only = False,
                    sxr_prefix = 'sxr')

translator = Comments.Translator(markup='rst',               # use restructured text markup in comments
                                 filter=Comments.SSFilter(), # filter out any non-'//' comments
                                 processor=Composite(Comments.Grouper(),
                                                     Comments.Previous()))

linker = Linker(translator)

html = HTML.Formatter(title = 'Boost Python Reference Manual')

sxr = SXR.Formatter(url = '/sxr.cgi',
                    src_dir = 'boost/',
                    sxr_prefix='sxr')

process(parse = Composite(parser, linker),
        link = linker,
        dump = Dump.Formatter(),
        html = html,
        sxr = sxr)

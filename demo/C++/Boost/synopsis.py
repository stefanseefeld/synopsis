#! /usr/bin/env python

from Synopsis.process import process
from Synopsis.Processor import *
from Synopsis.Parsers import Cxx
from Synopsis.Processors import *
from Synopsis.Formatters import Dump
from Synopsis.Formatters import HTML
from Synopsis.Formatters.HTML.Views import *
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
                    main_file_only = False,
                    syntax_prefix = 'links/',
                    xref_prefix = 'xref/',
                    emulate_compiler = 'g++')

xref = XRefCompiler(prefix='xref/')    # compile xref dictionary


linker = Linker(SSComments(),       # filter out any non-'//' comments
                Grouper1(),         # group declarations according to '@group' tags
                Previous(),         # attach '//<' comments
                CommentStripper(),  # strip any 'suspicious' comments
                AccessRestrictor()) # filter out unwanted ('private', say) declarations

html = HTML.Formatter(views = [FramesIndex(),
                               Scope(),
                               ModuleListing(),
                               ModuleIndexer(),
                               FileListing(),
                               FileIndexer(),
                               FileDetails(),
                               InheritanceTree(),
                               InheritanceGraph(),
                               Source(prefix = 'links'),
                               NameIndex(),
                               XRef(xref_file = 'bpl.xref')])

sxr = SXR.Formatter(url = '/sxr.cgi',
                    src_dir = 'boost/',
                    xref_prefix='xref',
                    syntax_prefix='links')

process(parse = Composite(parser, linker),
        xref = xref,
        link = linker,
        dump = Dump.Formatter(),
        html = html,
        sxr = sxr)

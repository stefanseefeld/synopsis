#! /usr/bin/env python

from Synopsis import config
from Synopsis.process import process
from Synopsis.Processor import *
from Synopsis.Processors import *
from Synopsis.Parsers import Python
from Synopsis.Parsers import Cxx
from Synopsis.Formatters import HTML
from Synopsis.Formatters.HTML.FileLayout import *
from Synopsis.Formatters.HTML.TreeFormatterJS import *
from Synopsis.Formatters.HTML import Comments
from Synopsis.Formatters.HTML.Views import *
from Synopsis.Formatters import Texinfo
from Synopsis.Formatters import Dump

from distutils import sysconfig
import sys, os.path

topdir = os.path.abspath(sys.argv[0] + '/../../..')
python = Composite(Python.Parser(basename = topdir + '/'),
                   JavaTags(),
                   Summarizer())

cxx = Cxx.Parser(base_path = topdir,
                 cppflags = ['-I%s'%(topdir + '/Synopsis/Parsers/Cxx'),
                             '-I%s'%(sysconfig.get_python_inc())],
                 syntax_prefix='links',
                 xref_prefix='xref')

cxx_processor = Linker(SSDComments(),
                       JavaTags(),
                       Summarizer(),
                       NamePrefixer(prefix = ['Synopsis', 'Parsers', 'Cxx', 'Parser'],
                                    type = 'Package'))

html = HTML.Formatter(toc_out = 'links.toc',
                      file_layout = NestedFileLayout(),
                      tree_formatter = TreeFormatterJS(),
                      comment_formatters = [Comments.Javadoc(), Comments.Section()],
                      views = [FramesIndex(),
                               Scope(),
                               Source(prefix = 'links',
                                      toc_in = ['links.toc'],
                                      scope = 'Synopsis::Parsers::Cxx'),
                               XRef(xref_file = 'all.xref'),
                               ModuleListing(),
                               ModuleIndexer(),
                               FileListing(),
                               FileIndexer(),
                               FileDetails(),
                               InheritanceTree(),
                               InheritanceGraph(),
                               NameIndex()])

process(python = python,
        cxx = cxx,
        link_python = Linker(),
        link_cxx = cxx_processor,
        link = Linker(),
        strip = Stripper(),
        xref = XRefCompiler(prefix='xref'),
        html = html,
        dump = Dump.Formatter(),
        texi = Texinfo.Formatter())

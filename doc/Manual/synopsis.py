#! /usr/bin/env python

from Synopsis.process import process
from Synopsis.Processor import *
from Synopsis.Processors import *
from Synopsis.Parsers import Python
from Synopsis.Parsers import Cxx
from Synopsis.Formatters import HTML
from Synopsis.Formatters.HTML.FileLayout import *
from Synopsis.Formatters.HTML.TreeFormatterJS import *
from Synopsis.Formatters.HTML.CommentFormatter import *
from Synopsis.Formatters.HTML.Pages import *
from Synopsis.Formatters import TexInfo

from distutils import sysconfig

topdir = '../../'

python = Composite(Python.Parser(basename = topdir),
                   JavaTags(),
                   Summarizer())

cxx = Composite(Cxx.Parser(base_path = topdir,
                           cppflags = ['-I%s'%(topdir + 'Synopsis/Parsers/Cxx'),
                                       '-I%s'%(sysconfig.get_python_inc())],
                           syntax_prefix='links',
                           xref_prefix='xref'),
                EmptyNS(),
                Dummies(),
                SSDComments(),
                JavaTags(),
                Summarizer())

cxx_processor = Composite(Unduplicator(),
                          NamePrefixer(prefix = ['Synopsis','Parser','C++'],
                                       type = 'Package'))

html = HTML.Formatter(stylesheet_file = '../../demo/html.css',
                      toc_out = 'links.toc',
                      file_layout = NestedFileLayout(),
                      tree_formatter = TreeFormatterJS(),
                      comment_formatters = [JavadocFormatter(), SectionFormatter()],
                      pages = [FramesIndex(),
                               Scope(),
                               FileSource(prefix = 'links',
                                          toc_in = ['links.toc'],
                                          scope = 'Synopsis::Parser::C++::'),
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
        link_python = Processor(),
        link_cxx = cxx_processor,
        link = Processor(),
        strip = Stripper(),
        html = html,
        texi = TexInfo.Formatter())

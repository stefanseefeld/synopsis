#! /usr/bin/env python

from Synopsis.process import process
from Synopsis.Processor import *
from Synopsis.Parsers import Cxx
from Synopsis.Processors import *
from Synopsis.Formatters import HTML
from Synopsis.Formatters.HTML import Page
from Synopsis.Formatters.HTML.Pages import *
from Synopsis.Formatters import TexInfo

parser = Cxx.Parser(syntax_prefix = 'links',
                    xref_prefix = 'xref')

linker = Linker(Stripper(),         # strip prefix (see Linker.Stripper.Stripper docs)
                NameMapper(),       # apply name mapping if any (prefix adding, etc.)
                SSDComments(),      # filter out any non-'//.' comments
                Grouper1(),         # group declarations according to '@group' tags
                CommentStripper(),  # strip any 'suspicious' comments
                Previous(),         # attach '//<-' comments
                Dummies(),          # drop 'dummy' declarations
                EmptyNS(),          # skip empty namespaces
                AccessRestrictor()) # filter out unwanted ('private', say) declarations

xref = XRefCompiler(prefix = 'xref')

html = HTML.Formatter(stylesheet_file = '../../html.css',
                      pages = [FramesIndex(),
                               Scope(),
                               ModuleListing(),
                               ModuleIndexer(),
                               FileListing(),
                               FileIndexer(),
                               FileDetails(),
                               FileSource(prefix='links'),
                               InheritanceTree(),
                               InheritanceGraph(),
                               NameIndex(),
                               XRef(xref_file = 'GapBuffer.xref')])

template = Page.Template(template='sxr.tmpl',
                         copy_files = ['logo.png'])

sxr = HTML.Formatter(stylesheet_file = '../../html.css',
                     pages = [DirBrowse(template = template),
                              Scope(template = template),
                              ModuleListing(template = template),
                              InheritanceTree(template = template),
                              InheritanceGraph(template = template),
                              FileSource(prefix = 'links',
                                         #toc_from = 'XRef',
                                         template = template),
                              RawFile(template = template),
                              NameIndex(template = template),
                              XRef(xref_file = 'GapBuffer.xref',
                                   link_to_scope = True,
                                   template = template)])

texinfo = TexInfo.Formatter()

process(parse = Composite(parser, linker),
        xref = xref,
        link = linker,
        html = html,
        sxr = sxr,
        texinfo = texinfo)

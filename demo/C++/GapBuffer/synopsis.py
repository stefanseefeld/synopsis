#! /usr/bin/env python

from Synopsis.process import process
from Synopsis.Processor import *
from Synopsis.Parsers import Cxx
from Synopsis.Processors import *
from Synopsis.Formatters import HTML
from Synopsis.Formatters.HTML import View
from Synopsis.Formatters.HTML.Views import *
from Synopsis.Formatters import Texinfo

parser = Cxx.Parser(syntax_prefix = 'links',
                    xref_prefix = 'xref')

linker = Linker(Stripper(),         # strip prefix (see Linker.Stripper.Stripper docs)
                NameMapper(),       # apply name mapping if any (prefix adding, etc.)
                SSDComments(),      # filter out any non-'//.' comments
                Grouper1(),         # group declarations according to '@group' tags
                CommentStripper(),  # strip any 'suspicious' comments
                Previous(),         # attach '//<' comments
                AccessRestrictor()) # filter out unwanted ('private', say) declarations

xref = XRefCompiler(prefix = 'xref')

html = HTML.Formatter(stylesheet_file = '../../html.css',
                      views = [FramesIndex(),
                               Scope(),
                               ModuleListing(),
                               ModuleIndexer(),
                               FileListing(),
                               FileIndexer(),
                               FileDetails(),
                               Source(prefix='links'),
                               InheritanceTree(),
                               InheritanceGraph(),
                               NameIndex(),
                               XRef(xref_file = 'GapBuffer.xref')])

template = View.Template(template='sxr.tmpl',
                         copy_files = ['logo.png'])

sxr = HTML.Formatter(stylesheet_file = '../../html.css',
                     views = [DirBrowse(template = template),
                              Scope(template = template),
                              ModuleListing(template = template),
                              InheritanceTree(template = template),
                              InheritanceGraph(template = template),
                              Source(prefix = 'links',
                                     #toc_from = 'XRef',
                                     template = template),
                              RawFile(template = template),
                              NameIndex(template = template),
                              XRef(xref_file = 'GapBuffer.xref',
                                   link_to_scope = True,
                                   template = template)])

texinfo = Texinfo.Formatter()

process(parse = Composite(parser, linker),
        xref = xref,
        link = linker,
        html = html,
        sxr = sxr,
        texinfo = texinfo)

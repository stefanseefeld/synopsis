#! /usr/bin/env python

from Synopsis.process import process
from Synopsis.Processor import *
from Synopsis.Parsers import Cxx
from Synopsis.Parsers import IDL
from Synopsis.Processors import *
from Synopsis.Formatters import HTML
from Synopsis.Formatters.HTML import Page
from Synopsis.Formatters.HTML.Pages import *
from Synopsis.Formatters import TexInfo

class Cxx2IDL(TypeMapper):
    """this processor maps a C++ external reference to an IDL interface
    if the name either starts with 'POA_' or ends in '_ptr'"""

    def visitUnknown(self, unknown):

        name = unknown.name()
        if unknown.language() != "C++": return
        if name[0][0:4] == "POA_":
            interface = map(None, name)
            interface[0] = interface[0][4:]
            unknown.resolve("IDL", name, tuple(interface))
            if self.verbose:
               print "mapping", string.join(name, "::"), "to", string.join(interface, "::")
        elif name[-1][-4:] == "_ptr" or name[-1][-4:] == "_var":
            interface = map(None, name)
            interface[-1] = interface[-1][:-4]
            unknown.resolve("IDL", name, tuple(interface))
            if self.verbose:
               print "mapping", string.join(name, "::"), "to", string.join(interface, "::")

idl = Composite(IDL.Parser(),
                Linker(),           # remove duplicate and forward declarations
                SSDComments(),      # filter out any non-'//.' comments
                CommentStripper())  # strip any 'suspicious' comments

cxx = Composite(Cxx.Parser(preprocessor = 'c++',
                           cppflags = ['-I.', '-D__x86__']),
                Cxx2IDL(),          # map to interface to hide the skeletons
                Linker(),           # remove duplicate and forward declarations
                SSDComments(),      # filter out any non-'//.' comments
                CommentStripper())  # strip any 'suspicious' comments

format_idl = HTML.Formatter(stylesheet_file = '../html.css',
                            toc_out = 'interface.toc',
                            pages = [FramesIndex(),
                                     Scope(),
                                     ModuleListing(),
                                     ModuleIndexer(),
                                     FileListing(),
                                     FileIndexer(),
                                     FileDetails(),
                                     InheritanceGraph(),
                                     NameIndex()])

format_cxx = HTML.Formatter(stylesheet_file = '../html.css',
                            toc_in = ['interface.toc|../interface'],
                            pages = [FramesIndex(),
                                     Scope(),
                                     ModuleListing(),
                                     ModuleIndexer(),
                                     FileListing(),
                                     FileIndexer(),
                                     FileDetails(),
                                     InheritanceGraph(),
                                     NameIndex()])

process(idl = idl,
        cxx = cxx,
        format_idl = format_idl,
        format_cxx = format_cxx)

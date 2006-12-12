#! /usr/bin/env python

from Synopsis.process import process
from Synopsis.Processor import *
from Synopsis.Parsers import Cxx
from Synopsis.Parsers import IDL
from Synopsis.Processors import Linker
from Synopsis.Processors import Comments
from Synopsis.Processors import TypeMapper
from Synopsis.Formatters import HTML

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
                Linker(Comments.Translator(filter = Comments.SSDFilter(),
                                           processor = Comments.Grouper())))

cxx = Composite(Cxx.Parser(cppflags = ['-I.', '-D__x86__', '-I', '/usr/local/include/omniORB4/']),
                Linker(Cxx2IDL(),
                       Comments.Translator(filter = Comments.SSDFilter(),
                                           processor = Comments.Grouper())))

format_idl = HTML.Formatter(toc_out = 'interface.toc')
format_cxx = HTML.Formatter(toc_in = ['interface.toc|../interface'])

process(idl = idl,
        cxx = cxx,
        format_idl = format_idl,
        format_cxx = format_cxx)

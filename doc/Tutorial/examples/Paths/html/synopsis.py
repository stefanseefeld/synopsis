#
# Copyright (C) 2006 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.process import process
from Synopsis.Processor import Processor, Parameter, Composite
from Synopsis.Parsers import Cxx
from Synopsis.Parsers import Python
from Synopsis.Processors import Linker
from Synopsis.Processors import Comments
from Synopsis.Formatters import HTML
from Synopsis.Formatters import Dot
from Synopsis.Formatters import Dump

class Joker(Processor):
    
    parameter = Parameter(':-)', 'a friendly parameter')

    def process(self, ast, **keywords):
        # override default parameter values
        self.set_parameters(keywords)
        # merge in ast from 'input' parameter if given
        self.ast = self.merge_input(ast)

        print 'this processor is harmless...', self.parameter
      
        # write to output (if given) and return ast
        return self.output_and_return_ast()

cxx = Cxx.Parser(base_path='../src')

ss = Comments.Translator(filter = Comments.SSFilter(),
                         processor = Comments.Grouper())
ssd_prev = Comments.Translator(filter = Comments.SSDFilter(),
                               processor = Composite(Comments.Previous(),
                                                     Comments.Grouper()))
javadoc = Comments.Translator(markup='javadoc',
                              filter = Comments.JavaFilter(),
                              processor = Comments.Grouper())
rst = Comments.Translator(markup='rst',
                          filter = Comments.SSDFilter(),
                          processor = Comments.Grouper())

process(cxx_ss = Composite(cxx, ss),
        cxx_ssd_prev = Composite(cxx, ssd_prev),
        cxx_javadoc = Composite(cxx, javadoc),
        cxx_rst = Composite(cxx, rst),
        link = Linker(),
        html = HTML.Formatter(),
        dot = Dot.Formatter(),
        joker = Joker(parameter = '(-;'))

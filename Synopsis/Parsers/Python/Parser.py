# $Id: Parser.py,v 1.3 2003/11/11 03:03:19 stefan Exp $
#
# Copyright (C) 2003 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Processor import Processor, Parameter
from Core import AST
from python import parse

class Parser(Processor):

   basename = Parameter('', 'basename to strip off of the parsed modules')
    
   def process(self, ast, **kwds):

      input = kwds.get('input')
      self.set_parameters(kwds)
      for file in input:
         ast.merge(parse(file, 0, {}, None))

      return output_and_return_ast(self)

# $Id: Parser.py,v 1.5 2003/11/18 21:56:27 stefan Exp $
#
# Copyright (C) 2003 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.Processor import Processor, Parameter
from Synopsis import AST
from python import parse

class Parser(Processor):

   basename = Parameter('', 'basename to strip off of the parsed modules')
    
   def process(self, ast, **kwds):

      input = kwds.get('input')
      self.set_parameters(kwds)
      self.ast = ast
      for file in input:
         ast.merge(parse(file, 0, {}, None))

      return self.output_and_return_ast()

# $Id: TypeMapper.py,v 1.1 2003/11/19 16:21:39 stefan Exp $
#
# Copyright (C) 2003 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.Processor import Processor, Parameter
from Synopsis import AST, Type, Util

import string

class TypeMapper(Processor, Type.Visitor):
   """Base class for type mapping"""

   def process(self, ast, **kwds):

      self.set_parameters(kwds)
      self.ast = self.merge_input(ast)

      for type in self.ast.types().values():
         type.accept(self)

      return self.output_and_return_ast()


#
# Copyright (C) 2004 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

"""C Parser
"""

from Synopsis.Processor import Processor, Parameter
from Synopsis import AST
import ctool

class Parser(Processor):

   prefix = Parameter(None, 'prefix to strip off from the filename')

   def process(self, ast, **kwds):

      self.set_parameters(kwds)
      self.ast = ast

      for file in self.input:
          self.ast = ctool.parse(self.ast, file, self.prefix, self.verbose, self.debug)

      return self.output_and_return_ast()

#
# Copyright (C) 2003 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.Processor import Processor, Parameter
from Synopsis import ASG

class TypeMapper(Processor, ASG.Visitor):
   """Base class for type mapping"""

   def process(self, ir, **kwds):

      self.set_parameters(kwds)
      self.ir = self.merge_input(ir)

      for type in self.ir.asg.types.values():
         type.accept(self)

      return self.output_and_return_ir()


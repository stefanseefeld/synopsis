#!/usr/bin/env python
# $Id: synopsis_test.py,v 1.2 2003/12/03 05:43:55 stefan Exp $
#
# Copyright (C) 2003 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from qm.fields import TextField
from qm.executable import RedirectedExecutable
from qm.test.test import Test
from qm.test.result import Result

import os
import string
import sys

class ProcessorTest(Test):
   """Process an input file with a synopsis script and
   compare the output."""

   arguments = [TextField(name="srcdir", description="The source directory."),
                TextField(name="synopsis", description="The synopsis script."),
                TextField(name="input", description="The input files."),
                TextField(name="output", description="The output file."),
                TextField(name="expected", description="The output file.")]

   def run_processor(self, context, result):

      input = map(lambda x:os.path.join(self.srcdir, x), self.input)
      if not os.path.isdir(os.path.dirname(self.output)):
         os.makedirs(os.path.dirname(self.output))

      command = 'python %s parse --output=%s %s'%(self.synopsis,
                                                  self.output,
                                                  string.join(self.input, ' '))
      script = RedirectedExecutable(60) # 1 minute ought to be enough...
      status = script.Run(string.split(command))
      if status != 0:
         result.Fail("unable to run '%s'"%command)
      return status == 0

   def Run(self, context, result):

      if self.run_processor(context, result):
         expected = open(self.expected).readlines()
         output = open(self.output).readlines()
         if expected != output:
            result.Fail("output mismatch")

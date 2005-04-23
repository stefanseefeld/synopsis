#!/usr/bin/env python
#
# Copyright (C) 2003 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from qm.fields import TextField
from qm.executable import RedirectedExecutable
from qm.test.test import Test
from qm.test.resource import Resource
from qm.test.result import Result

import os, sys, string, re

class APITest(Test):
   """Compile and run a test to validate the C++ API."""

   arguments = [TextField(name="src", description="The source file."),
                TextField(name="exe", description="The executable file."),
                TextField(name="expected", description="The expected output file.")]

   def compile(self, context, result):
      if not os.path.isdir(os.path.dirname(self.exe)):
         os.makedirs(os.path.dirname(self.exe))

      CXX = context.get('CXX', 'c++')
      CPPFLAGS = context.get('CPPFLAGS', '')
      CXXFLAGS = context.get('CXXFLAGS', '')
      LDFLAGS = context.get('LDFLAGS', '')
      LIBS = context.get('LIBS', '')
      command = '%s %s %s %s -o %s %s %s'%(CXX, CPPFLAGS, CXXFLAGS, LDFLAGS,
                                           self.exe, self.src, LIBS)
      compiler = RedirectedExecutable()
      status = compiler.Run(string.split(command))
      if os.WIFEXITED(status) and os.WEXITSTATUS(status) == 0:
         return self.exe
      else:
         result.Fail('compilation failed',
                     {'synopsis_test.error': compiler.stderr,
                      'synopsis_test.command': command})
         return None      

   def Run(self, context, result):
      exe = self.compile(context, result)
      if not exe: return
      test = RedirectedExecutable()
      status = test.Run([exe])
      if not os.WIFEXITED(status) and os.WEXITSTATUS(status) == 0:
         result.Fail('program exit value : %i'%os.WEXITSTATUS(status))
         if test.stderr: result['orthotest.error'] = test.stderr

      expected = string.join(open(self.expected, 'r').readlines(), '')
      if expected and not test.stdout:
         result.Fail('program did not generate output')
      elif expected and not expected == test.stdout:
         expected = '\'%s\''%(expected)
         output = '\'%s\''%(test.stdout)
         result.Fail('incorrect output',
                     {'synopsis_test.expected': expected,
                      'synopsis_test.output': output})


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

class CxxResource(Resource):
   """build the executables the CxxTests all depend on."""

   arguments = [TextField(name="MAKE", description="The make tool."),
                TextField(name="exe", description="The executable file.")]

   def compile(self, context, result):

      make = os.environ.get('MAKE', 'make')
      command = '%s -C Cxx %s '%(make, self.exe)
      compiler = RedirectedExecutable()
      status = compiler.Run(string.split(command))
      if os.WIFEXITED(status) and os.WEXITSTATUS(status) == 0:
         return self.exe
      else:
         result.Fail('compilation failed',
                     {'synopsis_test.error': compiler.stderr,
                      'synopsis_test.command': command})
         return None      

   def SetUp(self, context, result):

      self.compile(context, result)

class CxxTest(Test):
   """Process an input file with a processor and compare the output.
   If the processor doesn't exist yet, attempt to build it."""

   arguments = [TextField(name="applet", description="The applet."),
                TextField(name="srcdir", description="The source directory."),
                TextField(name="input", description="The input files."),
                TextField(name="output", description="The output files."),
                TextField(name="expected", description="The expected output file.")]

   _ld_paths = re.compile(r'-L\s*\S+')

   def run_applet(self, context, result):

      input = map(lambda x:os.path.join(self.srcdir, x), self.input)
      if not os.path.isdir(os.path.dirname(self.output)):
         os.makedirs(os.path.dirname(self.output))

      # Make sure we see the right libraries during program loading.
      LIBS=context.get('LIBS', '')
      LD_PATHS=[p[2:].strip() for p in CxxTest._ld_paths.findall(LIBS)]
      LD_LIBRARY_PATH = ':'.join(LD_PATHS)
      LD_LIBRARY_PATH += ':' + os.environ.get('LD_LIBRARY_PATH', '')
      os.environ['LD_LIBRARY_PATH'] = LD_LIBRARY_PATH

      test = RedirectedExecutable()
      command = '%s %s %s'%(self.applet, self.output, self.input)
      status = test.Run(command.split())
      if os.WIFSIGNALED(status):
         result.Fail('program killed with signal %i'%os.WTERMSIG(status))
         
      elif os.WIFEXITED(status) and os.WEXITSTATUS(status) != 0:
         result.Fail('program exit value : %i'%os.WEXITSTATUS(status))
         if test.stderr: result['synopsis_test.error'] = test.stderr

      else:
         try:
            expected = string.join(open(self.expected, 'r').readlines(), '')
         except IOError, error:
            result.Fail('error reading expected output',
                        {'synopsis_test.error': error.strerror})
            return
         try:
            output = string.join(open(self.output, 'r').readlines(), '')
         except IOError, error:
            result.Fail('error reading actual output',
                        {'synopsis_test.error': error.strerror})
            return
         
         if expected != output:
            expected = '\'%s\''%(expected)
            output = '\'%s\''%(output)
            result.Fail('incorrect output',
                        {'synopsis_test.expected': expected,
                         'synopsis_test.output': output})

   def Run(self, context, result):

      self.run_applet(context, result)

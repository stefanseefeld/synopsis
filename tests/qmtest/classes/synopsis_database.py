#!/usr/bin/env python
#
# Copyright (C) 2003 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from qm.fields import TextField
from qm.test import database
from qm.test.database import TestDescriptor
from qm.test.database import NoSuchTestError, NoSuchSuiteError
from explicit_suite import ExplicitSuite
from qm.test.suite import Suite

import os, string, dircache

class Database(database.Database):
   """The Database stores the synopsis tests."""

   arguments = [TextField(name="srcdir"),
                TextField(name="CXX"),
                TextField(name="CPPFLAGS"),
                TextField(name="CXXFLAGS"),
                TextField(name="LDFLAGS"),
                TextField(name="LIBS")]

   def GetSuite(self, id):
      """Construct a suite for the given id"""

      if not id:
         path = self.srcdir
      else:
         path = os.path.join(*[self.srcdir] + string.split(id, '.'))

      # make sure this is a suite
      if not (os.path.basename(path) != '.svn' and os.path.isdir(path)):
         raise NoSuchSuiteError, id
      
      # by default every subdirectory that is not named '.svn' is a suite
      def is_suite(x): return x != '.svn' and os.path.isdir(os.path.join(path, x))
      # filter out '.svn' from directory listings
      def listdir(path): return filter(lambda x: x != '.svn', dircache.listdir(path))

      test_ids = []
      suite_ids = []

      if id.startswith('Processors.Linker'):
         # if 'synopsis.py' exists in the subdir, it's a test
         def is_test(x): return x != '.svn' and os.path.isfile(os.path.join(path, x, 'synopsis.py'))
         test_ids = filter(is_test, listdir(path))
         # every other subdir is a suite
         suite_ids = filter(lambda x: is_suite(x) and not x in test_ids, listdir(path))

      elif id.startswith('Cxx-API'):
         # if 'src' exists, it contains the tests
         if os.path.isdir(os.path.join(path, 'src')):
            test_ids = map(lambda x: os.path.splitext(x)[0],
                           filter(lambda x: x.endswith('.cc'), listdir(os.path.join(path, 'src'))))
         else:
            suite_ids = filter(is_suite, listdir(path))

      else:
         if os.path.isfile(os.path.join(*string.split(id, '.') + ['synopsis.py'])):
            test_ids = map(lambda x: os.path.splitext(x)[0], listdir(os.path.join(path, 'input')))
         else:
            suite_ids = filter(is_suite, listdir(path))

      if '.svn' in test_ids: test_ids.remove('.svn')
      if '.svn' in suite_ids: suite_ids.remove('.svn')
      if 'autom4te.cache' in suite_ids: suite_ids.remove('autom4te.cache')

      if id:
         test_ids = map(lambda x: string.join([id, x], '.'), test_ids)
         suite_ids = map(lambda x: string.join([id, x], '.'), suite_ids)
      arguments = {'test_ids' : test_ids, 'suite_ids' : suite_ids}
      return ExplicitSuite(arguments,
                           qmtest_database = self,
                           qmtest_id = id)
        
   def GetTest(self, id):
      """Create a test for the given id. Tests are grouped by type,
      i.e. specific suites contain specific test classes."""

      if not id: raise NoSuchTestError, id
         
      if id.startswith('Processors.Linker'): return self.make_linker_test(id)
      elif id.startswith('Cxx-API'): return self.make_api_test(id)
      else: return self.make_processor_test(id)

   def make_processor_test(self, id):
      """A test id 'a.b.c' corresponds to an input file
      'a/b/input/c.<ext>. Create a ProcessorTest if that
      input file exists, and throw NoSuchTestError otherwise."""

      components = id.split('.')
      dirname = os.path.join(*[self.srcdir] + components[:-1])

      input = os.path.join(dirname, 'input', components[-1])
         
      if components[:-1] == ['Parsers', 'IDL']: input += '.idl'
      elif components[:-1] == ['Parsers', 'Python']: input += '.py'
      elif components[:-1] == ['Parsers', 'C']: input += '.c'
      else: input += '.cc'
      if not os.path.isfile(input): raise NoSuchTestError, id

      output = os.path.join(*components[:-1] + ['output', components[-1] + '.xml'])
      expected = os.path.join(dirname, 'expected', components[-1] + '.xml')
      synopsis = os.path.join(*components[:-1] + ['synopsis.py'])

      parameters = {}
      parameters['srcdir'] = self.srcdir
      parameters['input'] = [input]
      parameters['output'] = output
      parameters['expected'] = expected
      parameters['synopsis'] = synopsis
      
      return TestDescriptor(self, id, 'synopsis_test.ProcessorTest', parameters)

   def make_linker_test(self, id):
      """A test id 'a.b.c' corresponds to an input directory
      'a/b/c/input containing files to be linked together.
      Create a ProcessorTest if that directory exists,
      and throw NoSuchTestError otherwise."""

      path = os.path.join([self.srcdir] + id.split('.'))
      if not os.path.isdir(path): raise NoSuchTestError, id

      dirname = os.path.join(path, 'input')

      parameters = {}
      parameters['srcdir'] = self.srcdir
      parameters['input'] = map(lambda x: os.path.join(dirname, x),
                                dircache.listdir(dirname))
      parameters['output'] = os.path.join(*id.split('.') + ['output.xml'])
      parameters['expected'] = os.path.join(path, 'expected.xml')
      parameters['synopsis'] = os.path.join(*id.split('.') + ['synopsis.py'])
      
      return TestDescriptor(self, id, 'synopsis_test.ProcessorTest', parameters)

   def make_api_test(self, id):
      """A test id 'a.b.c' corresponds to an input file
      'a/b/src/c.cc. Create an APITest if that
      input file exists, and throw NoSuchTestError otherwise."""

      components = id.split('.')
      dirname = os.path.join(*[self.srcdir] + components[:-1])

      if not os.path.exists(os.path.join(dirname, 'src', components[-1]) + '.cc'):
         raise NoSuchTestError, id

      parameters = {}
      parameters['CXX'] = self.CXX
      parameters['CPPFLAGS'] = self.CPPFLAGS
      parameters['CXXFLAGS'] = self.CXXFLAGS
      parameters['LDFLAGS'] = self.LDFLAGS
      parameters['LIBS'] = self.LIBS
      parameters['src'] = os.path.join(dirname, 'src', components[-1]) + '.cc'
      parameters['exe'] = os.path.join(*id.split('.') + ['bin', components[-1]])
      parameters['expected'] = os.path.join(dirname, 'expected',
                                            components[-1] + '.out')
      
      return TestDescriptor(self, id, 'synopsis_test.APITest', parameters)

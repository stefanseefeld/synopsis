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

   arguments = [TextField(name="CXX"),
                TextField(name="CPPFLAGS"),
                TextField(name="CXXFLAGS"),
                TextField(name="LDFLAGS"),
                TextField(name="LIBS")]

   def GetSuite(self, id):
      """Construct a suite for the given id"""

      if not id:
         path = '.'
      else:
         path = os.path.join(*string.split(id, '.'))

      # make sure this is a suite
      if not (os.path.basename(path) != '.svn'
              and os.path.isdir(path)):
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
         if os.path.isfile(os.path.join(path, 'synopsis.py')):
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
      """create a test for the given id."""

      if not id:
         raise NoSuchTestError, id
         
      if id.startswith('Processors.Linker'):
         return self.make_linker_test(id)

      elif id.startswith('Cxx-API'):
         return self.make_api_test(id)

      else:
         return self.make_test(id)

   def make_test(self, id):

      components = id.split('.')
      dirname = os.path.join(*components[:-1])

      input = [os.path.join(dirname, 'input', components[-1])]
         
      if components[:-1] == ['Parsers', 'IDL']:
         input = map(lambda x: x + '.idl', input)
      elif components[:-1] == ['Parsers', 'Python']:
         input = map(lambda x: x + '.py', input)
      else:  # all other tests use C++ input
         input = map(lambda x: x + '.cc', input)
      if reduce(lambda x, y: x + y, # sum all non-files
                filter(lambda x: not os.path.isfile(x), input), 0):
         raise NoSuchTestError, id

      output = os.path.join(dirname, 'output', components[-1] + '.xml')
      expected = os.path.join(dirname, 'expected', components[-1] + '.xml')
      synopsis = os.path.join(dirname, 'synopsis.py')

      parameters = {}
      parameters['srcdir'] = '.'
      parameters['input'] = input
      parameters['output'] = output
      parameters['expected'] = expected
      parameters['synopsis'] = os.path.join(dirname, 'synopsis.py')
      
      return TestDescriptor(self, id, 'synopsis_test.ProcessorTest', parameters)

   def make_linker_test(self, id):

      if not os.path.isdir(id.replace('.', os.sep)):
         raise NoSuchTestError, id

      components = id.split('.')
      dirname = os.path.join(*components + ['input'])

      parameters = {}
      parameters['srcdir'] = '.'
      parameters['input'] = map(lambda x: os.path.join(dirname, x),
                                dircache.listdir(dirname))
      parameters['output'] = os.path.join(*components + ['output.xml'])
      parameters['expected'] = os.path.join(*components + ['expected.xml'])
      parameters['synopsis'] = os.path.join(*components + ['synopsis.py'])
      
      return TestDescriptor(self, id, 'synopsis_test.ProcessorTest', parameters)

   def make_api_test(self, id):

      components = id.split('.')
      dirname = os.path.join(*components[:-1])

      parameters = {}
      parameters['CXX'] = self.CXX
      parameters['CPPFLAGS'] = self.CPPFLAGS
      parameters['CXXFLAGS'] = self.CXXFLAGS
      parameters['LDFLAGS'] = self.LDFLAGS
      parameters['LIBS'] = self.LIBS
      parameters['src'] = os.path.join(dirname, 'src', components[-1]) + '.cc'
      parameters['exe'] = os.path.join(dirname, 'bin', components[-1])
      parameters['expected'] = os.path.join(dirname, 'expected',
                                            components[-1] + '.out')
      
      return TestDescriptor(self, id, 'synopsis_test.APITest', parameters)

#!/usr/bin/env python
# $Id: synopsis_database.py,v 1.4 2004/01/12 20:31:03 stefan Exp $
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
      if not (os.path.basename(path) != 'CVS'
              and os.path.isdir(path)):
         raise NoSuchSuiteError, id
      
      if id.startswith('Processors.Linker'):
         # tests in the Linker suite are treated differently because
         # input files in 'input' are processed together in a single test
         child_ids = filter(lambda x: os.path.isdir(os.path.join(path, x)),
                            dircache.listdir(path))
         # if there is a subdir 'input', we consider this a test
         # else it's a suite
         test_ids = filter(lambda x: os.path.isdir(os.path.join(path, x, 'input')),
                           dircache.listdir(path))
         suite_ids = []

      elif id.startswith('Cxx-API'):
         test_ids = []
         suite_ids = filter(lambda x: os.path.isdir(os.path.join(path, x)),
                            dircache.listdir(path))
         if 'src' in suite_ids:
            test_ids = map(lambda x: os.path.splitext(x)[0],
                           filter(lambda x: x.endswith('.cc'),   
                                  dircache.listdir(os.path.join(path, 'src'))))
            suite_ids.remove('src')
            if 'expected' in suite_ids: suite_ids.remove('expected')

      else:
         test_ids = []
         suite_ids = filter(lambda x: os.path.isdir(os.path.join(path, x)),
                            dircache.listdir(path))
         if 'input' in suite_ids:
            test_ids = map(lambda x: os.path.splitext(x)[0],
                           dircache.listdir(os.path.join(path, 'input')))
            suite_ids.remove('input')
            if 'expected' in suite_ids: suite_ids.remove('expected')

      if 'CVS' in test_ids: test_ids.remove('CVS')
      if 'CVS' in suite_ids: suite_ids.remove('CVS')
      if 'autom4te.cache' in suite_ids: suite_ids.remove('autom4te.cache')

      if id:
         test_ids = map(lambda x: string.join([id, x], '.'), test_ids)
         suite_ids = map(lambda x: string.join([id, x], '.'), suite_ids)
      return Suite(self, id, 0, test_ids, suite_ids)
        
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

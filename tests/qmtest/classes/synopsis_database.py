#!/usr/bin/env python
# $Id: synopsis_database.py,v 1.1 2003/11/29 22:57:06 stefan Exp $
#
# Copyright (C) 2003 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from   qm.test import database
from   qm.test.database import TestDescriptor
from   qm.test.database import NoSuchTestError, NoSuchSuiteError
from   qm.test.suite import Suite

import os, string, dircache

class Database(database.Database):
   """The Database stores the synopsis tests."""

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
      
      test_ids = []
      suite_ids = filter(lambda x: os.path.isdir(os.path.join(path, x)),
                         dircache.listdir(path))
      if 'input' in suite_ids:
         test_ids = map(lambda x: os.path.splitext(x)[0],
                        dircache.listdir(os.path.join(path, 'input')))
         if 'CVS' in test_ids: test_ids.remove('CVS')
         suite_ids.remove('input')
         if 'expected' in suite_ids: suite_ids.remove('expected')
      if 'CVS' in suite_ids: suite_ids.remove('CVS')

      if id:
         test_ids = map(lambda x: string.join([id, x], '.'), test_ids)
         suite_ids = map(lambda x: string.join([id, x], '.'), suite_ids)

      return Suite(self, id, 0, test_ids, suite_ids)
        
   def GetTest(self, id):
      """create a test for the given id."""

      if not id:
         raise NoSuchTestError, id
         
      components = id.split('.')
      dirname = os.path.join(*components[:-1])
      input = os.path.join(dirname, 'input', components[-1])
      if components[:-1] == ['Parsers', 'IDL']: input += '.idl'
      elif components[:-1] == ['Parsers', 'Cxx']: input += '.cc'
      elif components[:-1] == ['Parsers', 'Python']: input += '.py'
      if not os.path.isfile(input):
         raise NoSuchTestError, id

      parameters = {}
      parameters['srcdir'] = '.'
      parameters['input'] = [input]
      parameters['output'] = os.path.join(dirname, 'output', components[-1])
      parameters['expected'] = os.path.join(dirname, 'expected', components[-1])
      parameters['synopsis'] = os.path.join(dirname, 'synopsis.py')
      return TestDescriptor(self, id, 'synopsis_test.ProcessorTest', parameters)


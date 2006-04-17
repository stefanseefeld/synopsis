#!/usr/bin/env python
#
# Copyright (C) 2003 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from qm.fields import TextField
from qm.test import database
from qm.test.database import TestDescriptor, ResourceDescriptor
from qm.test.database import NoSuchTestError, NoSuchSuiteError, NoSuchResourceError
from explicit_suite import ExplicitSuite
from qm.test.suite import Suite

import os, string, dircache

class Database(database.Database):
   """The Database stores the synopsis tests."""

   arguments = [TextField(name="srcdir")]

   def __init__(self, path, arguments):

      arguments["modifiable"] = "false"
      database.Database.__init__(self, path, arguments)
      if os.name == 'nt':
         self.srcdir = os.popen('cygpath -w "%s"'%self.srcdir).read()[:-1]

   def get_src_path(self, id):
      return self.srcdir + os.sep + id.replace('.', os.sep)

   def get_build_path(self, id):
      return id.replace('.', os.sep)

   def get_file_tests(self, path, ext = ''):
      """tests are file names without extension"""
      return [t[0]
              for t in [os.path.splitext(x) for x in dircache.listdir(path)] 
              if t[0] != '' and (not ext or t[1] == ext)]

   def get_dir_tests(self, id, script):
      """tests are directories if <script> exists"""
      return [t for t in dircache.listdir(self.get_src_path(id))
              if os.path.isfile(os.path.join(self.get_build_path(id), t, script))]

   def get_dir_suites(self, id):
      """by default all directories are suites"""
      path = self.get_src_path(id)
      return filter(lambda x: os.path.isdir(os.path.join(path, x)),
                    dircache.listdir(path))

   def GetSubdirectories(self, dir):

      if not os.path.isdir(self.get_src_path(dir)):
         raise NoSuchSuiteError, dir

      suites = []

      if not dir:

         return ['Cxx', 'Parsers', 'Processors']

      elif dir.startswith('Processors.Linker'):

         # just make sure this isn't a test itself...
         if os.path.exists(os.path.join(self.get_build_path(dir), 'synopsis.py')):
            raise NoSuchSuiteError, dir

         tests = self.get_dir_tests(dir, 'synopsis.py')
         suites = [s for s in self.get_dir_suites(dir) if s not in tests]

      elif dir.startswith('Cxx-API'):
         # if 'src' exists, it contains the tests
         path = os.path.join(self.get_src_path(dir), 'src')
         if not os.path.isdir(path):
            suites = self.get_dir_suites(dir)

      elif dir.startswith('Cxx'):
         if not os.path.isdir(os.path.join(self.get_src_path(dir), 'input')):
            suites = self.get_dir_suites(dir)
            suites.remove('src')

      else:
         if not os.path.isdir(os.path.join(self.get_src_path(dir), 'input')):
            suites = self.get_dir_suites(dir)

      # ignore accidental inclusion of false tests / suites
      if '.svn' in suites: suites.remove('.svn')
      if 'autom4te.cache' in suites: suites.remove('autom4te.cache')

      return suites


   def GetTestIds(self, dir = "", scan_subdirs = 1):
      """Return all test IDs that are part of the given suite."""

      if not os.path.isdir(self.get_src_path(dir)):
         raise NoSuchSuiteError, dir

      tests = []

      if dir.startswith('Processors.Linker'):

         # just make sure this isn't a test itself...
         if os.path.exists(os.path.join(self.get_build_path(dir), 'synopsis.py')):
            raise NoSuchSuiteError, dir

         tests = self.get_dir_tests(dir, 'synopsis.py')

      elif dir.startswith('Cxx-API'):
         # if 'src' exists, it contains the tests
         path = os.path.join(self.get_src_path(dir), 'src')
         if os.path.isdir(path):
            tests = self.get_file_tests(path, '.cc')

      elif dir.startswith('Cxx'):
         if os.path.isdir(os.path.join(self.get_src_path(dir), 'input')):
            tests = self.get_file_tests(os.path.join(self.get_src_path(dir),
                                                     'input'),
                                   '.cc')

      else:
         if os.path.isdir(os.path.join(self.get_src_path(dir), 'input')):
            tests = self.get_file_tests(os.path.join(self.get_src_path(dir),
                                                     'input'))
      # ignore accidental inclusion of false tests / suites
      if '.svn' in tests: tests.remove('.svn')

      if dir:
         tests = ['%s.%s'%(dir, t) for t in tests]

      if scan_subdirs:
         for d in self.GetSubdirectories(dir):
            d = self.JoinLabels(dir, d)
            tests.extend(self.GetTestIds(d, scan_subdirs))

      return tests
   
   
   def GetResourceIds(self, directory="", scan_subdirs=1):

      if directory == '' and scan_subdirs or directory == 'Cxx':
         return ['Cxx.Lexer',
                 'Cxx.Parser',
                 'Cxx.Encoding',
                 'Cxx.ConstEvaluator',
                 'Cxx.SymbolLookup',
                 'Cxx.TypeAnalysis']
      else:
         return []

   def GetResource(self, id):
      """Construct a resource for the given id.
      For now the only resources provided are Cxx test applets."""

      if not id.startswith('Cxx'): raise NoSuchResourceError(id)

      parameters = {}

      suite = id.split('.', 1)[1]
      if suite not in ['Lexer', 'Parser', 'Encoding',
                       'ConstEvaluator', 'SymbolLookup', 'TypeAnalysis']:
         raise NoSuchResourceError(id)
      
      parameters['exe'] = os.path.normpath('bin/%s'%suite)

      return ResourceDescriptor(self, id, 'synopsis_test.CxxResource', parameters)
      
   def GetSuite(self, id):
      """Construct a suite for the given id.
      For suites there is a general mapping from ids to paths:
      replace the period by os.sep to get the path in the build tree,
      and prefix that with self.srcdir to get the path in the source tree."""

      test_ids = self.GetTestIds(id)
      suite_ids = self.GetSuiteIds(id)

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
      elif id.startswith('Cxx'): return self.make_opencxx_test(id)
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
      if components[0] == 'Processors' and components[1] != 'Linker':
         synopsis = os.path.join('Processors', 'synopsis.py')
      else:
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

      path = self.get_src_path(id)
      if not os.path.isdir(path): raise NoSuchTestError, id

      dirname = os.path.join(path, 'input')

      parameters = {}
      parameters['srcdir'] = self.srcdir
      parameters['input'] = [os.path.join(dirname, x) for x in dircache.listdir(dirname) if x != '.svn']
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
      parameters['exe'] = os.path.join(*components[:-1] + ['bin', components[-1]])
      parameters['expected'] = os.path.join(dirname, 'expected',
                                            components[-1] + '.out')
      
      return TestDescriptor(self, id, 'synopsis_test.APITest', parameters)

   def make_opencxx_test(self, id):
      """A test id 'a.b.c' corresponds to an input file
      'a/b/input/c.<ext>. Create an CxxTest if that
      input file exists, and throw NoSuchTestError otherwise."""

      base, suite, test = id.split('.')
      dirname = os.path.join(*[self.srcdir] + [base, suite])

      input = os.path.join(dirname, 'input', test) + '.cc'
      output = os.path.join(*[base, suite, 'output', test + '.out'])
         
      if not os.path.isfile(input): raise NoSuchTestError, id

      expected = os.path.join(dirname, 'expected', test + '.out')

      parameters = {}
      parameters['input'] = input
      parameters['output'] = output
      parameters['expected'] = expected

      parameters['resources'] = ['Cxx.%s'%suite]
      parameters['applet'] = 'Cxx/bin/%s'%suite
         
      return TestDescriptor(self, id, 'synopsis_test.CxxTest', parameters)


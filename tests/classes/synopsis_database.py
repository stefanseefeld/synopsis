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

   arguments = [TextField(name="srcdir"),
                TextField(name="CXX"),
                TextField(name="CPPFLAGS"),
                TextField(name="CXXFLAGS"),
                TextField(name="LDFLAGS"),
                TextField(name="LIBS")]

   def __init__(self, *args, **kwrds):
      database.Database.__init__(self, *args, **kwrds)
      if os.name == 'nt':
         self.srcdir = os.popen('cygpath -w "%s"'%self.srcdir).read()[:-1]

   def get_src_path(self, id) : return self.srcdir + os.sep + id.replace('.', os.sep)
   def get_build_path(self, id) : return id.replace('.', os.sep)

   def GetResource(self, id):
      """Construct a resource for the given id.
      For now the only resources provided are OpenCxx test applets."""

      if not id.startswith('OpenCxx'): raise NoSuchResourceError(id)

      parameters = {}
      parameters['CXX'] = self.CXX
      parameters['CPPFLAGS'] = (self.CPPFLAGS +
                                ' -I../Synopsis/Parsers/Cxx/occ' +
                                ' -I%s/../Synopsis/Parsers/Cxx/occ'%self.srcdir)
      parameters['CXXFLAGS'] = self.CXXFLAGS
      parameters['LDFLAGS'] = self.LDFLAGS
      parameters['LIBS'] = self.LIBS
      parameters['builddir'] = '../Synopsis/Parsers/Cxx'


      if id == 'OpenCxx.Lexer':
         parameters['src'] = os.path.normpath('%s/OpenCxx/src/Lexer.cc'%self.srcdir)
         parameters['exe'] = os.path.normpath('OpenCxx/bin/Lexer')
      elif id == 'OpenCxx.Parser':
         parameters['src'] = os.path.normpath('%s/OpenCxx/src/Parser.cc'%self.srcdir)
         parameters['exe'] = os.path.normpath('OpenCxx/bin/Parser')
      elif id == 'OpenCxx.ConstEvaluator':
         parameters['src'] = os.path.normpath('%s/OpenCxx/src/ConstEvaluator.cc'%self.srcdir)
         parameters['exe'] = os.path.normpath('OpenCxx/bin/ConstEvaluator')
      elif id == 'OpenCxx.Encoding':
         parameters['src'] = os.path.normpath('%s/OpenCxx/src/Encoding.cc'%self.srcdir)
         parameters['exe'] = os.path.normpath('OpenCxx/bin/Encoding')
      else: raise NoSuchResourceError(id)

      return ResourceDescriptor(self, id, 'synopsis_test.OpenCxxResource', parameters)
      
   def GetSuite(self, id):
      """Construct a suite for the given id.
      For suites there is a general mapping from ids to paths:
      replace the period by os.sep to get the path in the build tree,
      and prefix that with self.srcdir to get the path in the source tree."""

      def get_dir_suites(id):
         """by default all directories are suites"""
         path = self.get_src_path(id)
         return filter(lambda x: os.path.isdir(os.path.join(path, x)),
                       dircache.listdir(path))

      def get_file_tests(path, ext = ''):
         """tests are file names without extension"""
         return [t[0] for t in map(lambda x:os.path.splitext(x), dircache.listdir(path)) 
                 if t[0] != '' and not ext or t[1] == ext] # splitext('.svn') == ('','svn') !!

      def get_dir_tests(id, script):
         """tests are directories if <script> exists"""
         return [t for t in dircache.listdir(self.get_src_path(id))
                 if os.path.isfile(os.path.join(self.get_build_path(id), t, script))]

      if not os.path.isdir(self.get_src_path(id)): raise NoSuchSuiteError, id

      test_ids = []
      suite_ids = []

      if id.startswith('Processors.Linker'):

         # just make sure this isn't a test itself...
         if os.path.exists(os.path.join(self.get_build_path(id), 'synopsis.py')):
            raise NoSuchSuiteError, id

         test_ids = get_dir_tests(id, 'synopsis.py')
         suite_ids = [s for s in get_dir_suites(id) if s not in test_ids]

      elif id.startswith('Cxx-API'):
         # if 'src' exists, it contains the tests
         path = os.path.join(self.get_src_path(id), 'src')
         if os.path.isdir(path):
            test_ids = get_file_tests(path, '.cc')
         else:
            suite_ids = get_dir_suites(id)

      elif id.startswith('CTool'):
         if os.path.isfile(os.path.join(self.get_build_path(id), 'ctool.py')):
            test_ids = get_file_tests(os.path.join(self.get_src_path(id), 'input'), '.c')
         else:
            suite_ids = get_dir_suites(id)

      elif id.startswith('OpenCxx'):
         if os.path.isdir(os.path.join(self.get_src_path(id), 'input')):
            test_ids = get_file_tests(os.path.join(self.get_src_path(id), 'input'), '.cc')
         else:
            suite_ids = get_dir_suites(id)

      else:
         if os.path.isfile(os.path.join(self.get_build_path(id), 'synopsis.py')):
            test_ids = get_file_tests(os.path.join(self.get_src_path(id), 'input'))
         else:
            suite_ids = get_dir_suites(id)

      # ignore accidental inclusion of false tests / suites
      if '.svn' in test_ids: test_ids.remove('.svn')
      if '.svn' in suite_ids: suite_ids.remove('.svn')
      if 'autom4te.cache' in suite_ids: suite_ids.remove('autom4te.cache')

      if id:
         test_ids = ['%s.%s'%(id, t) for t in test_ids]
         suite_ids = ['%s.%s'%(id, s) for s in suite_ids]

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
      elif id.startswith('CTool'): return self.make_ctool_test(id)
      elif id.startswith('OpenCxx'): return self.make_opencxx_test(id)
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

   def make_ctool_test(self, id):
      """A test id 'a.b.c' corresponds to an input file
      'a/b/input/c.<ext>. Create a CToolTest if that
      input file exists, and throw NoSuchTestError otherwise."""

      components = id.split('.')
      dirname = os.path.join(*[self.srcdir] + components[:-1])

      input = os.path.join(dirname, 'input', components[-1]) + '.c'
         
      if not os.path.isfile(input): raise NoSuchTestError, id

      output = os.path.join(*components[:-1] + ['output', components[-1] + '.out'])
      expected = os.path.join(dirname, 'expected', components[-1] + '.ans')
      ctool = os.path.join(*components[:-1] + ['ctool.py'])

      parameters = {}
      parameters['srcdir'] = self.srcdir
      parameters['input'] = [input]
      parameters['output'] = output
      parameters['expected'] = expected
      parameters['ctool'] = ctool
      
      return TestDescriptor(self, id, 'synopsis_test.CToolTest', parameters)

   def make_opencxx_test(self, id):
      """A test id 'a.b.c' corresponds to an input file
      'a/b/input/c.<ext>. Create an OpenCxxTest if that
      input file exists, and throw NoSuchTestError otherwise."""

      components = id.split('.')
      dirname = os.path.join(*[self.srcdir] + components[:-1])

      input = os.path.join(dirname, 'input', components[-1]) + '.cc'
      output = os.path.join(*components[:-1] + ['output', components[-1] + '.out'])
         
      if not os.path.isfile(input): raise NoSuchTestError, id

      expected = os.path.join(dirname, 'expected', components[-1] + '.out')

      parameters = {}
      parameters['input'] = input
      parameters['output'] = output
      parameters['expected'] = expected
      if id.startswith('OpenCxx.Lexer'):
         parameters['resources'] = ['OpenCxx.Lexer']
         parameters['applet'] = 'OpenCxx/bin/Lexer'
      elif id.startswith('OpenCxx.Parser'):
         parameters['resources'] = ['OpenCxx.Parser']
         parameters['applet'] = 'OpenCxx/bin/Parser'
      elif id.startswith('OpenCxx.ConstEvaluator'):
         parameters['resources'] = ['OpenCxx.ConstEvaluator']
         parameters['applet'] = 'OpenCxx/bin/ConstEvaluator'
      elif id.startswith('OpenCxx.Encoding'):
         parameters['resources'] = ['OpenCxx.Encoding']
         parameters['applet'] = 'OpenCxx/bin/Encoding'
         
      return TestDescriptor(self, id, 'synopsis_test.OpenCxxTest', parameters)


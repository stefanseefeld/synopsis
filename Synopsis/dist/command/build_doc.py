#
# Copyright (C) 2003 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

import os, sys, string, re, stat
from stat import *
import os.path
from shutil import *
import glob

from distutils.command import build
from distutils.spawn import spawn, find_executable
from distutils.dep_util import newer, newer_group
from distutils.dir_util import copy_tree, remove_tree, mkpath
from distutils.file_util import copy_file
from distutils import sysconfig

class build_doc(build.build):
   """Defines the specific procedure to build synopsis' documentation."""

   description = "build documentation"
   user_options = [('manual', 'm', "build the manual only"),
                   ('tutorial', 't', "build the tutorial only"),
                   ('html', 'h', "build for html output only"),
                   ('printable', 'p', "build for pdf output only"),
                   ('sxr=', 'x', "build the sxr database for synopsis for the given URL (requires -m)")]
   boolean_options = ['manual', 'tutorial', 'html', 'print']

   def initialize_options (self):

      build.build.initialize_options(self)
      self.manual = False
      self.tutorial = False
      self.html = False
      self.printable = False
      self.sxr = ''

   def finalize_options (self):

      if not (self.manual or self.tutorial): # if no option was given, do both
         self.manual = self.tutorial = True
      if not (self.html or self.printable): # if no option was given, do both
         self.html = self.printable = True
      build.build.finalize_options(self)

   def run(self):
      """Run this command, i.e. do the actual document generation."""

      if not os.path.exists(self.build_temp):
         self.run_command('config')

      self.build_lib = '.'

      if self.manual or self.sxr: self.build_manual()
      if self.tutorial: self.build_tutorial()
    
   def build_manual(self):
      """Build the manual."""
        
      self.announce("building reference manual")

      tmp_man_dir = os.path.abspath(os.path.join(self.build_temp,
                                                 'doc/Manual'))

      make = os.environ.get('MAKE', 'make')

      build_clib = self.distribution.get_command_obj('build_clib')
      build_clib.ensure_finalized()

      tmpdir = os.path.join(build_clib.build_ctemp, 'src')

      # build the 'doc' target
      spawn([make, '-C', tmpdir, 'doc'])

      for d in ['cxx.syn', 'cxx-impl.syn']:
         src, dest = os.path.join(tmpdir, d), os.path.join(tmp_man_dir, d)
         if os.path.exists(src) and newer(src, dest):
            copy_file(src, dest)
         else:
            print 'not copying', src
      for d in ['links', 'xref']:
         src, dest = os.path.join(tmpdir, d), os.path.join(tmp_man_dir, d)
         if os.path.exists(src) and newer(src, dest):
            copy_tree(src, dest)
         else:
            print 'not copying', src
         
      # now run make inside doc/Manual to do the rest

      srcdir = os.path.abspath('doc/Manual/')

      cwd = os.getcwd()
      mkpath(tmp_man_dir, 0777, self.verbose, self.dry_run)

      if self.html:
         spawn([make, '-C', tmp_man_dir, 'html'])
      if self.printable:
         spawn([make, '-C', tmp_man_dir, 'pdf'])
      if self.sxr:
         spawn([make, '-C', tmp_man_dir, 'sxr', 'sxr=%s'%self.sxr])

      builddir = os.path.abspath(os.path.join(self.build_lib,
                                              'share/doc/Synopsis/html/Manual'))
      for d in ['python', 'cxx', 'sxr']:
         src = os.path.join(tmp_man_dir, 'html', d)
         dest = os.path.join(builddir, d)

         if newer(src, dest):
            rmtree(dest, True)
            copy_tree(src, dest)

      if self.printable:
         builddir = os.path.abspath(os.path.join(self.build_lib,
                                                 'share/doc/Synopsis/print'))
         mkpath(builddir, 0777, self.verbose, self.dry_run)
         copy_file(os.path.join(tmp_man_dir, 'Manual.pdf'),
                   os.path.join(builddir, 'Manual.pdf'))

   def build_tutorial(self):
      """Build the tutorial."""

      self.announce("building tutorial")
      srcdir = os.path.abspath('doc/Tutorial/')
      tempdir = os.path.abspath(os.path.join(self.build_temp,
                                             'doc/Tutorial'))
      cwd = os.getcwd()
      mkpath(tempdir, 0777, self.verbose, self.dry_run)

      make = os.environ.get('MAKE', 'make')

      if self.html:
         spawn([make, '-C', tempdir, 'html'])
      if self.printable:
         spawn([make, '-C', tempdir, 'pdf'])

      if self.html:
         builddir = os.path.abspath(os.path.join(self.build_lib,
                                                 'share/doc/Synopsis'))
         if os.path.isdir(os.path.join(builddir, 'html', 'Tutorial')):
            rmtree(os.path.join(builddir, 'html', 'Tutorial'), 1)
         copy_tree(os.path.join(tempdir, 'html'),
                   os.path.join(builddir, 'html', 'Tutorial'))

         if os.path.isdir(os.path.join(builddir, 'examples')):
            rmtree(os.path.join(builddir, 'examples'), 1)
         copy_tree(os.path.join(tempdir, 'examples'),
                   os.path.join(builddir, 'examples'))

      if self.printable:
         builddir = os.path.abspath(os.path.join(self.build_lib,
                                                 'share/doc/Synopsis/print'))
         mkpath(builddir, 0777, self.verbose, self.dry_run)
         copy_file(os.path.join(tempdir, 'Tutorial.pdf'),
                   os.path.join(builddir, 'Tutorial.pdf'))


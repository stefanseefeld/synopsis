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
                   ('printable', 'p', "build for pdf output only")]
   boolean_options = ['manual', 'tutorial', 'html', 'print']

   def initialize_options (self):

      build.build.initialize_options(self)
      self.manual = False
      self.tutorial = False
      self.html = False
      self.printable = False

   def finalize_options (self):

      if not (self.manual or self.tutorial): # if no option was given, do both
         self.manual = self.tutorial = True
      if not (self.html or self.printable): # if no option was given, do both
         self.html = self.printable = True
      build.build.finalize_options(self)

      self.extensions = self.distribution.ext_modules
         
   def run(self):
      """Run this command, i.e. do the actual document generation."""

      self.build_lib = '.'

      if self.manual: self.build_manual()
      if self.tutorial: self.build_tutorial()
    
   def build_manual(self):
      """Build the manual."""
        
      self.announce("building reference manual")

      tempdir = os.path.abspath(os.path.join(self.build_temp,
                                             'doc/Manual'))

      # start by building the docs for all the extensions

      build_ext = self.distribution.get_command_obj('build_ext')
      build_ext.ensure_finalized()

      for e in self.extensions:
         # replace 'foo.so' by 'foo.syn' as the new target
         e = e[0], os.path.splitext(e[1])[0] + '.syn'
         build_ext.build_extension(e, False)

         if os.path.exists(os.path.join(build_ext.build_temp, e[0], e[1])):
            copy_file(os.path.join(build_ext.build_temp, e[0], e[1]),
                      os.path.join(tempdir, e[1]))
            copy_tree(os.path.join(build_ext.build_temp, e[0], 'links'),
                      os.path.join(tempdir, 'links'))
            copy_tree(os.path.join(build_ext.build_temp, e[0], 'xref'),
                      os.path.join(tempdir, 'xref'))
         
      # now run make inside doc/Manual to do the rest

      srcdir = os.path.abspath('doc/Manual/')

      cwd = os.getcwd()
      mkpath(tempdir, 0777, self.verbose, self.dry_run)

      make = os.environ.get('MAKE', 'make')

      if self.html:
         #spawn([make, '-s', '-f', srcdir + '/Makefile', '-C', tempdir,
         #       'srcdir=%s'%srcdir, 'topdir=%s'%cwd, 'html'])
         spawn([make, '-C', tempdir, 'html'])
      if self.printable:
         spawn([make, '-C', tempdir, 'pdf'])
         #spawn([make, '-s', '-f', srcdir + '/Makefile', '-C', tempdir,
         #       'srcdir=%s'%srcdir, 'topdir=%s'%cwd, 'pdf'])

      builddir = os.path.abspath(os.path.join(self.build_lib,
                                              'share/doc/Synopsis'))
      if os.path.isdir(os.path.join(builddir, 'html', 'Manual')):
         rmtree(os.path.join(builddir, 'html', 'Manual'), 1)
      mkpath(os.path.join(builddir, 'html'), 0777, self.verbose, self.dry_run)
      copy_tree(os.path.join(tempdir, 'html'),
                os.path.join(builddir, 'html', 'Manual'))
      copy_file(os.path.join(tempdir, 'all.xref'),
                os.path.join(builddir, 'html', 'Manual', 'xref_db'))
      if self.printable:
         mkpath(os.path.join(builddir, 'print'), 0777, self.verbose, self.dry_run)
         copy_file(os.path.join(tempdir, 'Manual.pdf'),
                   os.path.join(builddir, 'print', 'Manual.pdf'))

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
         spawn([make, '-s', '-f', srcdir + '/Makefile', '-C', tempdir,
                'srcdir=%s'%srcdir, 'topdir=%s'%cwd, 'html'])
      if self.printable:
         spawn([make, '-s', '-f', srcdir + '/Makefile', '-C', tempdir,
                'srcdir=%s'%srcdir, 'topdir=%s'%cwd, 'pdf'])

      builddir = os.path.abspath(os.path.join(self.build_lib,
                                              'share/doc/Synopsis'))
      if self.html:
         if os.path.isdir(os.path.join(builddir, 'html', 'Tutorial')):
            rmtree(os.path.join(builddir, 'html', 'Tutorial'), 1)
         mkpath(os.path.join(builddir, 'html'), 0777, self.verbose, self.dry_run)
         copy_tree(os.path.join(tempdir, 'html'),
                   os.path.join(builddir, 'html', 'Tutorial'))
      if self.printable:
         mkpath(os.path.join(builddir, 'print'), 0777, self.verbose, self.dry_run)
         copy_file(os.path.join(tempdir, 'Tutorial.pdf'),
                   os.path.join(builddir, 'print', 'Tutorial.pdf'))


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

      self.extensions = self.distribution.ext_modules
         
   def run(self):
      """Run this command, i.e. do the actual document generation."""

      self.build_lib = '.'

      if self.manual or self.sxr: self.build_manual()
      if self.tutorial: self.build_tutorial()
    
   def build_manual(self):
      """Build the manual."""
        
      self.announce("building reference manual")

      tempdir = os.path.abspath(os.path.join(self.build_temp,
                                             'doc/Manual'))

      # start by building the docs for all the extensions

      build_ext = self.distribution.get_command_obj('build_ext')
      build_ext.ensure_finalized()

      for e in [('Cxx-API', 'cxx-api')] + self.extensions:

         # replace <name>.so by <name>.syn
         api = os.path.splitext(e[1])[0] + '.syn'
         all = os.path.splitext(e[1])[0] + '-impl.syn'

         # build the 'doc' target
         e = e[0], 'doc'
         build_ext.build_extension(e, False)

         if (os.path.exists(os.path.join(build_ext.build_temp, e[0], api))
             and newer(os.path.join(build_ext.build_temp, e[0], api),
                       os.path.join(tempdir, api))):
            copy_file(os.path.join(build_ext.build_temp, e[0], api),
                      os.path.join(tempdir, api))
            copy_tree(os.path.join(build_ext.build_temp, e[0], 'links'),
                      os.path.join(tempdir, 'links'))
            copy_tree(os.path.join(build_ext.build_temp, e[0], 'xref'),
                      os.path.join(tempdir, 'xref'))
         if (os.path.exists(os.path.join(build_ext.build_temp, e[0], all))
             and newer(os.path.join(build_ext.build_temp, e[0], all),
                       os.path.join(tempdir, all))):
            copy_file(os.path.join(build_ext.build_temp, e[0], all),
                      os.path.join(tempdir, all))
         
      # now run make inside doc/Manual to do the rest

      srcdir = os.path.abspath('doc/Manual/')

      cwd = os.getcwd()
      mkpath(tempdir, 0777, self.verbose, self.dry_run)

      make = os.environ.get('MAKE', 'make')

      if self.html:
         spawn([make, '-C', tempdir, 'html'])
      if self.printable:
         spawn([make, '-C', tempdir, 'pdf'])
      if self.sxr:
         spawn([make, '-C', tempdir, 'sxr', 'sxr=%s'%self.sxr])

      builddir = os.path.abspath(os.path.join(self.build_lib,
                                              'share/doc/Synopsis/html/Manual'))
      if newer(os.path.join(tempdir, 'html', 'python'),
               os.path.join(builddir, 'python')):
         rmtree(os.path.join(builddir, 'python'), 1)
         copy_tree(os.path.join(tempdir, 'html', 'python'),
                   os.path.join(builddir, 'python'))
      if newer(os.path.join(tempdir, 'html', 'cxx-api'),
               os.path.join(builddir, 'cxx-api')):
         rmtree(os.path.join(builddir, 'cxx-api'), 1)
         copy_tree(os.path.join(tempdir, 'html', 'cxx-api'),
                   os.path.join(builddir, 'cxx-api'))
      if newer(os.path.join(tempdir, 'html', 'ucpp'),
               os.path.join(builddir, 'ucpp')):
         rmtree(os.path.join(builddir, 'ucpp'), 1)
         copy_tree(os.path.join(tempdir, 'html', 'ucpp'),
                   os.path.join(builddir, 'ucpp'))
      if newer(os.path.join(tempdir, 'html', 'ctool'),
               os.path.join(builddir, 'ctool')):
         rmtree(os.path.join(builddir, 'ctool'), 1)
         copy_tree(os.path.join(tempdir, 'html', 'ctool'),
                   os.path.join(builddir, 'ctool'))
      if newer(os.path.join(tempdir, 'html', 'occ'),
               os.path.join(builddir, 'occ')):
         rmtree(os.path.join(builddir, 'occ'), 1)
         copy_tree(os.path.join(tempdir, 'html', 'occ'),
                   os.path.join(builddir, 'occ'))
      if self.sxr and newer(os.path.join(tempdir, 'html', 'sxr'),
               os.path.join(builddir, 'sxr')):
         rmtree(os.path.join(builddir, 'sxr'), 1)
         copy_tree(os.path.join(tempdir, 'html', 'sxr'),
                   os.path.join(builddir, 'sxr'))

      if self.printable:
         builddir = os.path.abspath(os.path.join(self.build_lib,
                                                 'share/doc/Synopsis/print'))
         mkpath(builddir, 0777, self.verbose, self.dry_run)
         copy_file(os.path.join(tempdir, 'Manual.pdf'),
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


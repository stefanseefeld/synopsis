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
         
   def run(self):
      """Run this command, i.e. do the actual document generation."""

      self.build_lib = '.'

      if self.manual: self.build_manual()
      if self.tutorial: self.build_tutorial()
    
   def build_manual(self):
      """Build the manual."""
        
      self.announce("building reference manual")
      srcdir = os.path.abspath('doc/Manual/')
      tempdir = os.path.abspath(os.path.join(self.build_temp,
                                             'share/doc/Synopsis/Manual'))
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
      if os.path.isdir(os.path.join(builddir, 'html', 'Manual')):
         rmtree(os.path.join(builddir, 'html', 'Manual'), 1)
      mkpath(os.path.join(builddir, 'html'), 0777, self.verbose, self.dry_run)
      copytree(os.path.join(tempdir, 'html'),
               os.path.join(builddir, 'html', 'Manual'))
      if self.printable:
         mkpath(os.path.join(builddir, 'print'), 0777, self.verbose, self.dry_run)
         copy_file(os.path.join(tempdir, 'Manual.pdf'),
                   os.path.join(builddir, 'print', 'Manual.pdf'))

   def build_tutorial(self):
      """Build the tutorial."""

      self.announce("building tutorial")
      srcdir = os.path.abspath('doc/Tutorial/')
      tempdir = os.path.abspath(os.path.join(self.build_temp,
                                             'share/doc/Synopsis/Tutorial'))
      cwd = os.getcwd()
      mkpath(tempdir, 0777, self.verbose, self.dry_run)
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
         copytree(os.path.join(tempdir, 'html'),
                  os.path.join(builddir, 'html', 'Tutorial'))
      if self.printable:
         mkpath(os.path.join(builddir, 'print'), 0777, self.verbose, self.dry_run)
         copy_file(os.path.join(tempdir, 'Tutorial.pdf'),
                   os.path.join(builddir, 'print', 'Tutorial.pdf'))


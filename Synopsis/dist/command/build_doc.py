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

   def run(self):
      """Run this command, i.e. do the actual document generation."""

      #self.manual()
      self.tutorial()
    
   def manual(self):
      """Build the manual."""
        
      self.announce("building reference manual")
      srcdir = os.path.abspath('doc/Manual/')
      tempdir = os.path.abspath(os.path.join(self.build_temp,
                                               'share/doc/Synopsis/Manual'))
      cwd = os.getcwd()
      mkpath(tempdir, 0777, self.verbose, self.dry_run)
      spawn(['make', '-s', '-f', srcdir + '/Makefile', '-C', tempdir,
             'srcdir=%s'%srcdir, 'topdir=%s'%cwd])

      builddir = os.path.abspath(os.path.join(self.build_lib,
                                              'share/doc/Synopsis'))
      if os.path.isdir(os.path.join(builddir, 'html', 'Manual')):
         rmtree(os.path.join(builddir, 'html', 'Manual'), 1)
      mkpath(os.path.join(builddir, 'html'), 0777, self.verbose, self.dry_run)
      copytree(os.path.join(tempdir, 'html'),
               os.path.join(builddir, 'html', 'Manual'))

   def tutorial(self):
      """Build the tutorial."""

      srcdir = os.path.abspath('doc/Tutorial/')
      tempdir = os.path.abspath(os.path.join(self.build_temp,
                                             'share/doc/Synopsis/Tutorial'))
      builddir = os.path.abspath(os.path.join(self.build_lib,
                                              'share/doc/Synopsis/print'))

      if os.path.isdir(os.path.join(builddir, 'html', 'Tutorial')):
         rmtree(os.path.join(builddir, 'html', 'Tutorial'), 1)

      xsltproc = find_executable('xsltproc')
      if not xsltproc:
         self.announce("cannot build html tutorial without 'xsltproc'")
      else:
         self.announce("building html tutorial")
         command = [xsltproc, '--novalid',
                    '-o', os.path.join(tempdir, 'html'),
                    os.path.join(srcdir, 'html.xsl'),
                    os.path.join(srcdir, 'synopsis.xml')]
         if self.verbose:
            print string.join(command)
         if not self.dry_run:
            mkpath(tempdir, 0777, self.verbose, self.dry_run)
            spawn(command)
            copytree(os.path.join(tempdir, 'html'),
                     os.path.join(builddir, 'html', 'Tutorial'))

      docbook2pdf = find_executable('docbook2pdf')
      if not docbook2pdf:
         self.announce("cannot build pdf docs without 'docbook2pdf'")
      else:
         self.announce("building pdf manual")
         command = [docbook2pdf, '-o', os.path.join(tempdir, 'synopsis.pdf'),
                    os.path.join(srcdir, 'synopsis.xml')]
         if self.verbose:
            print string.join(command)
         if not self.dry_run:
            spawn(command)
         copy_file(os.path.join(tempdir, 'synopsis.pdf'), builddir)


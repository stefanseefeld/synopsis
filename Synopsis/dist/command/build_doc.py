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

      self.manual()
    
   def manual(self):
      """Build the manual."""
        
      self.announce("building reference manual")
      srcdir = os.path.abspath('doc/Manual/')
      tempdir = os.path.abspath(os.path.join(self.build_temp,
                                               'share/doc/Synopsis/Manual'))
      cwd = os.getcwd()
      mkpath(tempdir, 0777, self.verbose, self.dry_run)
      spawn(['make', '-s', '-f', srcdir + '/Makefile', '-C', tempdir,
             'script="%s/synopsis.py"'%srcdir, 'topdir=%s'%cwd])

      builddir = os.path.abspath(os.path.join(self.build_lib,
                                              'share/doc/Synopsis'))
      if os.path.isdir(os.path.join(builddir, 'html', 'Manual')):
         rmtree(os.path.join(builddir, 'html', 'Manual'), 1)
      mkpath(builddir, 0777, self.verbose, self.dry_run)
      copytree(os.path.join(tempdir, 'html'), os.path.join(builddir, 'html', 'Manual'))

   def tutorial(self):

      xsltproc = find_executable('xsltproc')
      if not xsltproc:
         self.announce("cannot build html tutorial without 'xsltproc'")
         return
      self.announce("building html tutorial")
      srcdir = os.path.abspath('doc/Tutorial/')
      tempdir = os.path.abspath(os.path.join(self.build_temp, 'share/doc/Synopsis'))
      builddir = os.path.abspath(os.path.join(self.build_lib, 'share/doc/Synopsis'))
      cwd = os.getcwd()
      mkpath(tempdir, 0777, self.verbose, self.dry_run)
      os.chdir(tempdir)
      spawn([xmlto, '--skip-validation', '-o', 'html',
             '-m', os.path.join(srcdir, 'synopsis-html.xsl'),
             'html', os.path.join(srcdir, 'synopsis.xml')])

      mkpath(builddir, 0777, self.verbose, self.dry_run)
      if os.path.isdir(os.path.join(builddir, 'html')):
         rmtree(os.path.join(builddir, 'html'), 1)
      copytree(os.path.join(tempdir, 'html'), os.path.join(builddir, 'html'))

      docbook2pdf = find_executable('docbook2pdf')
      if not docbook2pdf:
         self.announce("cannot build pdf docs without 'docbook2pdf'")
         return
      self.announce("building pdf manual")
      spawn([docbook2pdf, os.path.join(srcdir, 'synopsis.xml')])
      copy2('synopsis.pdf', builddir)
      os.chdir(cwd)

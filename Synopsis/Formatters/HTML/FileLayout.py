# $Id: FileLayout.py,v 1.23 2003/11/14 14:51:09 stefan Exp $
#
# Copyright (C) 2000 Stephen Davies
# Copyright (C) 2000 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

"""
FileLayout module. This is the base class for the FileLayout interface. The
default implementation stores everything in the same directory.
"""

from Synopsis import Util, AST
from Synopsis.Formatters import TOC
from core import config
from Tags import *

import os, os.path, sys, stat, string, re

class FileLayout (TOC.Linker):
   """Base class for naming files.
   You may derive from this class an
   reimplement any methods you like. The default implementation stores
   everything in the same directory (specified by -o)."""

   def init(self, processor):

      self.processor = processor
      basename = self.processor.output
      stylesheet = config.stylesheet
      stylesheet_file = config.stylesheet_file
      if basename is None:
         print "ERROR: No output directory specified."
         print "You must specify a base directory with the -o option."
         sys.exit(1)
      if not os.path.exists(basename):
         if config.verbose: print "Warning: Output directory does not exist. Creating."
         try:
            os.makedirs(basename, 0755)
         except EnvironmentError, reason:
            print "ERROR: Creating directory:",reason
            sys.exit(2)
      if stylesheet_file:
         # Copy stylesheet in
         self.copyFile(stylesheet_file, os.path.join(basename, stylesheet))
      if not os.path.isdir(basename):
         print "ERROR: Output must be a directory."
         sys.exit(1)

   def copyFile(self, orig_name, new_name):
      """Copies file if newer. The file named by orig_name is compared to
      new_name, and if newer or new_name doesn't exist, it is copied."""

      try:
         filetime = os.stat(orig_name)[stat.ST_MTIME]
         if not os.path.exists(new_name) or \
		filetime > os.stat(new_name)[stat.ST_MTIME]:
            fin = open(orig_name,'r')
            fout = Util.open(new_name)
            fout.write(fin.read())
            fin.close()
            fout.close()
      except EnvironmentError, reason:
         import traceback
         traceback.print_exc()
         print "ERROR: ", reason
         sys.exit(2)
    
   def _checkMain(self, filename):
      """Checks whether the given filename is the main index page or not. If
      it is, then it returns the filename from nameOfIndex(), else it returns
      it unchanged"""

      if filename == config.page_main: return self.nameOfIndex()
      return filename
	
   def nameOfScope(self, scope):
      """Return the filename of a scoped name (class or module).
      The default implementation is to join the names with '-' and append
      ".html" Additionally, special characters are Util.quoted in URL-style"""

      if not len(scope): return self._checkMain(self.nameOfSpecial('global'))
      return Util.quote(string.join(scope,'-')) + ".html"

   def _stripFilename(self, file):

      if len(file) and file[-1] == '/': file = file[:-1]
      return file

   def nameOfFileIndex(self, file):
      """Return the filename for the index of an input file.
      The base_dir config option is used.
      Default implementation is to join the path with '-', prepend "_file-"
      and append ".html" """

      file = self._stripFilename(file)
      return "_file-"+rel(config.base_dir, file).replace(os.sep, '-')+".html"

   def nameOfFileSource(self, file):
      """Return the filename for the source of an input file.
      The base_dir config option is used.
      Default implementation is to join the path with '-', prepend "_source-"
      and append ".html" """

      file = self._stripFilename(file)
      return "_source-"+rel(config.base_dir, file).replace(os.sep, '-')+".html"

   def nameOfFileDetails(self, file):
      """Return the filename for the details of an input file.
      The base_dir config option is used.
      Default implementation is to join the path with '-', prepend
      "_filedetail-" and append ".html" """

      file = self._stripFilename(file)
      return "_filedetail-"+rel(config.base_dir, file).replace(os.sep, '-')+".html"

   def nameOfIndex(self):
      """Return the name of the main index file. Default is index.html"""

      return "index.html"

   def nameOfSpecial(self, name):
      """Return the name of a special file (tree, etc). Default is
      _name.html"""

      return self._checkMain("_" + name + ".html")

   def nameOfScopedSpecial(self, name, scope, ext=".html"):
      """Return the name of a special type of scope file. Default is to join
      the scope with '-' and prepend '-'+name"""

      return "_%s-%s%s"%(name, Util.quote(string.join(scope, '-')), ext)

   def nameOfModuleTree(self):
      """Return the name of the module tree index. Default is
      _modules.html"""

      return "_modules.html"

   def nameOfModuleIndex(self, scope):
      """Return the name of the index of the given module. Default is to
      join the name with '-', prepend "_module_" and append ".html" """

      # Prefer module index for frames
      if config.using_module_index:
         return "_module_" + Util.quote(string.join(scope, '-')) + ".html"
      # Fall back to the scope page
      return self.nameOfScope(scope)
	
   def link(self, decl):
      """Create a link to the named declaration. This method may have to
      deal with the directory layout."""

      if isinstance(decl, AST.Scope):
         # This is a class or module, so it has its own file
         return self.nameOfScope(decl.name())
      # Assume parent scope is class or module, and this is a <A> name in it
      filename = self.nameOfScope(decl.name()[:-1])
      anchor = anglebrackets(decl.name()[-1].replace(' ','-'))
      return filename + "#" + anchor

class NestedFileLayout (FileLayout):
   """generates a structured file system instead of a flat one"""

   def nameOfScope(self, scope):
      """Return the filename of a scoped name (class or module).
      One subdirectory per scope"""

      prefix = 'Scopes'
      if not len(scope):
         return self._checkMain(os.path.join(prefix, 'global') + '.html')
      else: return Util.quote(reduce(os.path.join, scope, prefix)) + '.html'

   def nameOfFileIndex(self, file):
      """Return the filename for the index of an input file.
      The base_dir config option is used.
      Default implementation is to join the path with '-', prepend "_file-"
      and append ".html" """

      file = self._stripFilename(file)
      return os.path.join("File", rel(config.base_dir, file)+".html")

   def nameOfFileSource(self, file):
      """Return the filename for the source of an input file.
      The base_dir config option is used.
      Default implementation is to join the path with '-', prepend "_source-"
      and append ".html" """

      file = self._stripFilename(file)
      return os.path.join("Source", rel(config.base_dir, file)+".html")

   def nameOfFileDetails(self, file):
      """Return the filename for the details of an input file.
      The base_dir config option is used.
      Returns the filename nested in the FileDetails directory and with
      .html appended."""

      file = self._stripFilename(file)
      return os.path.join("FileDetails", rel(config.base_dir, file)+".html")

   def nameOfIndex(self):
      """Return the name of the main index file."""

      return "index.html"

   def nameOfSpecial(self, name):
      """Return the name of a special file (tree, etc)."""

      return self._checkMain(name + ".html")
    
   def nameOfScopedSpecial(self, name, scope, ext=".html"):
      """Return the name of a special type of scope file"""

      return Util.quote(reduce(os.path.join, scope, name)) + ext

   def nameOfModuleTree(self):
      """Return the name of the module tree index"""

      return "Modules.html"

   def nameOfModuleIndex(self, scope):
      """Return the name of the index of the given module"""

      return Util.quote(reduce(os.path.join, scope, 'Modules')) + '.html'

# $Id: FileLayout.py,v 1.24 2003/11/16 21:09:45 stefan Exp $
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
from Tags import *

import os, os.path, sys, stat, string, re

class FileLayout (TOC.Linker):
   """Base class for naming files.
   You may derive from this class an
   reimplement any methods you like. The default implementation stores
   everything in the same directory (specified by -o)."""

   def init(self, processor):

      self.processor = processor
      self.base = self.processor.output
      if not os.path.exists(self.base):
         if processor.verbose: print "Warning: Output directory does not exist. Creating."
         try:
            os.makedirs(self.base, 0755)
         except EnvironmentError, reason:
            print "ERROR: Creating directory:",reason
            sys.exit(2)
      elif not os.path.isdir(self.base):
         print "ERROR: Output must be a directory."
         sys.exit(1)

      if processor.stylesheet_file:
         self.copy_file(processor.stylesheet_file, processor.stylesheet)

   def copy_file(self, src, dest):
      """Copies file if newer. The file named by orig_name is compared to
      <output>/<new_name>, and if newer or new_name doesn't exist, it is copied."""

      dest = os.path.join(self.base, dest)
      try:
         filetime = os.stat(src)[stat.ST_MTIME]
         if (not os.path.exists(dest)
             or filetime > os.stat(dest)[stat.ST_MTIME]):
            Util.open(dest).write(open(src, 'r').read())
      except EnvironmentError, reason:
         import traceback
         traceback.print_exc()
         print "ERROR: ", reason
         sys.exit(2)
    
   def _check_main(self, filename):
      """Checks whether the given filename is the main index page or not. If
      it is, then it returns the filename from index(), else it returns
      it unchanged"""

      if filename == self.processor.main_page: return self.index()
      return filename
	
   def scope(self, scope):
      """Return the filename of a scoped name (class or module).
      The default implementation is to join the names with '-' and append
      ".html" Additionally, special characters are Util.quoted in URL-style"""

      if not len(scope): return self._check_main(self.special('global'))
      return Util.quote(string.join(scope,'-')) + ".html"

   def _strip_filename(self, file):

      if len(file) and file[-1] == '/': file = file[:-1]
      return file

   def file_index(self, file):
      """Return the filename for the index of an input file.
      Default implementation is to join the path with '-', prepend "_file-"
      and append ".html" """

      file = self._strip_filename(file)
      return "_file-"+rel(self.base, file).replace(os.sep, '-')+".html"

   def file_source(self, file):
      """Return the filename for the source of an input file.
      Default implementation is to join the path with '-', prepend "_source-"
      and append ".html" """

      file = self._strip_filename(file)
      return "_source-"+rel(self.base, file).replace(os.sep, '-')+".html"

   def file_details(self, file):
      """Return the filename for the details of an input file.
      Default implementation is to join the path with '-', prepend
      "_filedetail-" and append ".html" """

      file = self._strip_filename(file)
      return "_filedetail-"+rel(self.base, file).replace(os.sep, '-')+".html"

   def index(self):
      """Return the name of the main index file. Default is index.html"""

      return "index.html"

   def special(self, name):
      """Return the name of a special file (tree, etc). Default is
      _name.html"""

      return self._check_main("_" + name + ".html")

   def scoped_special(self, name, scope, ext=".html"):
      """Return the name of a special type of scope file. Default is to join
      the scope with '-' and prepend '-'+name"""

      return "_%s-%s%s"%(name, Util.quote(string.join(scope, '-')), ext)

   def module_tree(self):
      """Return the name of the module tree index. Default is
      _modules.html"""

      return "_modules.html"

   def module_index(self, scope):
      """Return the name of the index of the given module. Default is to
      join the name with '-', prepend "_module_" and append ".html" """

      # Prefer module index for frames
      if self.processor.using_module_index:
         return "_module_" + Util.quote(string.join(scope, '-')) + ".html"
      # Fall back to the scope page
      return self.scope(scope)
	
   def link(self, decl):
      """Create a link to the named declaration. This method may have to
      deal with the directory layout."""

      if isinstance(decl, AST.Scope):
         # This is a class or module, so it has its own file
         return self.scope(decl.name())
      # Assume parent scope is class or module, and this is a <A> name in it
      filename = self.scope(decl.name()[:-1])
      anchor = anglebrackets(decl.name()[-1].replace(' ','-'))
      return filename + "#" + anchor

class NestedFileLayout (FileLayout):
   """generates a structured file system instead of a flat one"""

   def scope(self, scope):
      """Return the filename of a scoped name (class or module).
      One subdirectory per scope"""

      prefix = 'Scopes'
      if not len(scope):
         return self._check_main(os.path.join(prefix, 'global') + '.html')
      else: return Util.quote(reduce(os.path.join, scope, prefix)) + '.html'

   def file_index(self, file):
      """Return the filename for the index of an input file.
      Default implementation is to join the path with '-', prepend "_file-"
      and append ".html" """

      file = self._strip_filename(file)
      return os.path.join("File", rel(self.base, file)+".html")

   def file_source(self, file):
      """Return the filename for the source of an input file.
      Default implementation is to join the path with '-', prepend "_source-"
      and append ".html" """

      file = self._strip_filename(file)
      return os.path.join("Source", rel(self.base, file)+".html")

   def file_details(self, file):
      """Return the filename for the details of an input file.
      Returns the filename nested in the FileDetails directory and with
      .html appended."""

      file = self._strip_filename(file)
      return os.path.join("FileDetails", rel(self.base, file)+".html")

   def special(self, name):
      """Return the name of a special file (tree, etc)."""

      return self._check_main(name + ".html")
    
   def scoped_special(self, name, scope, ext=".html"):
      """Return the name of a special type of scope file"""

      return Util.quote(reduce(os.path.join, scope, name)) + ext

   def module_tree(self):
      """Return the name of the module tree index"""

      return "Modules.html"

   def module_index(self, scope):
      """Return the name of the index of the given module"""

      return Util.quote(reduce(os.path.join, scope, 'Modules')) + '.html'

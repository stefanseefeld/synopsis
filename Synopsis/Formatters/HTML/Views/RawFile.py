# $Id: RawFile.py,v 1.9 2003/11/14 14:51:09 stefan Exp $
#
# Copyright (C) 2000 Stephen Davies
# Copyright (C) 2000 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.Processor import Parameter
from Synopsis import AST, Util
from Part import Page
import ASTFormatter
from core import config
from Tags import *

import time, os, stat, os.path, string

class RawFilePages(Page):
   """A module for creating a page for each file with hyperlinked source"""

   def register(self, processor):

      Page.register(self, processor)
      self.__base = config.base_dir
      self.__start = config.start_dir
      self.__files = None
      self.__exclude_globs = config.exclude_globs

   def filename(self):
      """since RawFilePages generates a whole file hierarchy, this method returns the current filename,
      which may change over the lifetime of this object"""

      return self.__filename

   def title(self):
      """since RawFilePages generates a while file hierarchy, this method returns the current title,
      which may change over the lifetime of this object"""

      return self.__title

   def _get_files(self):
      """Returns a list of (path, output_filename) for each file"""

      if self.__files is not None: return self.__files
      self.__files = []
      dirs = [self.__start]
      while dirs:
         dir = dirs.pop(0)
         for entry in os.listdir(os.path.abspath(dir)):
            # Check if entry is in exclude list
            exclude = 0
            for re in self.__exclude_globs:
               if re.match(entry):
                  exclude = 1
            if exclude:
               continue
            entry_path = os.path.join(dir, entry)
            info = os.stat(entry_path)
            if stat.S_ISDIR(info[stat.ST_MODE]):
               dirs.append(entry_path)
            else:
               filename = self.processor.file_layout.nameOfFileSource(entry_path)
               self.__files.append( (entry_path, filename) )
      return self.__files

   def process(self, start):
      """Creates a page for every file"""

      for path, filename in self._get_files():
         self.process_file(path, filename)

   def register_filenames(self, start):
      """Registers a page for every file"""

      for path, filename in self._get_files():
         self.processor.register_filename(filename, self, path)

   def process_file(self, original, filename):
      """Creates a page for the given file"""

      # Check that we got the rego
      reg_page, reg_scope = self.processor.filename_info(filename)
      if reg_page is not self: return

      self.__filename = filename
      self.__title = original
      self.start_file()
      self.write(self.processor.formatHeader(filename, 2))
      self.write('<h1>'+original+'</h1>')
      try:
         f = open(original, 'rt')
         lines = ['']+f.readlines()
         f.close()
         wid = 1
         if len(lines) > 1000: wid = 4
         elif len(lines) > 100: wid = 3
         elif len(lines) > 10: wid = 2
         spec = '%%0%dd | %%s'%wid
         for i in range(1, len(lines)):
            lines[i] = spec%(i, anglebrackets(lines[i]))
         self.write('<pre>')
         self.write(string.join(lines, ''))
         self.write('</pre>')
      except:
         self.write('An error occurred')
      self.end_file()


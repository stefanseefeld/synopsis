# $Id: DirBrowse.py,v 1.10 2003/11/14 14:51:09 stefan Exp $
#
# Copyright (C) 2000 Stephen Davies
# Copyright (C) 2000 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.Processor import Parametrized
from Synopsis import AST, Util
from Synopsis.Formatters.HTML.Page import Page
from Synopsis.Formatters.HTML.core import config
from Synopsis.Formatters.HTML.Tags import *

import os, stat, os.path, string, time

class DirBrowse(Page):
   """A page that shows the entire contents of directories, in a form similar
   to LXR."""

   def filename(self):
      """since FileTree generates a whole file hierarchy, this method returns the current filename,
      which may change over the lifetime of this object"""

      return self.__filename

   def title(self):
      """since FileTree generates a while file hierarchy, this method returns the current title,
      which may change over the lifetime of this object"""

      return self.__title

   def filename_for_dir(self, dir):
      """Returns the output filename for the given input directory"""

      if dir is self.__start:
         return self.processor.file_layout.nameOfSpecial('dir')
      scope = string.split(rel(self.__start, dir), os.sep)
      return self.processor.file_layout.nameOfScopedSpecial('dir', scope)

   def register(self, processor):
      """Registers a page for each file in the hierarchy"""

      Page.register(self, processor)

      self.__filename = self.processor.file_layout.nameOfSpecial('dir')
      self.__title = 'Directory Listing'
      self.__base = config.base_dir
      self.__start = config.start_dir
      self.__exclude_globs = config.exclude_globs
      #if not self.__base: return
      config.set_main_page(self.__filename)
      self.__filename = self.processor.file_layout.nameOfSpecial('dir')
      self.processor.addRootPage(self.__filename, 'Files', 'main', 2)

   def register_filenames(self, start):
      """Registers a page for every directory"""
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
            if not stat.S_ISDIR(info[stat.ST_MODE]):
               continue
            filename = self.filename_for_dir(dir)
            self.processor.register_filename(filename, self, entry_path)
   
   def process(self, start):
      """Recursively visit each directory below the base path given in the
      config."""

      #if not self.__base: return
      self.process_dir(self.__start)
    
   def process_dir(self, path):
      """Process a directory, producing an output page for it"""

      file_layout = self.processor.file_layout

      # Find the filename
      filename = self.filename_for_dir(path)
      self.__filename = filename

      # Start the file
      self.start_file()
      self.write(self.processor.formatHeader(self.filename(), 1))
      # Write intro stuff
      root = rel(self.__base, self.__start)
      if not len(root) or root[-1] != '/': root = root + '/'
      if path is self.__start:
         self.write('<h1> '+root)
      else:
         self.write('<h1>'+href(file_layout.nameOfSpecial('dir'), root + ' '))
         dirscope = []
         scope = string.split(rel(self.__start, path), os.sep)
         for dir in scope[:-1]:
            dirscope.append(dir)
            dirlink = file_layout.nameOfScopedSpecial('dir', dirscope)
            dirlink = rel(self.filename(), dirlink)
            self.write(href(dirlink, dir+'/ '))
         if len(scope) > 0:
            self.write(scope[-1]+'/')
      self.write(' - Directory listing</h1>')
      # Start the table
      self.write('<table summary="Directory Listing">\n')
      self.write('<tr><th align=left>Name</th>')
      self.write('<th align="right">Size (bytes)</th>')
      self.write('<th align="right">Last modified (GMT)</th></tr>\n')
      # List all files in the directory
      entries = os.listdir(os.path.abspath(path))
      entries.sort()
      files = []
      dirs = []
      for entry in entries:
         # Check if entry is in exclude list
         exclude = 0
         for re in self.__exclude_globs:
            if re.match(entry):
               exclude = 1
         if exclude:
            continue
         entry_path = os.path.join(path, entry)
         info = os.stat(entry_path)
         if stat.S_ISDIR(info[stat.ST_MODE]):
            # A directory, process now
            scope = string.split(rel(self.__start, entry_path), os.sep)
            linkpath = file_layout.nameOfScopedSpecial('dir', scope)
            linkpath = rel(self.filename(), linkpath)
            self.write('<tr><td>%s</td><td></td><td align="right">%s</td></tr>\n'%(
               href(linkpath, entry+'/'),
               time.asctime(time.gmtime(info[stat.ST_MTIME]))))
            dirs.append(entry_path)
         else:
            files.append((entry_path, entry, info))
      for path, entry, info in files:
         size = info[stat.ST_SIZE]
         timestr = time.asctime(time.gmtime(info[stat.ST_MTIME]))
         linkpath = file_layout.nameOfFileSource(path)
         rego = self.processor.filename_info(linkpath)
         if rego:
            linkurl = rel(self.filename(), linkpath)
            self.write('<tr><td>%s</td><td align=right>%d</td><td align="right">%s</td></tr>\n'%(
               href(linkurl, entry, target="main"), size, timestr))
         else:
            #print "No link for",linkpath
            self.write('<tr><td>%s</td><td align=right>%d</td><td align="right">%s</td></tr>\n'%(
               entry, size, timestr))
      # End the table and file
      self.write('</table>')
      self.end_file()

      # recursively create all child directory pages
      for dir in dirs:
         self.process_dir(dir)

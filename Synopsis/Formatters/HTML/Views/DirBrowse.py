#
# Copyright (C) 2000 Stephen Davies
# Copyright (C) 2000 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.Processor import Parameter
from Synopsis import AST, Util
from Synopsis.Formatters.HTML.View import View
from Synopsis.Formatters.HTML.Tags import *

import os, stat, os.path, string, time, re

def compile_glob(globstr):
    """Returns a compiled regular expression for the given glob string. A
    glob string is something like "*.?pp" which gets translated into
    "^.*\..pp$"."""
    glob = string.replace(globstr, '.', '\.')
    glob = string.replace(glob, '?', '.')
    glob = string.replace(glob, '*', '.*')
    glob = re.compile('^%s$'%glob)
    return glob

class DirBrowse(View):
   """A view that shows the entire contents of directories, in a form similar
   to LXR."""

   src_dir = Parameter('', 'starting point for directory listing')
   base_path = Parameter('', 'path prefix to strip off of the file names')
   exclude = Parameter([], 'TODO: define an exclusion mechanism (glob based ?)')

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

      if dir is self.src_dir:
         return self.processor.file_layout.special('dir')
      scope = string.split(rel(self.src_dir, dir), os.sep)
      return self.processor.file_layout.scoped_special('dir', scope)

   def register(self, processor):
      """Registers a view for each file in the hierarchy"""

      View.register(self, processor)

      self._exclude = [compile_glob(e) for e in self.exclude]

      self.__filename = self.processor.file_layout.special('dir')
      
      self.__title = 'Directory Listing'
      processor.set_main_view(self.__filename)
      # FIXME: file_layout.special() will return two distinct values,
      #        depending on whether this is the main view or not
      #        but because __filename is used to *define* the main view,
      #        we are in a catch 22...
      self.processor.add_root_view(self.processor.file_layout.special('dir'),
                                   self.title(), 'main', 2)

   def register_filenames(self, start):
      """Registers a view for every directory"""

      dirs = [self.src_dir]
      while dirs:
         dir = dirs.pop(0)
         for entry in os.listdir(os.path.abspath(dir)):
            exclude = 0
            for re in self._exclude:
               if re.match(entry):
                  print entry, 'excluded'
                  exclude = 1
                  break
            if exclude:
               continue
            entry_path = os.path.join(dir, entry)
            if os.path.isdir(entry_path):
               filename = self.filename_for_dir(dir)
               self.processor.register_filename(filename, self, entry_path)
               dirs.append(entry_path)
   
   def process(self, start):
      """Recursively visit each directory below the base path given in the
      config."""

      #if not self.__base: return
      self.process_dir(self.src_dir)
    
   def process_dir(self, path):
      """Process a directory, producing an output view for it"""

      file_layout = self.processor.file_layout

      # Find the filename
      self.__filename = self.filename_for_dir(path)

      # Start the file
      self.start_file()
      self.write(self.processor.navigation_bar(self.filename(), 1))
      # Write intro stuff
      root = rel(self.base_path, self.src_dir)
      if not len(root) or root[-1] != '/': root = root + '/'
      if path is self.src_dir:
         self.write('<h1> '+root)
      else:
         self.write('<h1>'+href(file_layout.special('dir'), root + ' '))
         dirscope = []
         scope = string.split(rel(self.src_dir, path), os.sep)
         
         for dir in scope[:-1]:
            dirscope.append(dir)
            dirlink = file_layout.scoped_special('dir', dirscope)
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
         exclude = 0
         for re in self._exclude:
            if re.match(entry):
               exclude = 1
               print entry, 'excluded'
               break
         if exclude:
            continue
         entry_path = os.path.join(path, entry)
         info = os.stat(entry_path)
         if stat.S_ISDIR(info[stat.ST_MODE]):
            # A directory, process now
            scope = string.split(rel(self.src_dir, entry_path), os.sep)
            linkpath = file_layout.scoped_special('dir', scope)
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
         # strip of base_path
         path = path[len(self.base_path):]
         if path[0] == '/': path = path[1:]
         linkpath = file_layout.file_source(path)
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

      # recursively create all child directory views
      for dir in dirs:
         self.process_dir(dir)

# $Id: Source.py,v 1.5 2003/11/12 16:42:05 stefan Exp $
#
# Copyright (C) 2000 Stephen Davies
# Copyright (C) 2000 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.Processor import Parameter
from Synopsis import AST, Util

from Page import Page
import core
import ASTFormatter
from core import config
from Tags import *

import time, os

link = None
try:
   link = Util._import("Synopsis.Parsers.Cxx.link")
except ImportError:
   print "Warning: unable to import link module. Continuing..."

class FileSource(Page):
   """A module for creating a page for each file with hyperlinked source"""

   def register(self, manager):

      Page.register(self, manager)
      # Try old name first for backwards compatibility
      if hasattr(config.obj, 'FilePages'): myconfig = config.obj.FilePages
      else: myconfig = config.obj.FileSource
      self.__linkpath = myconfig.links_path
      self.__toclist = myconfig.toc_files
      self.__scope = myconfig.scope
      # We will NOT be in the Manual directory  TODO - use FileLayout for this
      self.__toclist = map(lambda x: ''+x, self.__toclist)
      if hasattr(myconfig, 'use_toc'):
         self.__tocfrom = myconfig.use_toc
      else:
         self.__tocfrom = config.default_toc

   def filename(self):
      """since FileSource generates a whole file hierarchy, this method returns the current filename,
      which may change over the lifetime of this object"""

      return self.__filename

   def title(self):
      """since FileSource generates a while file hierarchy, this method returns the current title,
      which may change over the lifetime of this object"""

      return self.__title

   def process(self, start):
      """Creates a page for every file"""

      # Get the TOC
      toc = self.manager.get_toc(start)
      tocfile = config.files.nameOfSpecial('FileSourceInputTOC')
      tocfile = os.path.join(config.basename, tocfile)
      toc.store(tocfile)
      self.__toclist.append(tocfile)
      # create a page for each main file
      for file in config.ast.files().values():
         if file.is_main():
            self.process_node(file)
	
      os.unlink(tocfile)

   def register_filenames(self, start):
      """Registers a page for every source file"""

      for file in config.ast.files().values():
         if file.is_main():
            filename = file.filename()
            filename = os.path.join(config.base_dir, filename)
            filename = config.files.nameOfFileSource(filename)
            #print "Registering",filename
            self.manager.register_filename(filename, self, file)
	     
   def process_node(self, file):
      """Creates a page for the given file"""

      # Start page
      toc = config.toc
      filename = file.filename()
      filename = os.path.join(config.base_dir, filename)
      self.__filename = config.files.nameOfFileSource(filename)
      #name = list(node.path)
      #while len(name) and name[0] == '..': del name[0]
      #source = string.join(name, os.sep)
      source = file.filename()
      self.__title = source

      # Massage toc list to prefix '../../.....' to any relative entry.
      prefix = rel(self.filename(), '')
      toc_to_change = config.toc_out
      toclist = list(self.__toclist)
      for index in range(len(toclist)):
         if '|' not in toclist[index]:
            toclist[index] = toclist[index]+'|'+prefix

      self.start_file()
      self.write(self.manager.formatHeader(self.filename()))
      self.write('File: '+entity('b', self.__title))

      if not link:
         # No link module..
         self.write('link module for highlighting source unavailable')
         try:
            self.write(open(file.full_filename(),'r').read())
         except IOError, e:
            self.write("An error occurred:"+ str(e))
      else:
         self.write('<br><div class="file-all">\n')
         # Call link module
         f_out = os.path.join(config.basename, self.__filename) + '-temp'
         f_in = file.full_filename()
         f_link = self.__linkpath%source
         #print "file: %s    link: %s    out: %s"%(f_in, f_link, f_out)
         try:
            link.link(toclist, f_in, f_out, f_link, self.__scope) #, config.types)
         except link.error, msg:
            print "An error occurred:",msg
         try:
            self.write(open(f_out,'r').read())
            os.unlink(f_out)
         except IOError, e:
            self.write("An error occurred:"+ str(e))
         self.write("</div>")

      self.end_file()

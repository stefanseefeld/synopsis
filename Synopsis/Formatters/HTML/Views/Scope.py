# $Id: Scope.py,v 1.22 2003/11/13 20:40:09 stefan Exp $
#
# Copyright (C) 2000 Stephen Davies
# Copyright (C) 2000 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.Processor import Parameter
from Synopsis import AST
from Synopsis.Formatters.TOC import TOC

from Page import Page
import core
import ASTFormatter
from core import config
from Tags import *

import time, os

class ScopePages(Page):
   """A module for creating a page for each Scope with summaries and
   details. This module is highly modular, using the classes from
   ASTFormatter to do the actual formatting. The classes to use may be
   controlled via the config script, resulting in a very configurable output.
   @see ASTFormatter The ASTFormatter module
   @see Config.Formatters.HTML.ScopePages Config for ScopePages
   """
   
   def register(self, manager):

      Page.register(self, manager)
      share = config.datadir
      self.syn_logo = 'synopsis200.jpg'
      if config.files:
         config.files.copyFile(os.path.join(share, 'synopsis200.jpg'), os.path.join(config.basename, self.syn_logo))
      self.__parts = []
      self._get_parts()
      self.__namespaces = []
      self.__toc = None
      
   def _get_parts(self):
      "Loads the list of parts from config"

      try:
         parts = config.obj.ScopePages.parts
      except AttributeError:
         parts = ['Heading', 'Summary', 'Detail']
      base = 'Synopsis.Formatters.HTML.ASTFormatter.'
      for part in parts:
         obj = core.import_object(part, basePackage=base)(self)
         self.__parts.append(obj)
    
   def get_toc(self, start):
      """Returns the TOC for the whole AST starting at start"""
      
      if self.__toc: return self.__toc
      self.__toc = TOC(config.files)
      start.accept(self.__toc)
      return self.__toc

   def filename(self):
      """since ScopePages generates a whole file hierarchy, this method returns the current filename,
      which may change over the lifetime of this object"""

      return self.__filename

   def title(self):
      """since ScopePages generates a while file hierarchy, this method returns the current title,
      which may change over the lifetime of this object"""

      return self.__title

   def scope(self):
      """return the current scope processed by this object"""

      return self.__scope

   def process(self, start):
      """Creates a page for every Scope"""

      self.__namespaces = [start]
      while self.__namespaces:
         ns = self.__namespaces.pop(0)
         self.process_scope(ns)
         
         # Queue child namespaces
         for child in config.sorter.children():
            if isinstance(child, AST.Scope):
               self.__namespaces.append(child)

   def register_filenames(self, start):
      """Registers a page for every Scope"""

      self.__namespaces = [start]
      while self.__namespaces:
         ns = self.__namespaces.pop(0)

         filename = config.files.nameOfScope(ns.name())
         self.manager.register_filename(filename, self, ns)

         config.sorter.set_scope(ns)
         
         # Queue child namespaces
         for child in config.sorter.children():
            if isinstance(child, AST.Scope):
               self.__namespaces.append(child)
     
   def process_scope(self, ns):
      """Creates a page for the given scope"""

      details = {} # A hash of lists of detailed children by type
      sections = [] # a list of detailed sections
	
      # Open file and setup scopes
      self.__scope = ns.name()
      self.__filename = config.files.nameOfScope(self.__scope)
      self.__title = anglebrackets(string.join(self.__scope))
      self.start_file()
	
      # Write heading
      self.write(self.manager.formatHeader(self.filename()))

      # Loop throught all the page Parts
      for part in self.__parts:
         part.process(ns)
      self.end_file()
    
   def end_file(self):
      """Overrides end_file to provide synopsis logo"""

      self.write('<hr>\n')
      now = time.strftime(r'%c', time.localtime(time.time()))
      logo = href('http://synopsis.fresco.org', 'synopsis')
      self.write(div('logo', 'Generated on ' + now + ' by \n<br>\n' + logo))
      Page.end_file(self)

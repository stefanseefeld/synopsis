# $Id: XRef.py,v 1.12 2003/11/13 20:40:09 stefan Exp $
#
# Copyright (C) 2000 Stephen Davies
# Copyright (C) 2000 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.Processor import Parameter
from Synopsis import AST, Type, Util
from Synopsis.Formatters.TOC import TOC

from Page import Page
from core import config
from Tags import *

import os

class XRefLinker(Linker):

   def __init__(self, xref):

      self.xref = xref

   def link(self, name):

      return file

class XRefPages(Page):
   """A module for creating pages full of xref infos"""

   def register(self, manager):

      Page.register(self, manager)
      self.xref = config.xref
      self.__filename = None
      self.__title = None
      self.__toc = None
      self.__link_to_scopepages = 0
      if hasattr(config.obj, 'XRefPages'):
         if hasattr(config.obj.XRefPages, 'xref_file'):
            self.xref.load(config.obj.XRefPages.xref_file)
         if hasattr(config.obj.XRefPages, 'link_to_scopepages'):
            self.__link_to_scopepages = config.obj.XRefPages.link_to_scopepages

   def get_toc(self, start):
      """Returns the toc for XRefPages"""

      if self.__toc: return self.__toc
      self.__toc = TOC(None)
      # Add an entry for every xref
      for name in self.xref.get_all_names():
         page = self.xref.get_page_for(name)
         file = config.files.nameOfSpecial('xref%d'%page)
         file = file + '#' + Util.quote(string.join(name,'::'))
         self.__toc.insert(TOC.Entry(name, file, 'C++', 'xref'))
      return self.__toc

   def filename(self):
      """Returns the current filename,
      which may change over the lifetime of this object"""

      return self.__filename

   def title(self):
      """Returns the current title,
      which may change over the lifetime of this object"""

      return self.__title

   def process(self, start):
      """Creates a page for every bunch of xref infos"""

      page_info = self.xref.get_page_info()
      if not page_info: return
      for i in range(len(page_info)):
         self.__filename = config.files.nameOfSpecial('xref%d'%i)
         self.__title = 'Cross Reference page #%d'%i

         self.start_file()
         self.write(self.manager.formatHeader(self.filename()))
         self.write(entity('h1', self.__title))
         self.write('<hr>')
         for name in page_info[i]:
            self.process_name(name)
         self.end_file()

   def register_filenames(self, start):
      """Registers each page"""

      page_info = self.xref.get_page_info()
      if not page_info: return
      for i in range(len(page_info)):
         filename = config.files.nameOfSpecial('xref%d'%i)
         self.manager.register_filename(filename, self, i)
    
   def process_link(self, file, line, scope):
      """Outputs the info for one link"""

      # Make a link to the highlighted source
      realfile = os.path.join(config.base_dir, file)
      file_link = config.files.nameOfFileSource(realfile)
      file_link = file_link + "#%d"%line
      # Try and make a descriptive
      desc = ''
      if config.types.has_key(scope):
         type = config.types[scope]
         if isinstance(type, Type.Declared):
            desc = ' ' + type.declaration().type()
      # Try and find a link to the scope
      scope_text = string.join(scope, '::')
      entry = config.toc[scope]
      if entry:
         scope_text = href(entry.link, scope_text)
      # Output list element
      self.write('<li><a href="%s">%s:%s</a>: in%s %s</li>\n'%(
         file_link, file, line, desc, scope_text))
    
   def describe_decl(self, decl):
      """Returns a description of the declaration. Detects constructors and
      destructors"""

      name = decl.name()
      if isinstance(decl, AST.Function) and len(name) > 1:
         real = decl.realname()[-1]
         if name[-2] == real:
            return 'Constructor '
         elif real[0] == '~' and name[-2] == real[1:]:
            return 'Destructor '
      return decl.type().capitalize() + ' '

   def process_name(self, name):
      """Outputs the info for a given name"""
      
      target_data = self.xref.get_info(name)
      if not target_data: return

      jname = string.join(name, '::')
      self.write(entity('a', '', name=Util.quote(jname)))
      desc = ''
      decl = None
      if config.types.has_key(name):
         type = config.types[name]
         if isinstance(type, Type.Declared):
            decl = type.declaration()
            desc = self.describe_decl(decl)
      self.write(entity('h2', desc + jname) + '<ul>\n')
	
      if self.__link_to_scopepages:
         if config.types.has_key(name):
            type = config.types[name]
            if isinstance(type, Type.Declared):
               link = config.files.link(type.declaration())
               self.write('<li>'+href(rel(self.__filename, link), 'Documentation')+'</li>')
      if target_data[0]:
         self.write('<li>Defined at:<ul>\n')
         for file, line, scope in target_data[0]:
            self.process_link(file, line, scope)
         self.write('</ul></li>\n')
      if target_data[1]:
         self.write('<li>Called from:<ul>\n')
         for file, line, scope in target_data[1]:
            self.process_link(file, line, scope)
         self.write('</ul></li>\n')
      if target_data[2]:
         self.write('<li>Referenced from:<ul>\n')
         for file, line, scope in target_data[2]:
            self.process_link(file, line, scope)
         self.write('</ul></li>\n')
      if isinstance(decl, AST.Scope):
         self.write('<li>Declarations:<ul>\n')
         for child in decl.declarations():
            file, line = child.file().filename(), child.line()
            realfile = os.path.join(config.base_dir, file)
            file_link = config.files.nameOfFileSource(realfile)
            file_link = '%s#%d'%(file_link,line)
            file_href = '<a href="%s">%s:%s</a>: '%(file_link,file,line)
            cname = child.name()
            entry = config.toc[cname]
            type = self.describe_decl(child)
            if entry:
               link = href(entry.link, Util.ccolonName(cname, name))
               self.write(entity('li', file_href + type + link))
            else:
               self.write(entity('li', file_href + type + Util.ccolonName(cname, name)))
         self.write('</ul></li>\n')
      self.write('</ul><hr>\n')

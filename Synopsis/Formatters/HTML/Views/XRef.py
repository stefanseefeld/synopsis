#
# Copyright (C) 2000 Stephen Davies
# Copyright (C) 2000 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.Processor import Parameter
from Synopsis import ASG, Type, Util
from Synopsis.Formatters.TOC import TOC, Linker
from Synopsis.Formatters.HTML.View import View
from Synopsis.Formatters.HTML.Tags import *
from Synopsis.Formatters.XRef import *

class XRef(View):
   """A module for creating views full of xref infos"""

   xref_file = Parameter('', '')
   link_to_scope = Parameter(True, '')

   def register(self, frame):

      super(XRef, self).register(frame)
      self.__filename = None
      self.__title = None
      if self.xref_file:
         self.processor.xref.load(self.xref_file)
      self.__toc = TOC(None)
      # Add an entry for every xref
      xref = self.processor.xref
      for name in xref.get_all_names():
         view = xref.get_page_for(name)
         file = self.directory_layout.special('xref%d'%view)
         file = file + '#' + Util.quote('::'.join(name))
         self.__toc.insert(TOC.Entry(name, file, 'C++', 'xref'))

   def toc(self, start):

      return self.__toc

   def filename(self):

      return self.__filename

   def title(self):

      return self.__title

   def process(self):

      page_info = self.processor.xref.get_page_info()
      if not page_info: return
      for i in range(len(page_info)):
         self.__filename = self.directory_layout.xref(i)
         self.__title = 'Cross Reference page #%d'%i

         self.start_file()
         self.write_navigation_bar()
         self.write(element('h1', self.title()))
         for name in page_info[i]:
            self.write('<div class="xref-name">')
            self.process_name(name)
            self.write('</div>')
         self.end_file()

   def register_filenames(self):
      """Registers each view"""

      page_info = self.processor.xref.get_page_info()
      if not page_info: return
      for i in range(len(page_info)):
         filename = self.directory_layout.special('xref%d'%i)
         self.processor.register_filename(filename, self, i)
    
   def process_link(self, file, line, scope):
      """Outputs the info for one link"""

      # Make a link to the highlighted source
      file_link = self.directory_layout.file_source(file)
      file_link = rel(self.filename(), file_link) + "#%d"%line
      # Try and make a descriptive
      desc = ''
      type = self.processor.ir.types.get(scope)
      if isinstance(type, Type.Declared):
         desc = ' ' + type.declaration.type
      # Try and find a link to the scope
      scope_text = '::'.join(scope)
      entry = self.processor.toc[scope]
      if entry:
         scope_text = href(rel(self.filename(), entry.link), escape(scope_text))
      else:
         scope_text = escape(scope_text)
      # Output list element
      self.write('<li><a href="%s">%s:%s</a>: in%s %s</li>\n'%(
         file_link, file, line, desc, scope_text))
    
   def describe_decl(self, decl):
      """Returns a description of the declaration. Detects constructors and
      destructors"""

      name = decl.name
      if isinstance(decl, ASG.Function) and len(name) > 1:
         real = decl.real_name[-1]
         if name[-2] == real:
            return 'Constructor '
         elif real[0] == '~' and name[-2] == real[1:]:
            return 'Destructor '
      return decl.type.capitalize() + ' '

   def process_name(self, name):
      """Outputs the info for a given name"""
      
      target_data = self.processor.xref.get_info(name)
      if not target_data: return

      jname = '::'.join(name)
      self.write(element('a', '', name=Util.quote(escape(jname))))
      desc = ''
      decl = None
      type = self.processor.ir.types.get(name)
      if isinstance(type, Type.Declared):
         decl = type.declaration
         desc = self.describe_decl(decl)
      self.write(element('h2', desc + escape(jname)) + '<ul>\n')
	
      if self.link_to_scope:
         type = self.processor.ir.types.get(name, None)
         if isinstance(type, Type.Declared):
            link = self.directory_layout.link(type.declaration)
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
      if isinstance(decl, ASG.Scope):
         self.write('<li>Declarations:<ul>\n')
         for child in decl.declarations:
            file, line = child.file.name, child.line
            file_link = self.directory_layout.file_source(file)
            file_link = rel(self.filename(),file_link) + '#%d'%line
            file_href = '<a href="%s">%s:%s</a>: '%(file_link,file,line)
            cname = child.name
            entry = self.processor.toc[cname]
            type = self.describe_decl(child)
            if entry:
               link = href(rel(self.filename(), entry.link), escape(Util.ccolonName(cname, name)))
               self.write(element('li', file_href + type + link))
            else:
               self.write(element('li', file_href + type + Util.ccolonName(cname, name)))
         self.write('</ul></li>\n')
      self.write('</ul>\n')

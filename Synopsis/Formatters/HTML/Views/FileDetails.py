#
# Copyright (C) 2000 Stephen Davies
# Copyright (C) 2000 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.Processor import Parameter
from Synopsis import ASG, Util
from Synopsis.Formatters.HTML.View import View
from Synopsis.Formatters.HTML.Tags import *
from Source import *

import os

class FileDetails(View):
   """A view that creates an index of files, and an index for each file.
   First the index of files is created, intended for the top-left frame.
   Second a view is created for each file, listing the major declarations for
   that file, eg: classes, global functions, namespaces, etc."""

   def register(self, frame):

      super(FileDetails, self).register(frame)
      self.__filename = ''
      self.__title = ''
      self.__link_source = self.processor.has_view('Source')

   def filename(self):
      """since FileTree generates a whole file hierarchy, this method returns the current filename,
      which may change over the lifetime of this object"""

      return self.__filename

   def title(self):
      """since FileTree generates a while file hierarchy, this method returns the current title,
      which may change over the lifetime of this object"""

      return self.__title

   def register_filenames(self):
      """Registers a view for each file indexed."""

      for filename, file in self.processor.ir.files.items():
         if file.annotations['primary']:
            filename = self.directory_layout.file_details(filename)
            self.processor.register_filename(filename, self, file)
    
   def process(self):
      """Creates a view for each known source file."""

      for filename, file in self.processor.ir.files.items():
         if file.annotations['primary']:
            self.process_file(filename, file)
            
   def process_file(self, filename, file):
      """Creates a view for the given file. The view is just an index,
      containing a list of declarations."""

      # set up filename and title for the current view
      self.__filename = self.directory_layout.file_details(filename)
      # (get rid of ../'s in the filename)
      name = filename.split(os.sep)
      while len(name) and name[0] == '..': del name[0]
      self.__title = os.sep.join(name)+' Details'

      self.start_file()
      self.write_navigation_bar()
      self.write(entity('h1', os.sep.join(name))+'<br/>')
      if self.__link_source:
         link = rel(self.filename(),
                    self.directory_layout.file_source(filename))
         self.write('(' + href(link, 'Source', target='content')+')<br/>')

      # Print list of includes
      try:
         sourcefile = self.processor.ir.files[filename]
         # Only show files from the project
         includes = [i for i in sourcefile.includes
                     if i.target.annotations['primary']]
         self.write('<h2 class="heading">Includes from this file:</h2>')
         if not includes:
            self.write('No includes.<br/>')
         for include in includes:
            target_filename = include.target.name
            if include.is_next: idesc = 'include_next '
            else: idesc = 'include '
            if include.is_macro: idesc = idesc + 'from macro '
            link = rel(self.filename(), self.directory_layout.file_details(target_filename))
            self.write(idesc + href(link, target_filename)+'<br/>')
      except:
         pass

      self.write('<h2 class="heading">Declarations in this file:</h2>')
      # Sort items (by name)
      items = [(d.type, d.name, d) for d in file.declarations]
      # ignore ASG.Builtin
      items = [i for i in items if not isinstance(i[2], ASG.Builtin)]
      items.sort()
      curr_scope = None
      curr_type = None
      br = 0
      for decl_type, name, decl in items:
         # Check scope and type to see if they've changed since the last
         # declaration, thereby forming sections of scope and type
         decl_scope = name[:-1]
         if decl_scope != curr_scope or decl_type != curr_type:
            if curr_scope is not None:
               self.write('\n</div>')
            curr_scope = decl_scope
            curr_type = decl_type
            if len(curr_type) and curr_type[-1] == 's': plural = 'es'
            else: plural = 's'
            if len(curr_scope):
               self.write('<h3>%s%s in %s</h3>\n<div>'%(
                  curr_type.capitalize(), plural,
                  escape(Util.ccolonName(curr_scope))))
            else:
               self.write('<h3>%s%s</h3>\n<div>'%(curr_type.capitalize(),plural))
            br = 0
            
         # Format this declaration
         entry = self.processor.toc[name]
         label = escape(Util.ccolonName(name, curr_scope))
         label = replace_spaces(label)
         if entry:
            link = rel(self.filename(), entry.link)
            text = ' ' + href(link, label)
         else:
            text = ' ' + label
         if br: self.write('<br/>')
         self.write(text)
         # Print summary
         text = self.processor.documentation.summary(decl, self)
         self.write('<br/>\n' + span('summary', text))
         br = 1
	
      # Close open div
      if curr_scope is not None:
         self.write('\n</div>')
      self.end_file()

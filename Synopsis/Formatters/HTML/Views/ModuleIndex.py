#
# Copyright (C) 2000 Stephen Davies
# Copyright (C) 2000 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.Processor import Parameter
from Synopsis import ASG
from Synopsis.Formatters.HTML.View import View
from Synopsis.Formatters.HTML.Tags import *

class ModuleIndex(View):
   """A module for indexing ASG.Modules. Each module gets its own view with a
   list of nested scope declarations with documentation. It is intended to go in
   the left frame..."""

   def register(self, frame):

      super(ModuleIndex, self).register(frame)
      self.__filename = self.directory_layout.module_index(self.processor.root.name)
      self.__title = 'Global %s Index'%self.processor.root.type.capitalize()

   def filename(self):

      return self.__filename

   def title(self):

      return self.__title

   def process(self):

      self.module_queue = [self.processor.root]
      while self.module_queue:
         m = self.module_queue.pop(0)
         self.process_module_index(m)
    
   def make_view_heading(self, module):
      """Creates a HTML fragment which becomes the name at the top of the
      index view. This may be overridden, but the default is (now) to make a
      linked fully scoped name, where each scope has a link to the relevant
      index."""

      name = module.name
      if not name: return 'Global Index'
      links = []
      for depth in range(0, len(name)):
         url = self.directory_layout.module_index(name[:depth+1])
         label = escape(name[depth])
         links.append(href(rel(self.__filename, url), label))
      return element('b', name.sep.join(links) + ' Index')

   def process_module_index(self, module):
      "Index one module"

      sorter = self.processor.sorter.clone(module.declarations)

      self.__filename = self.directory_layout.module_index(module.name)
      self.__title = str(module.name) or 'Global Module'
      self.__title = self.__title + ' Index'
      # Create file
      self.start_file()
      #target = rel(self.__filename, self.directory_layout.scope(module.name))
      #link = href(target, self.__title, target='content')
      self.write(self.make_view_heading(module))

      toc = self.processor.toc

      # Make script to switch main frame upon load
      load_script = '<!--\n'
      if toc[module.name]:
         target = rel(self.__filename, toc[module.name].link)
         load_script = load_script + 'window.parent.frames["content"].location="%s";\n'%target
      load_script = load_script + 'function go(index,detail) {\n'\
                    'window.parent.frames["index"].location=index;\n'\
                    'window.parent.frames["detail"].location=detail;\n'\
                    'return false;}\n-->'
      self.write(element('script', load_script, type='text/javascript'))

      # Loop throught all the types of children
      for section in sorter:
         heading = '<br/>\n'+element('i', escape(section))+'<br/>\n'
         # Get a list of children of this type
         for child in sorter[section]:
            # Print out summary for the child
            if not isinstance(child, ASG.Scope):
               continue
            if heading:
               self.write(heading)
               heading = None
            label = str(module.name.prune(child.name))
            label = escape(label)
            label = replace_spaces(label)
            if isinstance(child, ASG.Module):
               index_url = rel(self.__filename,
                               self.directory_layout.module_index(child.name))
               self.write(href(index_url, label, target='detail'))
            else:
               entry = toc[child.name]
               if entry:
                  url = rel(self.__filename, entry.link)
                  self.write(href(url, label, target='content'))
               else:
                  self.write(label)
            self.write('<br/>\n')
      self.end_file()

      children = [c for c in module.declarations if isinstance(c, ASG.Module)]
      self.module_queue.extend(children)

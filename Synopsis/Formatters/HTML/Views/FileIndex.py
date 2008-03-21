#
# Copyright (C) 2000 Stephen Davies
# Copyright (C) 2000 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.Processor import Parameter
from Synopsis import ASG
from Synopsis.QualifiedName import *
from Synopsis.Formatters.HTML.View import View
from Synopsis.Formatters.HTML.Tags import *
import os

class FileIndex(View):
    """A view that creates an index of files, and an index for each file.
    First the index of files is created, intended for the top-left frame.
    Second a view is created for each file, listing the major declarations for
    that file, eg: classes, global functions, namespaces, etc."""

    def register(self, frame):

       super(FileIndex, self).register(frame)
       self.__filename = ''
       self.__title = ''
       self.link_source = self.processor.has_view('Source') and self.processor.sxr_prefix
       self.link_details = self.processor.has_view('FileDetails')

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
                filename = self.directory_layout.file_index(filename)
                self.processor.register_filename(filename, self, file)
    
    def process(self):
        """Creates a view for each known file."""

        for filename, file in self.processor.ir.files.items():
            if file.annotations['primary']:
                self.process_file(filename, file)

    def process_file(self, filename, file):
        """Creates a view for the given file. The view is just an index,
        containing a list of declarations."""

        # set up filename and title for the current view
        self.__filename = self.directory_layout.file_index(filename)
        # (get rid of ../'s in the filename)
        name = filename.split(os.sep)
        while len(name) and name[0] == '..': del name[0]
        self.__title = os.sep.join(name)

        self.start_file()
        self.write(element('b', os.sep.join(name))+'\n')
        if self.link_source:
            link = rel(self.filename(),
                      self.directory_layout.file_source(filename))
            self.write(div('', href(link, 'source code', target='content')) + '\n')
        if self.link_details:
            link = rel(self.filename(),
                       self.directory_layout.file_details(filename))
            self.write(div('', href(link, 'details', target='content')) + '\n')

        self.write(div('heading', 'Declarations') + '\n')
        # Sort items (by name)
        items = [(d.name, d) for d in file.declarations]
        items.sort()
        if file.annotations.get('language') == 'Python':
            scope = QualifiedPythonName()
        else:
            scope = QualifiedCxxName()
        last = scope
        for name, decl in items:
            entry = self.processor.toc[name]
            if not entry: continue
            # Print link to declaration's view
            link = rel(self.filename(), entry.link)
            label = isinstance(decl, ASG.Function) and decl.real_name or decl.name
            i = 0
            while i < len(label) - 1 and i < len(scope) and label[i] == scope[i]:
                i += 1
            self.write('</div>' * (len(scope) - i))
            scope = scope[:i]
            while i < len(label) - 1:
                scope = scope + (label[i],)
                if len(last) >= len(scope) and last[:len(scope)] == scope: div_bit = ''
                else: div_bit = label[i]+'<br/>\n'
                self.write('%s<div class="fileview-scope">'%div_bit)
                i += 1

            # Now print the actual item
            label = replace_spaces(escape(str(scope.prune(label))))
            title = '(%s)'%decl.type
            self.write(div('href',href(link, label, target='content', title=title)))
            # Store this name in case, f.ex, it's a class and the next item is
            # in that class scope
            last = name
        self.write('</div>\n' * len(scope))
        self.end_file()

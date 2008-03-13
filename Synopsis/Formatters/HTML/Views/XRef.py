#
# Copyright (C) 2000 Stephen Davies
# Copyright (C) 2000 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis import config
from Synopsis.Processor import Parameter
from Synopsis import ASG
from Synopsis.Formatters.TOC import TOC, Linker
from Synopsis.Formatters.HTML.View import View
from Synopsis.Formatters.HTML.Tags import *
from Synopsis.Formatters.HTML.DirectoryLayout import quote_name
from Synopsis.Formatters.XRef import *
import os, time

class XRef(View):
    """A module for creating views full of xref infos"""

    link_to_scope = Parameter(True, '')

    def register(self, frame):

        super(XRef, self).register(frame)
        self.__filename = None
        self.__title = None
        self.__toc = TOC(None)

        if self.processor.sxr_prefix is None: return

        self.icon = 'xref.png'
        share = config.datadir
        src = os.path.join(share, 'xref.png')
        self.directory_layout.copy_file(src, self.icon)

        # Add an entry for every xref
        for name in self.processor.ir.sxr.keys():
            page = self.processor.xref.get(name)
            file = self.directory_layout.special('xref%d'%page)
            file += '#' + quote_name(str(name))
            self.__toc.insert(TOC.Entry(name, file, 'C++', 'xref'))

    def toc(self, start):

        return self.__toc

    def filename(self):

        return self.__filename

    def title(self):

        return self.__title

    def process(self):

        if self.processor.sxr_prefix is None: return

        pages = self.processor.xref.pages()
        if not pages: return
        for p in range(len(pages)):
            self.__filename = self.directory_layout.xref(p)

            first, last = pages[p][0], pages[p][-1]
            self.__title = 'Cross Reference : %s - %s'%(first, last)

            self.start_file()
            self.write_navigation_bar()
            self.write(element('h1', self.title()))
            for name in pages[p]:
                self.write('<div class="xref-name">')
                self.process_name(name)
                self.write('</div>')
            self.end_file()

    def register_filenames(self):
        """Registers each view"""

        if self.processor.sxr_prefix is None: return

        pages = self.processor.xref.pages()
        if not pages: return
        for p in range(len(pages)):
            filename = self.directory_layout.special('xref%d'%p)
            self.processor.register_filename(filename, self, p)
    
    def process_link(self, file, line, scope):
        """Outputs the info for one link"""

        file_link = self.directory_layout.file_source(file)
        file_link = rel(self.filename(), file_link) + "#%d"%line
        desc = ''
        type = self.processor.ir.types.get(scope)
        if isinstance(type, ASG.Declared):
            desc = ' ' + type.declaration.type
        entry = self.processor.toc[scope]
        if entry:
            label = href(rel(self.filename(), entry.link), escape(str(scope)))
        else:
            label = escape(str(scope))
        self.write('<li><a href="%s">%s:%s</a>: in%s %s</li>\n'%(
            file_link, file, line, desc, label))
    
    def describe_declaration(self, decl):
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
      
        entry = self.processor.ir.sxr.get(name)
        if not entry: return
        self.write(element('a', '', name=quote_name(escape(str(name)))))
        desc = ''
        decl = None
        type = self.processor.ir.types.get(name)
        if isinstance(type, ASG.Declared):
            decl = type.declaration
            desc = self.describe_declaration(decl)
        self.write(element('h2', desc + escape(str(name))) + '<ul>\n')
	
        if self.link_to_scope:
            type = self.processor.ir.types.get(name, None)
            if isinstance(type, ASG.Declared):
                link = self.directory_layout.link(type.declaration)
                self.write('<li>'+href(rel(self.__filename, link), 'Documentation')+'</li>')
        if entry.definitions:
            self.write('<li>Defined at:\n<ul>\n')
            for file, line, scope in entry.definitions:
                self.process_link(file, line, scope)
            self.write('</ul></li>\n')
        if entry.calls:
            self.write('<li>Called from:\n<ul>\n')
            for file, line, scope in entry.calls:
                self.process_link(file, line, scope)
            self.write('</ul></li>\n')
        if entry.references:
            self.write('<li>Referenced from:\n<ul>\n')
            for file, line, scope in entry.references:
                self.process_link(file, line, scope)
            self.write('</ul></li>\n')
        if isinstance(decl, ASG.Scope):
            self.write('<li>Declarations:\n<ul>\n')
            for child in decl.declarations:
                if isinstance(child, ASG.Builtin) and child.type == 'EOS':
                    continue
                file, line = child.file.name, child.line
                file_link = self.directory_layout.file_source(file)
                file_link = rel(self.filename(),file_link) + '#%d'%line
                file_href = '<a href="%s">%s:%s</a>: '%(file_link,file,line)
                cname = child.name
                entry = self.processor.toc[cname]
                type = self.describe_declaration(child)
                if entry:
                    link = href(rel(self.filename(), entry.link), escape(str(name.prune(cname))))
                    self.write(element('li', file_href + type + link))
                else:
                    self.write(element('li', file_href + type + str(name.prune(cname))))
            self.write('</ul>\n</li>\n')
        self.write('</ul>\n')

    def end_file(self):
        """Overrides end_file to provide synopsis logo"""

        self.write('\n')
        now = time.strftime(r'%c', time.localtime(time.time()))
        logo = img(src=rel(self.filename(), 'synopsis.png'), alt='logo', border='0')
        logo = href('http://synopsis.fresco.org', logo + ' synopsis', target='_blank')
        logo += ' (version %s)'%config.version
        self.write(div('logo', 'Generated on ' + now + ' by \n<br/>\n' + logo))
        View.end_file(self)

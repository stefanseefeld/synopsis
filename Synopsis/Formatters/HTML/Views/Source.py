#
# Copyright (C) 2000 Stephen Davies
# Copyright (C) 2000 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis import config
from Synopsis.Processor import *
from Synopsis.QualifiedName import *
from Synopsis.Formatters import join_paths
from Synopsis.Formatters.HTML.View import View
from Synopsis.Formatters.HTML.Tags import *
from xml.dom.minidom import parse
import os, urllib, time

class SXRTranslator:
    """Read in an sxr file, resolve references, and write it out as
    part of a Source view."""

    def __init__(self, filename, language, debug):

        try:
            self.sxr = parse(filename)
        except:
            if debug:
                print 'Error parsing', filename
                raise
            else:
                raise InternalError('parsing %s'%filename)
        if language == 'Python':
            self.qname = lambda name: QualifiedPythonName(str(name).split('.'))
        else:
            self.qname = lambda name: QualifiedCxxName(str(name).split('::'))

    def link(self, linker):

        for a in self.sxr.getElementsByTagName('a'):
            ref = self.qname(a.getAttribute('href'))
            target = linker(ref)
            a.setAttribute('href', target)
            if a.hasAttribute('type'):
                a.removeAttribute('type')
            if a.hasAttribute('from'):
                a.removeAttribute('from')
            if a.hasAttribute('continuation'):
                a.removeAttribute('continuation')

    def translate(self, writer):

        writer.write('<pre class="sxr">')
        lines = self.sxr.getElementsByTagName('line')
        lineno_template = '%%%ds' % len(`len(lines)`)
        for lineno, line in enumerate(lines):
            writer.write('<a id="line%d"></a>'%(lineno + 1))
            text = lineno_template % (lineno + 1)
            writer.write('<span class="lineno">%s</span>'%text)
            text = ''.join([n.toxml() for n in line.childNodes])
            if text:
                writer.write('<span class="line">%s</span>\n'%text)
            else:
                writer.write('\n')
        writer.write('</pre>')


class Source(View):
    """A module for creating a view for each file with hyperlinked source"""

    external_url = Parameter(None, 'base url to use for external links (if None the toc will be used')
   
    def register(self, frame):

        super(Source, self).register(frame)
        self.icons = {}
        if self.processor.sxr_prefix:
            share = config.datadir
            self.icons['C'] = 'src-c.png'
            self.icons['C++'] = 'src-c++.png'
            self.icons['Python'] = 'src-py.png'
            for l in self.icons:
                src = os.path.join(share, self.icons[l])
                self.directory_layout.copy_file(src, self.icons[l])


    def filename(self):

        return self.__filename


    def title(self):

        return self.__title


    def process(self):
        """Creates a view for every file"""

        self.prefix = self.processor.sxr_prefix
        if self.prefix is None: return
        
        # Get the TOC
        self.__toc = self.processor.toc
        # create a view for each primary file
        for file in self.processor.ir.files.values():
            if file.annotations['primary']:
                self.process_node(file)


    def register_filenames(self):
        """Registers a view for every source file"""

        if self.processor.sxr_prefix is None: return

        for file in self.processor.ir.files.values():
            if file.annotations['primary']:
                filename = file.name
                filename = self.directory_layout.file_source(filename)
                self.processor.register_filename(filename, self, file)

	     
    def process_node(self, file):
        """Creates a view for the given file"""

        # Start view
        filename = file.name
        self.__filename = self.directory_layout.file_source(filename)
        self.rel_url = rel(self.filename(), '')
        
        source = file.name
        self.__title = source

        self.start_file()
        self.write_navigation_bar()
        self.write('File: '+element('b', self.__title))

        sxr = join_paths(self.prefix, source + '.sxr')
        if os.path.exists(sxr):
            translator = SXRTranslator(sxr, file.annotations['language'], self.processor.debug)
            linker = self.external_url and self.external_ref or self.lookup_symbol
            translator.link(linker)
            translator.translate(self)
        self.end_file()


    def lookup_symbol(self, name):

        e = self.__toc.lookup(name)
        return e and self.rel_url + e.link or ''


    def external_ref(self, name):

        return self.external_url + urllib.quote(str(name))

    def end_file(self):
        """Overrides end_file to provide synopsis logo"""

        self.write('\n')
        now = time.strftime(r'%c', time.localtime(time.time()))
        logo = img(src=rel(self.filename(), 'synopsis.png'), alt='logo')
        logo = href('http://synopsis.fresco.org', logo + ' synopsis', target='_blank')
        logo += ' (version %s)'%config.version
        self.write(div('Generated on ' + now + ' by \n<br/>\n' + logo, class_='logo'))
        View.end_file(self)

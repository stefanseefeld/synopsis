#
# Copyright (C) 2000 Stephen Davies
# Copyright (C) 2000 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis import config
from Synopsis.Processor import Parameter
from Synopsis.QualifiedName import *
from Synopsis.Formatters.HTML.View import View
from Synopsis.Formatters.HTML.Tags import *
from xml.dom.minidom import parse
import os, urllib, time

class SXRTranslator:
    """Read in an sxr file, resolve references, and write it out as
    part of a Source view."""

    def __init__(self, filename, language):

        self.sxr = parse(filename)
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

    def translate(self, writer):

        writer.write('<pre class="sxr">')
        lines = self.sxr.getElementsByTagName('line')
        lineno_template = '%%%ds' % len(`len(lines)`)
        for lineno, line in enumerate(lines):
            writer.write('<a name="%d"></a>'%(lineno + 1))
            text = lineno_template % (lineno + 1)
            writer.write('<span class="lineno">%s</span>'%text)
            text = ''.join([n.toxml() for n in line.childNodes])
            writer.write('<span class="line">%s</span>\n'%text)
        writer.write('</pre>')


class Source(View):
    """A module for creating a view for each file with hyperlinked source"""

    prefix = Parameter('', 'prefix to the syntax files')
    external_url = Parameter(None, 'base url to use for external links (if None the toc will be used')
   
    def filename(self):

        return self.__filename


    def title(self):

        return self.__title


    def process(self):
        """Creates a view for every file"""

        # Get the TOC
        self.__toc = self.processor.toc
        # create a view for each primary file
        for file in self.processor.ir.files.values():
            if file.annotations['primary']:
                self.process_node(file)


    def register_filenames(self):
        """Registers a view for every source file"""

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

        sxr = os.path.join(self.prefix, source + '.sxr')
        if os.path.exists(sxr):
            translator = SXRTranslator(sxr, file.annotations['language'])
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
        logo = img(src=rel(self.filename(), 'synopsis.png'), alt='logo', border='0')
        logo = href('http://synopsis.fresco.org', logo + ' synopsis', target='_blank')
        logo += ' (version %s)'%config.version
        self.write(div('logo', 'Generated on ' + now + ' by \n<br/>\n' + logo))
        View.end_file(self)

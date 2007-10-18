#
# Copyright (C) 2000 Stephen Davies
# Copyright (C) 2000 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.Processor import Parameter
from Synopsis import AST, Util
from Synopsis.Formatters.HTML.View import View
from Synopsis.Formatters.HTML.Tags import *
from xml.dom.minidom import parse
import os, urllib

link = None
try:
    link = Util._import("Synopsis.Parsers.Cxx.link")
except ImportError:
    print "Warning: unable to import link module. Continuing..."


class SXRTranslator:
    """Read in an sxr file, resolve references, and write it out as
    part of a Source view."""

    def __init__(self, filename):

        self.sxr = parse(filename)

    def link(self, linker):

        for a in self.sxr.getElementsByTagName('a'):
            ref = a.getAttribute('href').split('.')
            target = linker(ref)
            a.setAttribute('href', target)


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
        self.write('File: '+entity('b', self.__title))

        sxr = os.path.join(self.prefix, source + '.sxr')
        if os.path.exists(sxr):
            translator = SXRTranslator(sxr)
            linker = self.external_url and self.external_ref or self.lookup_symbol
            translator.link(linker)
            translator.translate(self)
        elif not link:
            # No link module..
            self.write('link module for highlighting source unavailable')
            try:
                self.write(open(file.abs_name,'r').read())
            except IOError, e:
                self.write("An error occurred:"+ str(e))
        else:
            # Call link module
            f_out = os.path.join(self.processor.output, self.__filename) + '-temp'
            f_in = file.abs_name
            f_link = os.path.join(self.prefix, source)
            try:
                lookup = self.external_url and self.external_ref or self.lookup_symbol
                link.link(lookup, f_in, f_out, f_link)

            except link.error, msg:
                print "An error occurred:",msg

            try:
                self.write(open(f_out,'r').read())
                os.unlink(f_out)

            except IOError, e:
                self.write("An error occurred:"+ str(e))

        self.end_file()


    def lookup_symbol(self, name):

        e = self.__toc.lookup(tuple(name))
        return e and self.rel_url + e.link or ''


    def external_ref(self, name):

        return self.external_url + urllib.quote('::'.join(name))

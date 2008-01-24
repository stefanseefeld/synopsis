#
# Copyright (C) 2004 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

"""The SXR Facade around the HTML Formatter """

from Synopsis import config
from Synopsis.Processor import Processor, Parameter
from Synopsis.Processors.XRefCompiler import XRefCompiler
import HTML
from HTML.View import View, Template
from HTML.Views import Directory, Source, RawFile
import os, os.path
from shutil import copyfile

class SXRIndex(View):
    """Top level Index View. This is the starting point for the SXR browser."""

    sxr_cgi = Parameter('sxr.cgi', 'URL to use for sxr.cgi script')

    def filename(self):

        return self.directory_layout.index()

    def title(self):

        return 'Index'

    def root(self):

        return self.filename(), self.title()

    def process(self):
        """Recursively visit each directory below the base path given in the
        config."""

        self.start_file()

        self.write("""
<table class="form">
  <tr>
    <td>Click here to start browsing at the root of the directory tree:</td>
    <td>
      <a href="dir.html">/</a>
    </td>
  </tr>
  <tr>
    <td>Enter a file name to search:</td>
    <td>
      <form method="get" action="%(script)s/file">
        <input type="text" name="string" value="" size="15"/>
        <input type="submit" value="Find"/>
      </form>
    </td>
  </tr>
  <tr>
    <td>Enter a variable, type, or function name to search:</td>
    <td>
      <form method="get" action="%(script)s/ident">
        <input type="text" name="string" value="" size="15"/>
        <input type="submit" value="Find"/>
      </form>
    </td>
  </tr>
</table>
"""%{'script' : self.sxr_cgi})
        self.end_file()

class Formatter(Processor):
    """This is a facade to the HTML.Formatter. It adds an 'url' parameter and
    dispatches it to various 'views'."""

    title = Parameter('Synopsis - Cross-Reference', 'title to put into html header')
    url = Parameter('/sxr.cgi', 'the base url to use for the sxr cgi')
    syntax_prefix = Parameter(None, 'path prefix (directory) to contain syntax info')
    xref_prefix = Parameter(None, 'path prefix (directory) to contain xref info')
    src_dir = Parameter('', 'starting point for directory listing')
    exclude = Parameter([], 'TODO: define an exclusion mechanism (glob based ?)')
    sxr_template = Parameter(os.path.join(config.datadir, 'sxr-template.html'), 'html template to be used by the sxr.cgi script')
    stylesheet = Parameter(os.path.join(config.datadir, 'html.css'), 'stylesheet to be used')

    def process(self, ir, **kwds):

        self.set_parameters(kwds)
        self.ir = self.merge_input(ir)

        if not os.path.exists(self.output): os.makedirs(self.output)

        xref = XRefCompiler(prefix = self.xref_prefix)
        self.ir = xref.process(self.ir, output = os.path.join(self.output, 'xref_db'))

        content = [SXRIndex(sxr_cgi = self.url,
                            template = Template(template = self.sxr_template)),
                   Directory(src_dir = self.src_dir,
                             base_path = self.src_dir,
                             exclude = self.exclude,
                             template = Template(template = self.sxr_template)),
                   Source(prefix = self.syntax_prefix,
                          external_url = '%s/ident?full=1&string='%self.url,
                          template = Template(template = self.sxr_template)),
                   RawFile(src_dir = self.src_dir,
                           base_path = self.src_dir,
                           exclude = self.exclude,
                           template = Template(template = self.sxr_template))]
        
        html = HTML.Formatter(index = [],
                              detail = [],
                              content = content,
                              stylesheet = self.stylesheet)
        self.ir = html.process(self.ir, output = self.output)
        
        if self.sxr_template:
            copyfile(self.sxr_template,
                     os.path.join(self.output, 'sxr-template.html'))
            
        return self.ir

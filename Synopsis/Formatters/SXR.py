#
# Copyright (C) 2004 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

"""The SXR Facade around the HTML Formatter """

from Synopsis import config
from Synopsis import AST
from Synopsis.Processor import Processor, Parameter
from Synopsis.Processors.XRefCompiler import XRefCompiler
import HTML
from HTML.View import View, Template
from HTML.Views import DirBrowse, Source, RawFile
from HTML.FileLayout import NestedFileLayout
from HTML.TreeFormatterJS import TreeFormatterJS
import os, os.path
from shutil import copyfile

class SXRIndex(View):
   """Top level Index View. This is the starting point for the SXR browser."""

   sxr_cgi = Parameter('sxr.cgi', 'URL to use for sxr.cgi script')

   def filename(self): return self.processor.file_layout.index()

   def title(self): return 'Cross Reference'

   def register(self, processor):
      """Registers a view for each file in the hierarchy"""

      View.register(self, processor)

      processor.set_main_view(self.filename())
      processor.add_root_view(self.processor.file_layout.index(), 'Top', 'main', 2)

   def process(self, start):
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

   url = Parameter('/sxr.cgi', 'the base url to use for the sxr cgi')
   syntax_prefix = Parameter(None, 'path prefix (directory) to contain syntax info')
   xref_prefix = Parameter(None, 'path prefix (directory) to contain xref info')
   src_dir = Parameter('', 'starting point for directory listing')
   exclude = Parameter([], 'TODO: define an exclusion mechanism (glob based ?)')
   sxr_template = Parameter(os.path.join(config.datadir, 'sxr-template.html'), 'html template to be used by the sxr.cgi script')
   stylesheet = Parameter(os.path.join(config.datadir, 'html.css'), 'stylesheet to be used')

   def process(self, ast, **kwds):

      self.set_parameters(kwds)
      self.ast = self.merge_input(ast)

      if not os.path.exists(self.output): os.makedirs(self.output)

      xref = XRefCompiler(prefix = self.xref_prefix)
      self.ast = xref.process(self.ast, output = os.path.join(self.output, 'xref_db'))

      views = [SXRIndex(sxr_cgi = self.url,
                        template = Template(template = self.sxr_template)),
               DirBrowse(src_dir = self.src_dir,
                         base_path = self.src_dir,
                         exclude = self.exclude),
               Source(prefix = self.syntax_prefix,
                      external_url = '%s/ident?full=1&string='%self.url),
               RawFile(src_dir = self.src_dir,
                       base_path = self.src_dir,
                       exclude = self.exclude)]

      file_layout = NestedFileLayout()
      html = HTML.Formatter(file_layout = file_layout,
                            tree_formatter = TreeFormatterJS(),
                            views = views,
                            stylesheet = self.stylesheet)
      self.ast = html.process(self.ast, output = self.output)

      if self.sxr_template:
         copyfile(self.sxr_template,
                  os.path.join(self.output, 'sxr-template.html'))

      return self.ast

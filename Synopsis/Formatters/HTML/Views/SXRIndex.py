#
# Copyright (C) 2004 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.Processor import Parameter
from Synopsis import AST, Util
from Synopsis.Formatters.HTML.View import View
from Synopsis.Formatters.HTML.Tags import *

import os, stat, os.path, string, time

class SXRIndex(View):
   """"""

   sxr_cgi = Parameter('sxr.cgi', 'URL to use for sxr.cgi script')

   def filename(self): return self.processor.file_layout.index()

   def title(self): return 'Cross Reference'

   def register(self, processor):
      """Registers a view for each file in the hierarchy"""

      View.register(self, processor)

      processor.set_main_view(self.filename())

   def process(self, start):
      """Recursively visit each directory below the base path given in the
      config."""

      # Start the file
      self.start_file()

      self.write("""
<table>
  <tr>
    <td colspan="2">
      Click here to start browsing at the root of the directory tree:
    </td>
  </tr>
  <tr>
    <td>
      <a href="/dir.html">/</a>
    </td>
    <td>
    </td>
  </tr>
  <tr>
    <td colspan="2">
      Use this field to search for files by name:
    </td>
  </tr>
  <tr>
    <td valign="top">
      <a href="file">File Name Search</a>
    </td>
    <td>
      <form method="get" action="%(script)s/file">
        <input type="text" name="string" value="" size="15"/>
        <input type="submit" value="Find"/>
      </form>
    </td>
  </tr>
  <tr>
    <td colspan="2">
      Use this field to find a particular function, variable, etc.:
    </td>
  </tr>
  <tr>
    <td valign="top">
      <a href="ident">Identifier Search</a>
    </td>
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


    

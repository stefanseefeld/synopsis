# $Id: XRefLinker.py,v 1.3 2003/12/09 06:30:56 stefan Exp $
#
# Copyright (C) 2000 Stephen Davies
# Copyright (C) 2000 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.Formatters.HTML.Tags import *
from Default import Default
from Synopsis import Util

class XRefLinker(Default):
   """Adds an xref link to all declarations"""

   def register(self, formatter):

      Default.register(self, formatter)
      self.xref = self.processor.xref

   def format_declaration(self, decl):

      info = self.xref.get_info(decl.name())
      if not info:
         return ''
      view = self.xref.get_view_for(decl.name())
      filename = self.processor.file_layout.xref(view)
      filename = filename + "#" + Util.quote(string.join(decl.name(), '::'))
      return href(rel(self.formatter.filename(), filename), "[xref]")


# $Id: DetailCommenter.py,v 1.2 2003/12/08 00:39:24 stefan Exp $
#
# Copyright (C) 2000 Stephen Davies
# Copyright (C) 2000 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.Formatters.HTML.Tags import *
from Default import Default

class DetailCommenter(Default):
   """Adds summary comments to all declarations"""

   def format_declaration(self, decl):

      text = self.processor.comments.format(self.view, decl)
      if text: return desc(text)
      return ''


# $Id: SummaryCommenter.py,v 1.1 2003/12/05 22:31:53 stefan Exp $
#
# Copyright (C) 2000 Stephen Davies
# Copyright (C) 2000 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.Formatters.HTML.Tags import *
from Default import Default

class SummaryCommenter(Default):
   """Adds summary comments to all declarations"""

   def format_declaration(self, decl):
      summary = self.processor.comments.format_summary(self.page, decl)
      if summary:
         return '<br>'+span('summary', summary)
      return ''

   def format_group(self, decl):
      """Override for group to use the div version of commenting, and no
      <br> before"""

      summary = self.processor.comments.format(self.page, decl)
      if summary:
         return desc(summary)
      return ''
    
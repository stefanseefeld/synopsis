# $Id: QuoteHTML.py,v 1.2 2003/12/08 00:39:24 stefan Exp $
#
# Copyright (C) 2000 Stephen Davies
# Copyright (C) 2000 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Formatter import Formatter

class QuoteHTML(Formatter):
   """A formatter that quotes HTML characters like angle brackets and
   ampersand. Formats both text and summary."""
   
   def format(self, view, decl, text):
      """Replace angle brackets with HTML codes"""

      text = text.replace('&', '&amp;')
      text = text.replace('<', '&lt;')
      text = text.replace('>', '&gt;')
      return text

   def format_summary(self, view, decl, text):
      """Replace angle brackets with HTML codes"""

      text = text.replace('&', '&amp;')
      text = text.replace('<', '&lt;')
      text = text.replace('>', '&gt;')
      return text

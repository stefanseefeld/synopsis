#
# Copyright (C) 2000 Stephen Davies
# Copyright (C) 2000 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Formatter import Formatter
import re

class Section(Formatter):
   """A test formatter"""

   __re_break = '\n[ \t]*\n'

   def __init__(self):

      self.re_break = re.compile(Section.__re_break)

   def format(self, view, decl, text):

      if text is None: return text
      para = '</p>\n<p>'
      mo = self.re_break.search(text)
      while mo:
         start, end = mo.start(), mo.end()
         text = text[:start] + para + text[end:]
         end = start + len(para)
         mo = self.re_break.search(text, end)
      return '<p>%s</p>'%text

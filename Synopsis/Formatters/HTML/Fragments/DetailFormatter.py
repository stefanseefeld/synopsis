#
# Copyright (C) 2000 Stephen Davies
# Copyright (C) 2000 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from DeclarationFormatter import DeclarationFormatter
from Synopsis.Formatters.HTML.Tags import *

class DetailFormatter(DeclarationFormatter):
   """Provide detail-specific Declaration formatting."""

   col_sep = ' '
   row_sep = '<br/>'
   whole_row = ''

   def format_exceptions(self, oper):
      """Prints out the full exception spec"""

      if len(oper.exceptions()):
         raises = span("keyword", "raises")
         exceptions = []
         for exception in oper.exceptions():
            exceptions.append(self.reference(exception.name()))
         exceptions = span("raises", string.join(exceptions, ", "))
         return '%s (%s)'%(raises, exceptions)
      return ''

   def format_enum(self, enum):

      name = span("keyword", "enum ") + self.label(enum.name())
      start = '<div class="enum">'
      enumors = string.join(map(self.format_enumerator, enum.enumerators()))
      end = "</div>"
      return '%s%s%s%s'%(name, start, enumors, end)

   def format_enumerator(self, enumerator):

      text = self.label(enumerator.name())
      if len(enumerator.value()):
         value = " = " + span("value", enumerator.value())
      else: value = ''
      comments = self.processor.comments.format(self.view, enumerator)
      return '<div class="enumerator">%s%s%s</div>'%(text,value,comments)


#
# Copyright (C) 2000 Stephen Davies
# Copyright (C) 2000 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from DeclarationFormatter import DeclarationFormatter

class SummaryFormatter(DeclarationFormatter):
   """Derives from BaseStrategy to provide summary-specific methods.
   Currently the only one is format_exceptions"""

   def format_exceptions(self, oper):
      """Returns a reference to the detail if there are any exceptions."""
      if len(oper.exceptions()):
         return self.reference(oper.name(), " raises")
      return ''

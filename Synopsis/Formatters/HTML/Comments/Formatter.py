# $Id: Formatter.py,v 1.24 2003/12/04 21:04:27 stefan Exp $
#
# Copyright (C) 2000 Stephen Davies
# Copyright (C) 2000 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.Processor import Parametrized, Parameter
from Synopsis import AST, Type, Util

class Formatter(Parametrized):
   """Interface class that takes a comment and formats its summary and/or
   detail strings."""

   def init(self, processor):
      
      self.processor = processor

   def format(self, page, decl, text):
      """Format the given comment
      @param page the Page to use for references and determining the correct
      relative filename.
      @param decl the declaration
      @param text the comment text to format
      """

      return text
    
   def format_summary(self, page, decl, text):
      """Format the given comment summary
      @param page the Page to use for references and determining the correct
      relative filename.
      @param decl the declaration
      @param summary the comment summary to format
      """

      return text

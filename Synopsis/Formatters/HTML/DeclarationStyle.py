# $Id: DeclarationStyle.py,v 1.1 2003/11/15 19:55:06 stefan Exp $
#
# Copyright (C) 2000 Stephen Davies
# Copyright (C) 2000 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

class Style:
   """This class just maintains a mapping from declaration to display style.
   The style is an enumeration, possible values being: SUMMARY (only display
   a summary for this declaration), DETAIL (summary and detailed info),
   INLINE (summary and detailed info, where detailed info is an inline
   version of the declaration even if it's a class, etc.)"""

   SUMMARY = 0
   DETAIL = 1
   INLINE = 2
    
   def __init__(self):
      self.__dict = {}

   def style_of(self, decl):
      """Returns the style of the given decl"""
      SUMMARY = self.SUMMARY
      DETAIL = self.DETAIL
      key = id(decl)
      if self.__dict.has_key(key): return self.__dict[key]
      if len(decl.comments()) == 0:
         # Set to summary, as this will mean no detailed section
         style = SUMMARY
      else:
         comment = decl.comments()[0]
         # Calculate the style. The default is detail
         if not comment.text():
            # No comment, don't show detail
            style = SUMMARY
         elif comment.summary() != comment.text():
            # There is more to the comment than the summary, show detail
            style = DETAIL
         else:
            # Summary == Comment, don't show detail
            style = SUMMARY
	    # Always show tags
         if comment.tags():
            style = DETAIL
	    # Always show enums
         if isinstance(decl, AST.Enum):
            style = DETAIL
	    # Show functions if they have exceptions
         if isinstance(decl, AST.Function) and len(decl.exceptions()):
            style = DETAIL
	    # Don't show detail for scopes (they have their own pages)
         if isinstance(decl, AST.Scope):
            style = SUMMARY
      self.__dict[key] = style
      return style

   __getitem__ = style_of

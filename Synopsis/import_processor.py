#
# Copyright (C) 2003 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

def import_processor(name, verbose=False):
   """Import a named processor and return it.
   Throws ImportError on failure."""

   
   i = name.rfind('.')
   if i == -1:
      raise ImportError, '%s does not name a valid processor'%name

   module, processor = name[:i], name.split('.')

   mod = __import__(module)

   for c in processor[1:]:
      try:
         mod = getattr(mod, c)
      except AttributeError, msg:
         raise ImportError, "Unable to find %s in:\n%s"%(c, repr(mod))

   return mod

#
# Copyright (C) 2006 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

class DocString:
    """A doc-string for ASG nodes."""
   
    def __init__(self, text, markup):

       self.text = text
       self.markup = markup


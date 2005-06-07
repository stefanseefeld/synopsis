#
# Copyright (C) 2005 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Processor import Processor

class Filter(Processor):
    """Base class for comment filters.

    This is an AST visitor, and by default all declarations call process()
    with the current declaration. Subclasses may override just the process
    method.
    """
    def process_comments(self, decl):

        for comment in decl.comments():
            self.filter_comment(comment)

    def filter_comment(self, comment):
        """Filter comment."""
        pass

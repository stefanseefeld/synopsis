#
# Copyright (C) 2005 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis import AST
from Processor import Processor

class Summarizer(Processor):
    """Splits comments into summary/detail parts."""

    re_summary = r"[ \t\n]*(.*?\.)([ \t\n]|$)"

    def __init__(self):

        self.re_summary = re.compile(Summarizer.re_summary, re.S)

    def process_comments(self, decl):
        """Summarize the comment of this declaration."""

        comments = decl.comments()
        if not len(comments):
            return
        # Only use last comment
        comment = comments[-1]
        tags = comment.tags()
        # Decide how much of the comment is the summary
        text = comment.text()
        mo = self.re_summary.match(text)
        if mo:
            # Set summary to the sentence
            comment.set_summary(mo.group(1))
        else:
            # Set summary to whole text
            comment.set_summary(text)


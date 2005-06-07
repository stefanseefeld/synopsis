#
# Copyright (C) 2005 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis import AST
from Processor import Processor
import re

class JavaTags(Processor):
    """Extracts javadoc-style @tags from the end of comments."""

    _re_tags = '\n[ \t]*(?P<tags>@[a-zA-Z]+[ \t]+.*)'

    def __init__(self):

        self.re_tags = re.compile(self._re_tags,re.M|re.S)

    def process_comments(self, decl):
        """Extract tags from each comment of the given decl"""

        for comment in decl.comments():
            # Find tags
            text = comment.text()
            mo = self.re_tags.search(text)
            if not mo:
                continue
            # A lambda to use in the reduce expression
            joiner = lambda x,y: len(y) and y[0]=='@' and x+[y] or x[:-1]+[x[-1]+' '+y]

            tags = mo.group('tags')
            text = text[:mo.start('tags')]
            # Split the tag section into lines
            tags = [s.strip() for s in tags.split('\n')]
            # Join non-tag lines to the previous tag
            tags = reduce(joiner, tags, [])
            # Split the tag lines into @name, rest-of-line pairs
            tags = [line.split(' ', 1) for line in tags]
            # Convert the pairs into CommentTag objects
            tags = [AST.CommentTag(pair[0], pair[1]) for pair in tags]
            # Store back in comment
            comment.set_text(text)
            comment.tags().extend(tags)


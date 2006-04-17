#
# Copyright (C) 2006 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

class CommentTag:
    """Information about a tag in a comment. Tags can represent meta-information
    about a comment or extra attributes related to a declaration. For example,
    some tags can nominate a comment as belonging to another declaration,
    while others indicate information such as parameter and return type
    descriptions."""
   
    def __init__(self, name, text):
        """Constructor. Name is the name of tag, eg: 'class', 'param'. Text is
        the rest of the text for a tag."""

        self.name = name
        self.text = text
      

class Comment:
    """Information about a comment related to a declaration.
    The comments are extracted verbatim by the parsers, and various Linker
    CommentProcessors can select comments with appropriate formatting (eg: /**
    style comments, //. style comments, or all // style comments).
    The text field is text of the comment, less any tags that have been
    extracted.
    The summary field contains a summary of the comment, which may be equal to
    the comment text if there is no extra detail. The summary field is only
    set by Linker.Comments.Summarizer, which also ensures that there is only
    one comment for the declaration first.
    The list of tags in a comment can be extracted by a Linker
    CommentProcessor, or is an empty list if not set.
    C++ Comments may be suspect, which means that they were not immediately
    before a declaration, but the extract_tails option was set so they were
    kept for the Linker to deal with.
    """
    
    def __init__(self, text, file, line, suspect=0):

        self.file = file
        self.line = line
        self.text = text
        self.summary = None
        self.tags = []
        self.suspect = suspect

    def __str__(self):
        """Returns the text of the comment"""

        return self.__text

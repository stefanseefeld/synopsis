#
# Copyright (C) 2006 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

class Include:
    """Information about an include directive in a SourceFile.
    If the include directive required a macro expansion to get the filename,
    the is_macro will return true. If the include directive was actually an
    include_next, then is_next will return true.
    """
    
    def __init__(self, target, name, is_macro, is_next):

        self.target = target
        self.name = name
        self.is_macro = is_macro
        self.is_next = is_next


class MacroCall:
    """A class to support mapping from positions in a preprocessed file
    back to positions in the original file."""

    def __init__(self, name, start, end, diff):

        self.name = name
        self.start = start
        self.end = end
        self.diff = diff


class SourceFile:
    """The information about a file that the AST was generated from.
    Contains filename, all declarations from this file (even nested ones) and
    includes (aka imports) from this file."""

    def __init__(self, name, abs_name, language):
        """Constructor"""

        self.name = name
        self.abs_name = abs_name
        self.annotations = {'language':language, 'primary':False}
        self.includes = []
        self.declarations = []
        self.macro_calls = {}

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
        """The target SourceFile object being referenced."""
        self.name = name
        """The name by which the target is referenced."""
        self.is_macro = is_macro
        """True if the directive uses a macro."""
        self.is_next = is_next
        """True if this is using #include_next (GNU extension)."""


class MacroCall:
    """A class to support mapping from positions in a preprocessed file
    back to positions in the original file."""

    def __init__(self, name, start, end, diff):

        self.name = name
        self.start = start
        self.end = end
        self.diff = diff


class SourceFile:
    """The information about a file that the ASG was generated from.
    Contains filename, all declarations from this file (even nested ones) and
    includes (aka imports) from this file."""

    def __init__(self, name, abs_name, language, primary = False):
        """Constructor"""

        self.name = name
        """The filename."""
        self.abs_name = abs_name
        """The absolute filename."""
        self.annotations = {'language':language, 'primary':primary}
        """Dictionary with file annotations."""
        self.includes = []
        """List of includes this file contains."""
        self.declarations = []
        """List of declarations this file contains."""
        self.macro_calls = {}
        """Dictionary of macro calls this file contains."""


# $Id: __init__.py,v 1.8 2003/11/16 22:23:24 stefan Exp $
#
# Copyright (C) 2000 Stephen Davies
# Copyright (C) 2000 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

"""HTML Formatter package.

The HTML formatter consists of three broad groups: the core, the pages and the
utility classes. The core coordinates the page generation, and maintains the
config for the formatter, along with the utility classes in use. The pages can
be selected via the config to produce output tailored to the users needs.
Finally, the utility classes are those that are used by many pages, and
include things like the file layout, the table of contents, the tree formatter
to use, etc.
"""

from Formatter import Formatter

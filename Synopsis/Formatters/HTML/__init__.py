#
# Copyright (C) 2000 Stephen Davies
# Copyright (C) 2000 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

"""HTML Formatter package.

The HTML formatter consists of three broad groups: the core, the views and the
utility classes. The core coordinates the view generation, and maintains the
config for the formatter, along with the utility classes in use. The views can
be selected via the config to produce output tailored to the users needs.
Finally, the utility classes are those that are used by many views, and
include things like the file layout, the table of contents, the tree formatter
to use, etc.
"""

from Formatter import Formatter

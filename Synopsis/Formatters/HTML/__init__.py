# $Id: __init__.py,v 1.6 2001/07/04 08:17:48 uid20151 Exp $
#
# This file is a part of Synopsis.
# Copyright (C) 2000, 2001 Stephen Davies
# Copyright (C) 2000, 2001 Stefan Seefeld
#
# Synopsis is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA.
#
# $Log: __init__.py,v $
# Revision 1.6  2001/07/04 08:17:48  uid20151
# Comments
#
# Revision 1.5  2001/02/13 10:07:33  chalky
# Fixed typo
#
# Revision 1.4  2001/02/13 06:55:23  chalky
# Made synopsis -l work again
#
# Revision 1.3  2001/02/01 15:28:43  chalky
# Imported usage so it works.
#
# Revision 1.2  2001/02/01 15:23:24  chalky
# Copywritten brown paper bag edition.
#
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

#
# __init__.py
#
# The formatter has been split into a very modular format.
# The modules in the HTML package now contain the classes which may be
# overridden by user-specified modules

try:
    import core

    from core import format, usage

except:
    print "An error occurred loading the HTML module:"
    import traceback
    traceback.print_exc()
    raise

# THIS-IS-A-FORMATTER

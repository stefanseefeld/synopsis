# $Id: __init__.py,v 1.4 2001/02/13 06:55:23 chalky Exp $
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

"""HTML Formatter"""
#
# __init__.py
#
# The formatter has been split into a very modular format.
# The modules in the HTML package now contain the classes which may be
# overridden by user-specified modules

try:
    import core

    from core import usage

except:
    print "An error occurred loading the HTML module:"
    import traceback
    traceback.print_exc()
    raise

# THIS-IS-A-FORMATTER

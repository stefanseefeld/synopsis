# -*- python -*-
#                           Package   : omniidl
# idlutil.py                Created on: 1999/10/27
#			    Author    : Duncan Grisby (dpg1)
#
#    Copyright (C) 1999 AT&T Laboratories Cambridge
#
#  This file is part of omniidl.
#
#  omniidl is free software; you can redistribute it and/or modify it
#  under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#  General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
#  02111-1307, USA.
#
# Description:
#   
#   Utility functions

# $Id: Util.py,v 1.2 2001/01/21 06:22:51 chalky Exp $
# $Log: Util.py,v $
# Revision 1.2  2001/01/21 06:22:51  chalky
# Added Util.getopt_spec for --spec=file.spec support
#
# Revision 1.1  2001/01/08 19:48:41  stefan
# changes to allow synopsis to be installed
#
# Revision 1.2  2000/12/19 23:44:16  chalky
# Chalky's Big Commit. Loads of changes and new stuff:
# Rewrote HTML formatter so that it creates a page for each module and class,
# with summaries and details and sections on each page. It also creates indexes
# of modules, and for each module, and a frames index to organise them. Oh and
# an inheritance tree. Bug fixes to some other files. The C++ parser now also
# recognises class and functions to some extent, but support is not complete.
# Also wrote a DUMP formatter that verbosely dumps the AST and Types. Renamed
# the original HTML formatter to HTML_Simple. The ASCII formatter was rewritten
# to follow what looked like major changes to the AST ;) It now outputs
# something that should be the same as the input file, modulo whitespace and
# comments.
#
# Revision 1.1  2000/08/01 03:28:26  stefan
# a major rewrite, hopefully much more robust
#
# Revision 1.3  1999/11/15 15:49:23  dpg1
# Documentation strings.
#
# Revision 1.2  1999/11/01 20:18:30  dpg1
# Added string escaping
#
# Revision 1.1  1999/10/29 15:47:07  dpg1
# First revision.
#

"""Utility functions for IDL compilers

escapifyString() -- return a string with non-printing characters escaped.
slashName()      -- format a scoped name with '/' separating components.
dotName()        -- format a scoped name with '.' separating components.
ccolonName()     -- format a scoped name with '::' separating components.
pruneScope()     -- remove common prefix from a scoped name.
getopt_spec(args,options,longlist) -- version of getopt that adds transparent --spec= suppport"""

import string, getopt

def slashName(scopedName, our_scope=[]):
    """slashName(list, [list]) -> string

Return a scoped name given as a list of strings as a single string
with the components separated by '/' characters. If a second list is
given, remove a common prefix using pruneScope()."""
    
    pscope = pruneScope(scopedName, our_scope)
    return string.join(pscope, "/")

def dotName(scopedName, our_scope=[]):
    """dotName(list, [list]) -> string

Return a scoped name given as a list of strings as a single string
with the components separated by '.' characters. If a second list is
given, remove a common prefix using pruneScope()."""
    
    pscope = pruneScope(scopedName, our_scope)
    return string.join(pscope, ".")

def ccolonName(scopedName, our_scope=[]):
    """ccolonName(list, [list]) -> string

Return a scoped name given as a list of strings as a single string
with the components separated by '::' strings. If a second list is
given, remove a common prefix using pruneScope()."""
    
    pscope = pruneScope(scopedName, our_scope)
    return string.join(pscope, "::")

def pruneScope(target_scope, our_scope):
    """pruneScope(list A, list B) -> list

Given two lists of strings (scoped names), return a copy of list A
with any prefix it shares with B removed.

  e.g. pruneScope(['A', 'B', 'C', 'D'], ['A', 'B', 'D']) -> ['C', 'D']"""

    tscope = list(target_scope)
    i = 0
    while len(tscope) > 1 and \
          i < len(our_scope) and \
          tscope[0] == our_scope[i]:
        del tscope[0]
        i = i + 1
    return tscope

def escapifyString(str):
    """escapifyString(string) -> string

Return the given string with any non-printing characters escaped."""
    
    l = list(str)
    vis = string.letters + string.digits + " _!$%^&*()-=+[]{};'#:@~,./<>?|`"
    for i in range(len(l)):
        if l[i] not in vis:
            l[i] = "\\%03o" % ord(l[i])
    return string.join(l, "")

def splitAndStrip(line):
    """Splits a line at the first space, then strips the second argument"""
    pair = string.split(line, ' ', 1)
    return pair[0], string.strip(pair[1])

def getopt_spec(args, options, long_options=[]):
    """Transparently add --spec=file support to getopt"""
    long_options.append('spec=')
    opts, remainder = getopt.getopt(args, options, long_options)
    ret = []
    for pair in opts:
	if pair[0] == '--spec':
	    f = open(pair[1], 'rt')
	    spec_opts = map(splitAndStrip, f.readlines())
	    f.close()
	    ret.extend(spec_opts)
	else:
	    ret.append(pair)
    return ret, remainder

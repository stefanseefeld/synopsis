# $Id: Tags.py,v 1.11 2002/11/13 01:01:49 chalky Exp $
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
# $Log: Tags.py,v $
# Revision 1.11  2002/11/13 01:01:49  chalky
# Improvements to links when using the Nested file layout
#
# Revision 1.10  2002/11/02 06:37:37  chalky
# Allow non-frames output, some refactoring of page layout, new modules.
#
# Revision 1.9  2002/11/01 07:20:22  chalky
# Enhanced anglebrackets()
#
# Revision 1.8  2002/11/01 03:39:21  chalky
# Cleaning up HTML after using 'htmltidy'
#
# Revision 1.7  2002/07/11 09:28:40  chalky
# Missed one
#
# Revision 1.6  2002/07/11 02:03:49  chalky
# Oops, remove print that shouldn't be there.
#
# Revision 1.5  2002/07/04 06:43:18  chalky
# Improved support for absolute references - pages known their full path.
#
# Revision 1.4  2001/07/10 05:09:11  chalky
# More links work
#
# Revision 1.3  2001/06/28 07:22:18  stefan
# more refactoring/cleanup in the HTML formatter
#
# Revision 1.2  2001/02/01 15:23:24  chalky
# Copywritten brown paper bag edition.
#
#

"""HTML Tag generation utilities.
You will probably find it easiest to import * from this module."""

# System modules
import string, re

# HTML modules
import core

def k2a(keys):
    "Convert a name/value dict to a string of attributes"
    return string.join(map(lambda item:' %s="%s"'%item, keys.items()), '')
def rel(frm, to):
    "Find link to to relative to frm"
    frm = string.split(frm, '/'); to = string.split(to, '/')
    if len(frm) < len(to): check = len(frm)-1
    else: check = len(to)-1
    for l in range(check):
        if to[0] == frm[0]: del to[0]; del frm[0]
        else: break
    # If frm is a directory, and to is in that directory, frm[0] == to[0]
    if len(frm) == 1 and len(to) > 1 and frm[0] == to[0]:
	# Remove directory from to, but respect len(frm)-1 below
	del to[0]
    if frm: to = ['..'] * (len(frm) - 1) + to
    return string.join(to,'/')
def href(_ref, _label, **keys):
    "Return a href to 'ref' with name 'label' and attributes"
    # Remove target if not using frames
    if keys.has_key('target') and not core.config.using_frames:
	del keys['target']
    return '<a href="%s"%s>%s</a>'%(_ref,k2a(keys),_label)
def name(ref, label):
    "Return a name anchor with given reference and label"
    return '<a class="name" name="%s">%s</a>'%(ref,label)
def span(clas, body):
    "Wrap the body in a span of the given class"
    return '<span class="%s">%s</span>'%(clas,body)
def div(clas, body):
    "Wrap the body in a div of the given class"
    return '<div class="%s">%s</div>'%(clas,body)
def entity(_type, body, **keys):
    "Wrap the body in a tag of given type and attributes"
    return '<%s%s>%s</%s>'%(_type,k2a(keys),body,_type)
def solotag(_type, **keys):
    "Create a solo tag (no close tag) of given type and attributes"
    return '<%s%s>'%(_type,k2a(keys))
def desc(text):
    "Create a description div for the given text"
    return text and div("desc", text) or ''
def anglebrackets(text):
    """Replace angle brackets with HTML codes"""
    text = text.replace('&', '&amp;')
    text = text.replace('"', '&quot;')
    text = text.replace('<', '&lt;')
    text = text.replace('>', '&gt;')
    return text

def replace_spaces(text):
    """Replaces spaces in the given string with &nbsp; sequences. Does NOT
    replace spaces inside tags"""
    # original "hello <there stuff> fool <thing me bob>yo<a>hi"
    tags = string.split(text, '<')
    # now ['hello ', 'there stuff> fool ', 'thing me bob>yo', 'a>hi']
    tags = map(lambda x: string.split(x, '>'), tags)
    # now [['hello '], ['there stuff', ' fool '], ['thing me bob', 'yo'], ['a', 'hi']]
    tags = reduce(lambda x,y: x+y, tags)
    # now ['hello ', 'there stuff', ' fool ', 'thing me bob', 'yo', 'a', 'hi']
    for i in range(0,len(tags),2):
	tags[i] = tags[i].replace(' ', '&nbsp;')
    for i in range(1,len(tags),2):
	tags[i] = '<' + tags[i] + '>'
    return string.join(tags, '')



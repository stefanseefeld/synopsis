# $Id: Tags.py,v 1.2 2001/02/01 15:23:24 chalky Exp $
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
# Revision 1.2  2001/02/01 15:23:24  chalky
# Copywritten brown paper bag edition.
#
#

"""HTML Tag generation utilities.
You will probably find it easiest to import * from this module."""

# System modules
import string, re


def k2a(keys):
    "Convert a name/value dict to a string of attributes"
    return string.join(map(lambda item:' %s="%s"'%item, keys.items()), '')
def rel(frm, to):
    "Find link to to relative to frm"
    frm = string.split(frm, '/'); to = string.split(to, '/')
    for l in range((len(frm)<len(to)) and len(frm) or len(to)):
        if to[0] == frm[0]: del to[0]; del frm[0]
        else: break
    if len(frm) > len(to): to = ['..']*(len(frm)-len(to))+to
    return string.join(to,'/')
def href(ref, label, **keys):
    "Return a href to 'ref' with name 'label' and attributes"
    return '<a href="%s"%s>%s</a>'%(ref,k2a(keys),label)
def name(ref, label):
    "Return a name anchor with given reference and label"
    return '<a class="name" name="%s">%s</a>'%(ref,label)
def span(clas, body):
    "Wrap the body in a span of the given class"
    return '<span class="%s">%s</span>'%(clas,body)
def div(clas, body):
    "Wrap the body in a div of the given class"
    return '<div class="%s">%s</div>'%(clas,body)
def entity(type, body, **keys):
    "Wrap the body in a tag of given type and attributes"
    return '<%s%s>%s</%s>'%(type,k2a(keys),body,type)
def solotag(type, **keys):
    "Create a solo tag (no close tag) of given type and attributes"
    return '<%s%s>'%(type,k2a(keys))
def desc(text):
    "Create a description div for the given text"
    return text and div("desc", text) or ''
def anglebrackets(text):
    """Replace angle brackets with HTML codes"""
    return re.sub('<','&lt;',re.sub('>','&gt;',text))




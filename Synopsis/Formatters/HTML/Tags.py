# $Id: Tags.py,v 1.12 2003/11/15 19:55:31 stefan Exp $
#
# Copyright (C) 2000 Stephen Davies
# Copyright (C) 2000 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

"""HTML Tag generation utilities.
You will probably find it easiest to import * from this module."""

import string, re

using_frames = True #overwritten by Formatter...

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
   if keys.has_key('target') and not using_frames:
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



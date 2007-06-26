#
# Copyright (C) 2006 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

def attributes(keys):
    "Convert a name/value dict to a string of attributes."

    return ' '.join(['%s="%s"'%item for item in keys.iteritems()])


def rel(origin, target):
    "Find link to target relative to origin."

    origin = origin.split('/')
    target = target.split('/')
    check = len(origin) < len(target) and len(origin) - 1 or len(target) - 1
    for l in range(check):
        if target[0] == origin[0]: del target[0]; del origin[0]
        else: break
    # If origin is a directory, and target is in that directory,
    # origin[0] == target[0]
    if len(origin) == 1 and len(target) > 1 and origin[0] == target[0]:
        # Remove directory from target, but respect len(origin)-1 below
        del target[0]
    if origin: target = ['..'] * (len(origin) - 1) + target
    return '/'.join(target)

using_frames = False

def href(ref, label, **keys):
    "Return a href to 'ref' with name 'label' and attributes from 'keys'."

    # Remove target if not using frames
    if keys.has_key('target') and not using_frames:
        del keys['target']
    return '<a href="%s" %s>%s</a>'%(ref, attributes(keys), label)

def name(ref, label):
    "Return a name anchor with given reference and label."
    
    return '<a class="name" name="%s">%s</a>'%(ref, label)

def span(_class, body):
    "Wrap the body in a span of the given class."

    return '<span class="%s">%s</span>'%(_class, body)

def div(_class, body):
    "Wrap the body in a div of the given class."

    if _class: return '<div class="%s">%s</div>'%(_class, body)
    else: return '<div>%s</div>'%(body)

def element(_type, _body, **keys):
    "Wrap the body in a 'type' element using 'keys' as aruments."

    return '<%s %s>%s</%s>'%(_type, attributes(keys), _body, _type)

def empty(_type, **keys):
    "Create an empty '_type' element using 'keys' as arguments."

    return '<%s %s />'%(_type, attributes(keys))

def escape(text):

    for p in [('&', '&amp;'), ('"', '&quot;'), ('<', '&lt;'), ('>', '&gt;'),]:
        text = text.replace(*p)
    return text

def __replace_spaces(text):
   """Replaces spaces in the given string with &#160; sequences. Does NOT
   replace spaces inside tags"""

   # original "hello <there stuff> fool <thing me bob>yo<a>hi"
   tags = string.split(text, '<')
   # now ['hello ', 'there stuff> fool ', 'thing me bob>yo', 'a>hi']
   tags = map(lambda x: string.split(x, '>'), tags)
   # now [['hello '], ['there stuff', ' fool '], ['thing me bob', 'yo'], ['a', 'hi']]
   tags = reduce(lambda x,y: x+y, tags)
   # now ['hello ', 'there stuff', ' fool ', 'thing me bob', 'yo', 'a', 'hi']
   for i in range(0,len(tags),2):
      tags[i] = tags[i].replace(' ', '&#160;')
   for i in range(1,len(tags),2):
      tags[i] = '<' + tags[i] + '>'
   return string.join(tags, '')



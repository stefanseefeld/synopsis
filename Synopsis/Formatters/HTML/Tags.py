#
# Copyright (C) 2000 Stephen Davies
# Copyright (C) 2000 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

"""HTML Tag generation utilities.
You will probably find it easiest to import * from this module."""

using_frames = True #overwritten by Formatter...

def attributes(keys):
    "Convert a name/value dict to a string of attributes"

    return ' '.join(['%s="%s"'%(k,v) for k,v in keys.items()])

def rel(origin, target):
    "Find link to target relative to origin."

    origin, target = origin.split('/'), target.split('/')
    if len(origin) < len(target): check = len(origin) - 1
    else: check = len(target) - 1
    for l in range(check):
        if target[0] == origin[0]:
            del target[0]
            del origin[0]
        else: break
    # If origin is a directory, and target is in that directory, origin[0] == target[0]
    if len(origin) == 1 and len(target) > 1 and origin[0] == target[0]:
        # Remove directory from target, but respect len(origin) - 1 below
        del target[0]
    if origin: target = ['..'] * (len(origin) - 1) + target
    return '/'.join(target)

def href(_ref, _label, **attrs):
    "Return a href to 'ref' with name 'label' and attributes"

    # Remove target if not using frames
    if attrs.has_key('target') and not using_frames:
        del attrs['target']
    return '<a href="%s" %s>%s</a>'%(_ref,attributes(attrs),_label)

def img(**attrs):
    "Return an img element."

    return '<img %s/>'%attributes(attrs)

def name(ref, label):
    "Return a name anchor with given reference and label"

    return '<a class="name" name="%s">%s</a>'%(ref,label)

def span(class_, body):
    "Wrap the body in a span of the given class"

    if class_:
        return '<span class="%s">%s</span>'%(class_,body)
    else:
        return '<span>%s</span>'%body

def div(class_, body):
    "Wrap the body in a div of the given class"

    if class_:
        return '<div class="%s">%s</div>'%(class_,body)
    else:
        return '<div>%s</div>'%body

def element(_, body = None, **keys):
    "Wrap the body in a tag of given type and attributes"

    if body:
        return '<%s %s>%s</%s>'%(_, attributes(keys), body, _)
    else:
        return '<%s %s />'%(_, attributes(keys))

def desc(text):
    "Create a description div for the given text"

    return text and div("desc", text) or ''


def escape(text):

    for p in [('&', '&amp;'), ('"', '&quot;'), ('<', '&lt;'), ('>', '&gt;'),]:
        text = text.replace(*p)
    return text


def replace_spaces(text):
    """Replaces spaces in the given string with &#160; sequences. Does NOT
    replace spaces inside tags"""

    # original "hello <there stuff> fool <thing me bob>yo<a>hi"
    tags = text.split('<')
    # now ['hello ', 'there stuff> fool ', 'thing me bob>yo', 'a>hi']
    tags = [x.split('>') for x in tags]
    # now [['hello '], ['there stuff', ' fool '], ['thing me bob', 'yo'], ['a', 'hi']]
    tags = reduce(lambda x,y: x+y, tags)
    # now ['hello ', 'there stuff', ' fool ', 'thing me bob', 'yo', 'a', 'hi']
    for i in range(0,len(tags),2):
        tags[i] = tags[i].replace(' ', '&#160;')
    for i in range(1,len(tags),2):
        tags[i] = '<' + tags[i] + '>'
    return ''.join(tags)

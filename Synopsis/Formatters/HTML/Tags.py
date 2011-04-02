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

def rel(origin, target):
    "Find link to target relative to origin URL."

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

def attributes(keys):
    """Convert a name/value dict to a string of attributes.
    A common HTML attribute is 'class'. Since 'class' is a Python keyword,
    we accept 'class_' instead, and translate that to 'class'."""

    if 'class_' in keys:
        keys['class'] = keys['class_']
        del keys['class_']
    # Remove target if not using frames
    if 'target' in keys and not using_frames:
        del keys['target']
    return ' '.join(['%s="%s"'%(k,v) for k,v in keys.items()])

def element(_, body = None, **keys):
    "Wrap the body in a tag of given type and attributes"

    attrs = attributes(keys)
    if body != None:
        if attrs:
            return '<%s %s>%s</%s>'%(_, attrs, body, _)
        else:
            return '<%s>%s</%s>'%(_, body, _)
    else:
        if attrs:
            return '<%s %s/>'%(_, attrs)
        else:
            return '<%s/>'%(_)

def href(ref, label, **attrs): return element('a', label, href=ref, **attrs)
def img(**attrs): return element('img', **attrs)
def name(ref, label): return element('a', label, class_='name', id=ref)
def span(body, **attrs): return element('span', body, **attrs)
def div(body, **attrs): return element('div', body, **attrs)
def para(body, **attrs): return element('p', body, **attrs)

def desc(text):
    "Create a description div for the given text"

    return text and div("desc", text) or ''


def escape(text):

    for p in [('&', '&amp;'), ('"', '&quot;'), ('<', '&lt;'), ('>', '&gt;'),
              ('(', '&#40;'), (')', '&#41;'), (',', '&#44;'), (',', '&#59;')]:
        text = text.replace(*p)
    return text

def quote_as_id(text):

    for p in [(' ', '.'),
              ('<', '_L'), ('>', '_R'), ('(', '_l'), (')', '_r'), ('::', '-'), ('~', '_t'),
              (':', '.'), ('&', '_A'), ('*', '_S'), (' ', '_s'), (',', '_c'), (';', '_C'),
              ('!', '_n'), ('[', '_b'), (']', '_B'), ('=', '_e'), ('+', '_p'), ('-', '_m')]:
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

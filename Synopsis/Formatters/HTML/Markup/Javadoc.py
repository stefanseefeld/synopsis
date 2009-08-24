#
# Copyright (C) 2006 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.Formatters.HTML.Tags import *
from Synopsis.Formatters.HTML.Markup import *
import re

class Javadoc(Formatter):
    """
    A formatter that formats comments similar to Javadoc.
    See `Javadoc Spec`_ for info.

    .. _Javadoc Spec: http://java.sun.com/j2se/1.5.0/docs/tooldocs/solaris/javadoc.html"""

    class Block:

        def __init__(self, tag, arg, body):
            self.tag, self.arg, self.body = tag, arg, body
            

    summary = r'(\s*[\w\W]*?\.)(\s|$)'
    block_tag = r'(^\s*\@\w+[\s$])'
    inline_tag = r'{@(?P<tag>\w+)\s+(?P<content>[^}]+)}'
    inline_tag_split = r'({@\w+\s+[^}]+})'
    xref = r'([\w#.]+)(?:\([^\)]*\))?\s*(.*)'

    tag_name = {
    'author': ['Author', 'Authors'],
    'date': ['Date', 'Dates'],
    'deprecated': ['Deprecated', 'Deprecated'],
    'exception': ['Exception', 'Exceptions'],
    'invariant': ['Invariant', 'Invariants'],
    'keyword': ['Keyword', 'Keywords'],
    'param': ['Parameter', 'Parameters'],
    'postcondition': ['Postcondition', 'Postcondition'],
    'precondition': ['Precondition', 'Preconditions'],
    'return': ['Returns', 'Returns'],
    'see': ['See also', 'See also'],
    'throws': ['Throws', 'Throws'],
    'version': ['Version', 'Versions']}
    arg_tags = ['param', 'keyword', 'exception']


    def __init__(self):
        """Create regex objects for regexps"""

        self.summary = re.compile(Javadoc.summary)
        self.block_tag = re.compile(Javadoc.block_tag, re.M)
        self.inline_tag = re.compile(Javadoc.inline_tag)
        self.inline_tag_split = re.compile(Javadoc.inline_tag_split)
        self.xref = re.compile(Javadoc.xref)


    def split(self, doc):
        """Split a javadoc comment into description and blocks."""

        chunks = self.block_tag.split(doc)
        description = chunks[0]
        blocks = []
        for i in range(1, len(chunks)):
            if i % 2 == 1:
                tag = chunks[i].strip()[1:]
            else:
                if tag in self.arg_tags:
                    arg, body = chunks[i].strip().split(None, 1)
                else:
                    arg, body = None, chunks[i]

                if tag == 'see' and body:
                    if body[0] in ['"', "'"]:
                        if body[-1] == body[0]:
                            body = body[1:-1]
                    elif body[0] == '<':
                        pass
                    else:
                        # @link tags are interpreted as cross-references
                        #       and resolved later (see format_inline_tag)
                        body = '{@link %s}'%body
                blocks.append(Javadoc.Block(tag, arg, body))
        
        return description, blocks


    def extract_summary(self, description):
        """Generate a summary from the given description."""

        m = self.summary.match(description)
        if m:
            return m.group(1)
        else:
            return description.split('\n', 1)[0]+'...'


    def format(self, decl, view):
        """Format using javadoc markup."""

        doc = decl.annotations.get('doc')
        doc = doc and doc.text or ''
        if not doc:
            return Struct('', '')
        description, blocks = self.split(doc)

        details = self.format_description(description, view, decl)
        summary = self.extract_summary(details)
        details += self.format_params(blocks, view, decl)
        details += self.format_tag('return', blocks, view, decl)
        details += self.format_throws(blocks, view, decl)
        details += self.format_tag('precondition', blocks, view, decl)
        details += self.format_tag('postcondition', blocks, view, decl)
        details += self.format_tag('invariant', blocks, view, decl)
        details += self.format_tag('author', blocks, view, decl)
        details += self.format_tag('date', blocks, view, decl)
        details += self.format_tag('version', blocks, view, decl)
        details += self.format_tag('deprecated', blocks, view, decl)
        details += self.format_tag('see', blocks, view, decl)

        return Struct(summary, details)


    def format_description(self, text, view, decl):

        return self.format_inlines(view, decl, text)


    def format_inlines(self, view, decl, text):
        """Formats inline tags in the text."""

        chunks = self.inline_tag_split.split(text)
        text = ''
        # Every other chunk is now an inlined tag, which we process
        # in this loop.
        for i in range(len(chunks)):
            if i % 2 == 0:
                text += chunks[i]
            else:
                m = self.inline_tag.match(chunks[i])
                if m:
                    text += self.format_inline_tag(m.group('tag'),
                                                   m.group('content'),
                                                   view, decl)
        return text


    def format_params(self, blocks, view, decl):
        """Formats a list of (param, description) tags"""

        content = ''
        params = [b for b in blocks if b.tag == 'param']
        def row(dt, dd):
            return element('tr',
                           element('th', dt, class_='dt') +
                           element('td', dd, class_='dd'))
        if params:
            content += div('Parameters:', class_='tag-heading')
            dl = element('table', ''.join([row(p.arg, p.body) for p in params]),
                         class_='dl')
            content += div(dl, class_='tag-section')
        kwds = [b for b in blocks if b.tag == 'keyword']
        if kwds:
            content += div('Keywords:', class_='tag-heading')
            dl = element('dl', ''.join([row( k.arg, k.body) for k in kwds]), class_='dl')
            content += div(dl, class_='tag-section')
        return content


    def format_throws(self, blocks, view, decl):

        content = ''
        throws = [b for b in blocks if b.tag in ['throws', 'exception']]
        if throws:
            content += div('Throws:', class_='tag-heading')
            dl = element('dl', ''.join([element('dt', t.arg) + element('dd', t.body)
                                        for t in throws]))
            content += div(dl, class_='tag-section')
        return content


    def format_tag(self, tag, blocks, view, decl):

        content = ''
        items = [b for b in blocks if b.tag == tag]
        if items:
            content += div(self.tag_name[tag][1], class_='tag-heading')
            content += div('<br/>'.join([self.format_inlines(view, decl, i.body)
                                         for i in items]),
                           class_='tag-section')
        return content

    def format_inline_tag(self, tag, content, view, decl):

        text = ''
        if tag == 'link':
            xref = self.xref.match(content)
            name, label = xref.groups()
            if not label:
                label = name
            # javadoc uses '{@link  package.class#member  label}'
            # Here we simply replace '#' by either '::' or '.' to match
            # language-specific qualification rules.
            if '#' in name:
                if '::' in name:
                    name = name.replace('#', '::')
                else:
                    name = name.replace('#', '.')
            target = self.lookup_symbol(name, decl.name[:-1])
            if target:
                url = rel(view.filename(), target)
                text += href(url, label)
            else:
                text += label
        elif tag == 'code':
            text += '<code>%s</code>'%escape(content)
        elif tag == 'literal':
            text += '<code>%s</code>'%escape(content)

        return text

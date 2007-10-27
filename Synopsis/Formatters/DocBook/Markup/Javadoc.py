#
# Copyright (C) 2006 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.Formatters.DocBook.Markup import *
import re

class Javadoc(Formatter):
    """A formatter that formats comments similar to Javadoc.
    @see <a href="http://java.sun.com/j2se/1.5.0/docs/tooldocs/solaris/javadoc.html">Javadoc Spec</a>"""

    class Block:

        def __init__(self, tag, arg, body):
            self.tag, self.arg, self.body = tag, arg, body
            

    summary = r'(\s*[\w\W]*?\.)(\s|$)'
    block_tags = r'(^\s*\@\w+[\s$])'
    link_split = r'({@link(?:plain)?\s[^}]+})'
    link = r'{@link(?:plain)?\s+([\w#.]+)(?:\([^\)]*\))?(\s+.*)?}'

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
        self.block_tags = re.compile(Javadoc.block_tags, re.M)
        self.link_split = re.compile(Javadoc.link_split)
        self.link = re.compile(Javadoc.link)


    def split(self, doc):
        """Split a javadoc comment into description and blocks."""

        chunks = self.block_tags.split(doc)
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


    def format(self, decl):
        """Format using javadoc markup."""

        doc = decl.annotations.get('doc')
        doc = doc and doc.text or ''
        if not doc:
            return Struct('', '')
        description, blocks = self.split(doc)

        details = self.format_description(description, decl)
        summary = self.extract_summary(details)
        details += self.format_params(blocks, decl)
        details += self.format_tag('return', blocks, decl)
        details += self.format_throws(blocks, decl)
        details += self.format_tag('precondition', blocks, decl)
        details += self.format_tag('postcondition', blocks, decl)
        details += self.format_tag('invariant', blocks, decl)
        details += self.format_tag('author', blocks, decl)
        details += self.format_tag('date', blocks, decl)
        details += self.format_tag('version', blocks, decl)
        details += self.format_tag('see', blocks, decl)

        return Struct(summary, details)


    def format_description(self, text, decl):

        return self.format_link(decl, text)


    def format_link(self, decl, text):
        """Formats inline @see tags in the text"""

        chunks = self.link_split.split(text)
        text = ''
        for i in range(len(chunks)):
            if i % 2 == 0:
                text += chunks[i]
            else:
                m = self.link.match(chunks[i])
                if m is None:
                    continue
                target, name = m.groups()
                if target[0] == '#':
                    target = target[1:]
                target = target.replace('#', '.')
                target = re.sub(r'\(.*\)', '', target)

                if name is None:
                    name = target
                else:
                    name = name.strip()
                target = self.lookup_symbol(name, decl.name)
                if target:
                    text += href(target, name)
                else:
                    text += name
        return text


    def format_params(self, blocks, decl):
        """Formats a list of (param, description) tags"""

        content = ''
        params = [b for b in blocks if b.tag == 'param']
        def row(dt, dd):
            return entity('tr',
                          entity('th', dt, Class='dt') +
                          entity('td', dd, Class='dd'))
        if params:
            content += div('tag-heading',"Parameters:")
            dl = entity('table', ''.join([row(p.arg, p.body) for p in params]),
                        Class='dl')
            content += div('tag-section', dl)
        kwds = [b for b in blocks if b.tag == 'keyword']
        if kwds:
            content += div('tag-heading',"Keywords:")
            dl = entity('dl', ''.join([row( k.arg, k.body) for k in kwds]),
                        Class='dl')
            content += div('tag-section', dl)
        return content


    def format_throws(self, blocks, decl):

        content = ''
        throws = [b for b in blocks if b.tag in ['throws', 'exception']]
        if throws:
            content += div('tag-heading',"Throws:")
            dl = entity('dl', ''.join([entity('dt', t.arg) + entity('dd', t.body)
                                       for t in throws]))
            content += div('tag-section', dl)
        return content


    def format_tag(self, tag, blocks, decl):

        content = ''
        items = [b for b in blocks if b.tag == tag]
        if items:
            content += div('tag-heading', self.tag_name[tag][1])
            content += div('tag-section',
                           '<br/>'.join([i.body for i in items]))
        return content


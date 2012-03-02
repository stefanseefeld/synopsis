#
# Copyright (C) 2011 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.Formatters.HTML.Tags import *
from Synopsis.Formatters.HTML.Markup import *
import re

_inline_tags = {}
_inline_tags['c'] = 'code'
_inline_tags['e'] = 'em'
_inline_tags['em'] = 'em'
_inline_tags['a'] = 'em'

def _replace_inline_tag(match):

    word = match.group('word')
    tag = match.group('tag')
    if tag == 'c':
        return '<code>%s</code>'%escape(word)
    elif tag in _inline_tags:
        tag = _inline_tags[tag]
        return '<%s>%s</%s>'%(tag, word, tag)
    else:
        return word

class Doxygen(Formatter):
    """A formatter that formats comments similar to doxygen. (http://www.doxygen.org)"""

    class Block:

        def __init__(self, tag, arg, body):
            self.tag, self.arg, self.body = tag, arg, body

        def astext(self, f):
            text = f.inline_tag.sub(_replace_inline_tag, self.body)
            if not self.tag:
                return para(text)
            elif self.tag in ('result', 'return', 'returns'):
                return (div('Returns', class_='tag-heading') +
                        div(text, class_='tag-section'))
            elif self.tag in ('param'):
                return (div('Parameters', class_='tag-heading') +
                        div(text, class_='tag-section'))
            elif self.tag in ('code'):
                return element('pre', escape(text))
            else: # TODO: raise error on unknown tag ?
                return para(text)
            
    brief = r'^\s*(\\|@)brief\s+([\w\W]*?\.)(\s|$)'
    # Simple tags covering a single word.
    inline_tag = r'(\\|@)(?P<tag>\w+)\s+(?P<word>\S+)'
    # blocks are either enclosed by ('\tag', '\endtag') pairs, or
    # a '\tag' and the paragraph (denoted by an empty line).
    #
    # Note: For block tags we require the opening marker to appear
    #       near the start of a line. We therefor start with a check
    #       for '(^|\n)'. Since we also detect paragraph boundaries
    #       by an empty line '(\n\s*\n)', we can't consume the last
    #       newline there (as we consume it in the next block !).
    #       Thus, as end of paragraph we use a look-ahead pattern.
    block_tag = (r'(^|\n)\s*(\\|@)(?P<tag>\w+)\s+'
                 r'((?P<block>[\s\S]*)(\2end\3)|'
                 r'(?P<paragraph>[\s\S]*?)((\n\s*(?=\n))|(\s*$)))')

    arg_tags = ['param', 'keyword', 'exception']

    def __init__(self):
        """Create regex objects for regexps"""

        self.brief = re.compile(Doxygen.brief)
        self.inline_tag = re.compile(Doxygen.inline_tag)
        self.block_tag = re.compile(Doxygen.block_tag)

    def split(self, description):
        """Split summary and details from description."""

        m = self.brief.match(description)
        if m:
            return m.group(2), description[m.end():]
        else:
            return description.split('\n', 1)[0]+'...', description


    def format(self, decl, view):
        """Format using doxygen markup."""

        doc = decl.annotations.get('doc')
        text = doc and doc.text.strip() or ''
        if not text:
            return Struct('', '')
        summary, details = self.split(text)
        if not details:
            details = summary
        return Struct(self.format_summary(summary, view, decl),
                      self.format_details(details, view, decl))


    def format_summary(self, text, view, decl):

        text = self.inline_tag.sub(_replace_inline_tag, text)
        return text

    def format_details(self, text, view, decl):

        blocks = []
        pos = 0
        for m in self.block_tag.finditer(text):

            if pos < m.start():
                blocks.append(Doxygen.Block('', None, text[pos:m.start()]))
            arg, content = None, m.group('paragraph') or m.group('block')
            if m.group('tag') in self.arg_tags:
                arg, content = content.strip().split(None, 1)
            blocks.append(Doxygen.Block(m.group('tag'), arg, content))
            pos = m.end()
        if pos < len(text):
            blocks.append(Doxygen.Block('', None, text[pos:]))

        return self.format_blocks(blocks, view, decl)

    def format_blocks(self, blocks, view, decl):

        text = self.format_params([b for b in blocks if b.tag in ('param', 'keyword')],
                                  view, decl)
        text += self.format_throws([b for b in blocks if b.tag in ('throws')],
                                   view, decl)
        for b in blocks:
            if b.tag in ('param', 'keyword', 'throws'):
                continue
            text += b.astext(self)
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



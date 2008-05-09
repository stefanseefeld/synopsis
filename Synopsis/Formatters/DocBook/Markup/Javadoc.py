#
# Copyright (C) 2006 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.Formatters.DocBook.Markup import *
import re

def attributes(keys):
    "Convert a name/value dict to a string of attributes"

    return ''.join(['%s="%s"'%(k,v) for k,v in keys.items()])

def element(_type, body, **keys):
    "Wrap the body in a tag of given type and attributes"

    return '<%s %s>%s</%s>'%(_type,attributes(keys),body,_type)

def title(name) : return element('title', name)
def para(body) : return element('para', body)
def listitem(body) : return element('listitem', body)
def term(body) : return element('term', body)
def link(linkend, label) : return element('link', label, linkend=linkend)
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
            return '<para>%s</para>\n'%m.group(1)
        else:
            return '<para>%s</para>\n'%(description.split('\n', 1)[0]+'...')


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
        details += self.format_variablelist('return', blocks, decl)
        details += self.format_throws(blocks, decl)
        vl = self.format_varlistentry('precondition', blocks, decl)
        vl += self.format_varlistentry('postcondition', blocks, decl)
        vl += self.format_varlistentry('invariant', blocks, decl)
        vl += self.format_varlistentry('author', blocks, decl)
        vl += self.format_varlistentry('date', blocks, decl)
        vl += self.format_varlistentry('version', blocks, decl)
        vl += self.format_varlistentry('deprecated', blocks, decl)
        vl += self.format_varlistentry('see', blocks, decl)
        if vl:
            details += '<variablelist>\n%s\n</variablelist>\n'%vl
            
        return Struct(summary, details)


    def format_description(self, text, decl):

        return '<para>%s</para>\n'%self.format_inlines(decl, text)


    def format_inlines(self, decl, text):
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
                                                   decl)
        return text


    def format_params(self, blocks, decl):
        """Formats a list of (param, description) tags"""

        content = ''
        params = [b for b in blocks if b.tag == 'param']
        if params:
            params = [element('varlistentry', term(p.arg) + listitem(para(p.body)))
                      for p in params]
            content += element('variablelist',
                               title('Parameters') + '\n' + '\n'.join(params))
        kwds = [b for b in blocks if b.tag == 'keyword']
        if kwds:
            kwds = [element('varlistentry', term(k.arg) + listitem(para(k.body)))
                    for k in kwds]
            content += element('variablelist',
                               title('Keywords') + '\n' + '\n'.join(kwds))
        return content


    def format_throws(self, blocks, decl):

        content = ''
        throws = [b for b in blocks if b.tag in ['throws', 'exception']]
        if throws:
            throws = [element('varlistentry', term(t.arg) + listitem(para(t.body)))
                      for t in throws]
            content += element('variablelist',
                               title('Throws') + '\n' + '\n'.join(throws))
        return content


    def format_variablelist(self, tag, blocks, decl):
        """
        Generate a variablelist for the given tag.
        Each matching block is formatted to a varlistentry, with the value
        of its 'arg' member becoming the term."""

        content = ''
        items = [b for b in blocks if b.tag == tag]
        if items:
            items = [element('varlistentry', term(i.arg) + listitem(para(self.format_inlines(decl, i.body))))
                     for i in items]
            content += element('variablelist',
                               title(self.tag_name[tag][1]) + '\n' + '\n'.join(items))
        return content


    def format_varlistentry(self, tag, blocks, decl):
        """
        Generate a varlistentry for the given tag.
        The tag value itself becomes the term. If multiple blocks match,
        format them as an (inlined) simplelist, otherwise as a para."""

        items = [b for b in blocks if b.tag == tag]
        if not items:
            return ''
        if len(items) > 1:
            items = [element('member', self.format_inlines(decl, i.body)) for i in items]
            content = element('simplelist', '\n'.join(items), type='inline') + '\n'
        else:
            content = element('para', self.format_inlines(decl, items[0].body))

        return element('varlistentry', term(self.tag_name[tag][1]) + '\n' + listitem(content))


    def format_inline_tag(self, tag, content, decl):

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
                text += link(target, label)
            else:
                text += label
        elif tag == 'code':
            text += '<code>%s</code>'%escape(content)
        elif tag == 'literal':
            text += '<literal>%s</literal>'%escape(content)

        return text

#
# Copyright (C) 2008 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

import parser
import token
import tokenize
import symbol
import keyword

HAVE_ENCODING_DECL = hasattr(symbol, "encoding_decl") # python 2.3
HAVE_IMPORT_NAME = hasattr(symbol, "import_name") # python 2.4
HAVE_DECORATOR = hasattr(symbol,"decorator") # python 2.4

def num_tokens(ptree):
    """Count the number of leaf tokens in the given ptree."""

    if type(ptree) == str: return 1
    else: return sum([num_tokens(n) for n in ptree[1:]])


class LexerDebugger:

    def __init__(self, lexer):

        self.lexer = lexer

    def next(self):

        n = self.lexer.next()
        print 'next is "%s" (%s)'%(n[1], n[0])
        return n

header="""<sxr filename="%(filename)s">
<line>"""

trailer="""</line>
</sxr>
"""

def escape(text):

    for p in [('&', '&amp;'), ('"', '&quot;'), ('<', '&lt;'), ('>', '&gt;'),]:
        text = text.replace(*p)
    return text


class SXRGenerator:
    """"""

    def __init__(self):
        """"""

        self.handlers = {}
        self.handlers[token.ENDMARKER] = self.handle_end_marker
        self.handlers[token.NEWLINE] = self.handle_newline
        self.handlers[token.INDENT] = self.handle_indent
        self.handlers[token.DEDENT] = self.handle_dedent
        self.handlers[token.STRING] = self.handle_string
        self.handlers[symbol.funcdef]= self.handle_function
        self.handlers[symbol.parameters] = self.handle_parameters
        self.handlers[symbol.classdef] = self.handle_class
        self.handlers[token.NAME] = self.handle_name
        self.handlers[symbol.expr_stmt] = self.handle_expr_stmt
        #self.handlers[token.OP] = self.handle_op
        self.handlers[symbol.power] = self.handle_power
        if HAVE_ENCODING_DECL:
            self.handlers[symbol.encoding_decl] = self.handle_encoding_decl
        if HAVE_IMPORT_NAME:
            self.handlers[symbol.import_as_names] = self.handle_import_as_names
            self.handlers[symbol.dotted_as_names] = self.handle_dotted_as_names
            self.handlers[symbol.import_from] = self.handle_import_from
            self.handlers[symbol.import_name] = self.handle_import_name
        else:
            self.handlers[symbol.import_stmt] = self.handle_import
        if HAVE_DECORATOR:
            self.handlers[symbol.decorator] = self.handle_decorator

        self.col = 0
        self.lineno = 1
        self.parameters = []
        self.scopes = []

    def process_file(self, scope, sourcefile, sxr):

        self.scopes = list(scope)
        input = open(sourcefile.abs_name, 'r+')
        src = input.readlines()
        self.lines = len(`len(src) + 1`)
        ptree = parser.ast2tuple(parser.suite(''.join(src)))
        input.seek(0)
        self.lexer = tokenize.generate_tokens(input.readline)
        #self.lexer = LexerDebugger(tokenize.generate_tokens(input.readline))
        self.sxr = open(sxr, 'w+')
        lineno_template = '%%%ds' % self.lines
        lineno = lineno_template % self.lineno
        self.sxr.write(header % {'filename': sourcefile.name})
        try:
            self.handle(ptree)
        except StopIteration:
            raise
        self.sxr.write(trailer)
        self.sxr.close()
        self.scopes.pop()

    def handle(self, ptree):

        if type(ptree) == tuple:
            kind = ptree[0]
            value = ptree[1:]
            handler = self.handlers.get(kind, self.default_handler)
            handler(value)
        else:
            raise Exception("Process error: Type is not a tuple %s" % str(ptree))


    def default_handler(self, ptree):

        for node in ptree:
            if type(node) == tuple: self.handle(node)
            elif type(node) == str: self.handle_token(node)
            else: raise Exception("Invalid ptree node")


    def next_token(self):
        """Return the next visible token.
        Process tokens that are not part of the parse tree silently."""

        t = self.lexer.next()
        while t[0] in [tokenize.NL, tokenize.COMMENT]:
            if t[0] is tokenize.NL:
                self.print_newline()
            elif t[0] is tokenize.COMMENT:
                self.print_token(t)
                if t[1][-1] == '\n': self.print_newline()
            t = self.lexer.next()
        return t
    

    def handle_token(self, item = None):

        t = self.next_token()
        if item is not None and t[1] != item:
            raise 'Internal error in line %d: expected "%s", got "%s" (%d)'%(self.lineno, item, t[1], t[0])
        else:
            self.print_token(t)
  

    def handle_name_as_xref(self, xref, name, from_ = None, type = None):

        kind, value, (srow, scol), (erow, ecol), line = self.next_token()
        if (kind, value) != (token.NAME, name):
            raise 'Internal error in line %d: expected name "%s", got "%s" (%d)'%(name, self.lineno, item, t[1], t[0])

        if self.col != scol:
            self.sxr.write(' ' * (scol - self.col))
        attrs = []
        if from_: attrs.append('from="%s"'%from_)
        if type: attrs.append('type="%s"'%type)
        a = '<a href="%s" %s>%s</a>'%('.'.join(xref), ' '.join(attrs), value)
        self.sxr.write(a)
        self.col = ecol
  

    def handle_tokens(self, ptree):

        tokens = num_tokens(ptree)
        for i in xrange(tokens):
            self.handle_token()
  

    def handle_end_marker(self, nodes): pass
    def handle_newline(self, nodes):

        self.handle_token()


    def handle_indent(self, indent):

        self.handle_token()
        
        
    def handle_dedent(self, dedent):

        self.handle_token()
        
        
    def handle_string(self, content):

        self.handle_token()

        
    def handle_function(self, nodes):

        if HAVE_DECORATOR:
            if nodes[0][0] == symbol.decorators:
                offset = 1
                # FIXME
                self.handle(nodes[0])
            else:
                offset = 0
        else:
            offset = 0
            
        def_token = nodes[0 + offset]
        self.handle_token(def_token[1])
        name = nodes[1 + offset][1]
        qname = tuple(self.scopes + [name])
        self.handle_name_as_xref(qname, name, from_='.'.join(self.scopes), type='definition')
        # Handle the parameters.
        self.handle(nodes[2 + offset])

        colon_token = nodes[3 + offset]
        self.handle_token(colon_token[1])
        body = nodes[4 + offset]
        # Theoretically, we'd have to push the function scope here.
        # Practically, however, we don't inject xrefs (yet) into function bodies.
        self.handle_tokens(body)

        # Don't traverse the function body, since the ASG doesn't handle
        # local declarations anyways.


    def handle_parameters(self, nodes):

        self.handle_token(nodes[0][1])
        if nodes[1][0] == symbol.varargslist:
            args = list(nodes[1][1:])
            while args:
                if args[0][0] == token.COMMA:
                    self.handle_token(args[0][1])
                    pass
                elif args[0][0] == symbol.fpdef:
                    self.handle_tokens(args[0])
                elif args[0][0] == token.EQUAL:
                    self.handle_token(args[0][1])
                    del args[0]
                    self.handle_tokens(args[0])
                elif args[0][0] == token.DOUBLESTAR:
                    self.handle_token(args[0][1])
                    del args[0]
                    self.handle_token(args[0][1])
                elif args[0][0] == token.STAR:
                    self.handle_token(args[0][1])
                    del args[0]
                    self.handle_token(args[0][1])
                else:
                    print "Unknown symbol:",args[0]
                del args[0]
        self.handle_token(nodes[-1][1])


    def handle_class(self, nodes):

        class_token = nodes[0]
        self.handle_token(class_token[1])
        name = nodes[1][1]
        qname = tuple(self.scopes + [name])
        self.handle_name_as_xref(qname, name, from_='.'.join(self.scopes), type='definition')
        base_clause = nodes[2][0] == token.LPAR and nodes[3] or None
        self.handle_tokens(nodes[2])
        bases = []
        if base_clause:
            self.handle_tokens(base_clause)
            self.handle_token(')')
            self.handle_token(':')

            body = nodes[6]
        else:
            body = nodes[3]
        self.scopes.append(name)
        self.handle(body)
        self.scopes.pop()
        

    def handle_name(self, content):

        self.handle_token(content[0])


    def handle_expr_stmt(self, nodes):

        for n in nodes: self.handle_tokens(n)


    def handle_dotted_name(self, dname, rest):

        self.handle_token(dname[0])    
        for name in dname[1:]:
            self.handle_token('.')
            self.handle_token(name)
        map(self.handle, rest)
        

    def handle_op(self, nodes): pass


    def handle_power(self, content):

        def get_dotted_name(content):
            if content[0][0] != symbol.atom or content[0][1][0] != token.NAME:
                return None
            dotted_name = [content[0][1][1]]
            i = 1
            for param in content[1:]:
                if param[0] != symbol.trailer: break
                if param[1][0] != token.DOT: break
                if param[2][0] != token.NAME: break
                dotted_name.append(param[2][1])
                i += 1
            if i < len(content): return dotted_name, content[i:]
            else: return dotted_name, []

        name = get_dotted_name(content)
        if name: self.handle_dotted_name(*name)
        else: map(self.handle, content)


    def handle_encoding_decl(self, nodes): pass
    def handle_import_as_names(self, nodes):

        for n in nodes: self.handle(n)

        
    def handle_dotted_as_names(self, nodes):

        for n in nodes: self.handle(n)


    def handle_import_from(self, nodes):

        self.handle_token('from')
        self.handle(nodes[1])
        self.handle_token('import')
        self.handle(nodes[3])


    def handle_import_name(self, nodes):

        self.handle_token('import')
        self.handle_dotted_as_names(nodes[1][1:])

        
    def handle_import(self, nodes):

        #self.handle_token('import')
        for n in nodes: self.handle(n)

        
    def handle_decorator(self, nodes): pass


    def print_token(self, t):

        kind, value, (srow, scol), (erow, ecol), line = t
        if kind == token.NEWLINE:
            self.print_newline()
        else:
            if self.col != scol:
                self.sxr.write(' ' * (scol - self.col))
            if keyword.iskeyword(value):
                format = '<span class="py-keyword">%s</span>'
            elif kind == token.STRING:
                format = '<span class="py-string">%s</span>'
                chunks = value.split('\n')
                for c in chunks[:-1]:
                    self.sxr.write(format % escape(c))
                    self.print_newline()
                value = chunks[-1]
                    
            elif kind == tokenize.COMMENT:
                format = '<span class="py-comment">%s</span>'
                if value[-1] == '\n': value = value[:-1]
            else:
                format = '%s'

            self.sxr.write(format % escape(value))
            self.col = ecol


    def print_newline(self):

        self.col = 0
        self.lineno += 1
        self.sxr.write('</line>\n')
        self.sxr.write('<line>')



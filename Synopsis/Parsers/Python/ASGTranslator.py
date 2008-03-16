#
# Copyright (C) 2008 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis import ASG
from Synopsis.QualifiedName import QualifiedPythonName as QName
from Synopsis.SourceFile import *
from Synopsis.DocString import DocString
import sys, os.path
import compiler, tokenize, token
from compiler.consts import OP_ASSIGN
from compiler.visitor import ASTVisitor

class TokenParser:

    def __init__(self, text):
        self.text = text + '\n\n'
        self.lines = self.text.splitlines(1)
        self.generator = tokenize.generate_tokens(iter(self.lines).next)
        self.next()

    def __iter__(self):
        return self

    def next(self):
        self.token = self.generator.next()
        self.type, self.string, self.start, self.end, self.line = self.token
        return self.token

    def goto_line(self, lineno):
        while self.start[0] < lineno:
            self.next()
        return token

    def rhs(self, lineno):
        """
        Return a whitespace-normalized expression string from the right-hand
        side of an assignment at line `lineno`.
        """
        self.goto_line(lineno)
        while self.string != '=':
            self.next()
        self.stack = None
        while self.type != token.NEWLINE and self.string != ';':
            if self.string == '=' and not self.stack:
                self.tokens = []
                self.stack = []
                self._type = None
                self._string = None
                self._backquote = 0
            else:
                self.note_token()
            self.next()
        self.next()
        text = ''.join(self.tokens)
        return text.strip()

    closers = {')': '(', ']': '[', '}': '{'}
    openers = {'(': 1, '[': 1, '{': 1}
    del_ws_prefix = {'.': 1, '=': 1, ')': 1, ']': 1, '}': 1, ':': 1, ',': 1}
    no_ws_suffix = {'.': 1, '=': 1, '(': 1, '[': 1, '{': 1}

    def note_token(self):
        if self.type == tokenize.NL:
            return
        del_ws = self.del_ws_prefix.has_key(self.string)
        append_ws = not self.no_ws_suffix.has_key(self.string)
        if self.openers.has_key(self.string):
            self.stack.append(self.string)
            if (self._type == token.NAME
                or self.closers.has_key(self._string)):
                del_ws = 1
        elif self.closers.has_key(self.string):
            assert self.stack[-1] == self.closers[self.string]
            self.stack.pop()
        elif self.string == '`':
            if self._backquote:
                del_ws = 1
                assert self.stack[-1] == '`'
                self.stack.pop()
            else:
                append_ws = 0
                self.stack.append('`')
            self._backquote = not self._backquote
        if del_ws and self.tokens and self.tokens[-1] == ' ':
            del self.tokens[-1]
        self.tokens.append(self.string)
        self._type = self.type
        self._string = self.string
        if append_ws:
            self.tokens.append(' ')

    def function_parameters(self, lineno):
        """
        Return a dictionary mapping parameters to defaults
        (whitespace-normalized strings).
        """
        self.goto_line(lineno)
        while self.string != 'def':
            self.next()
        while self.string != '(':
            self.next()
        name = None
        default = False
        parameter_tuple = False
        self.tokens = []
        parameters = {}
        self.stack = [self.string]
        self.next()
        while 1:
            if len(self.stack) == 1:
                if parameter_tuple:
                    name = ''.join(self.tokens).strip()
                    self.tokens = []
                    parameter_tuple = False
                if self.string in (')', ','):
                    if name:
                        if self.tokens:
                            default_text = ''.join(self.tokens).strip()
                        else:
                            default_text = ''
                        parameters[name] = default_text
                        self.tokens = []
                        name = None
                        default = False
                    if self.string == ')':
                        break
                elif self.type == token.NAME:
                    if name and default:
                        self.note_token()
                    else:
                        assert name is None, (
                            'token=%r name=%r parameters=%r stack=%r'
                            % (self.token, name, parameters, self.stack))
                        name = self.string
                elif self.string == '=':
                    assert name is not None, 'token=%r' % (self.token,)
                    assert default is False, 'token=%r' % (self.token,)
                    assert self.tokens == [], 'token=%r' % (self.token,)
                    default = True
                    self._type = None
                    self._string = None
                    self._backquote = 0
                elif name:
                    self.note_token()
                elif self.string == '(':
                    parameter_tuple = True
                    self._type = None
                    self._string = None
                    self._backquote = 0
                    self.note_token()
                else:                   # ignore these tokens:
                    assert (self.string in ('*', '**', '\n') 
                            or self.type == tokenize.COMMENT), (
                        'token=%r' % (self.token,))
            else:
                self.note_token()
            self.next()
        return parameters

class ASGTranslator(ASTVisitor):

    def __init__(self, package, types):
        """Create an ASGTranslator.

        package: enclosing package the generated modules are to be part of."""

        ASTVisitor.__init__(self)
        self.scope = package and [package] or []
        self.file = None
        self.types = types
        self.attributes = []
        self.any_type = ASG.BuiltinTypeId('Python',QName('',))
        self.docformat = ''
        self.documentable = None
        self.name = QName()
        self.imports = []


    def process_file(self, file):

        self.file = file
        source = open(self.file.abs_name).read()
        self.token_parser = TokenParser(source)
        ast = compiler.parse(source)
        compiler.walk(ast, self, walker=self)

    def scope_name(self):
        return len(self.scope) and self.scope[-1].name or ()

    def default(self, node, *args):
        self.documentable = None

    def default_visit(self, node, *args):
        ASTVisitor.default(self, node, *args)

    def visitDiscard(self, node):
        if self.documentable:
            self.visit(node.expr)

    def visitConst(self, node):
        if self.documentable:
            if type(node.value) in (str, unicode):
                self.documentable.annotations['doc'] = DocString(node.value, self.docformat)
            else:
                self.documentable = None

    def visitStmt(self, node):
        self.default_visit(node)

    def visitAssign(self, node):

        save_attributes = self.attributes
        self.attributes = []
        self.in_ass_tuple = False
        for child in node.nodes:
            self.dispatch(child)
        if self.attributes:
            if type(self.scope[-1]) == ASG.Operation:
                # Inject the attributes into the class.
                self.scope[-2].declarations.extend(self.attributes)
            else:
                self.scope[-1].declarations.extend(self.attributes)
        if len(self.attributes) == 1:
            self.documentable = self.attributes[0]
        else:
            self.documentable = None
        self.attributes = save_attributes

    def visitModule(self, node):

        name = os.path.basename(os.path.splitext(self.file.name)[0])
        if name == '__init__':
            name = os.path.basename(os.path.dirname(self.file.name))
            qname = QName(self.scope_name() + (name,))
            module = ASG.Module(self.file, node.lineno, 'package', qname)
        else:
            qname = QName(self.scope_name() + (name,))
            module = ASG.Module(self.file, node.lineno, 'module', qname)            
        self.types[qname] = ASG.DeclaredTypeId('Python', qname, module)

        self.scope.append(module)
        self.documentable = module
        self.visit(node.node)
        self.scope.pop()
        self.file.declarations.append(module)

    def visitImport(self, node):

        self.imports.extend([(n[0], None) for n in node.names])
        self.documentable = None

    def visitFrom(self, node):

        self.imports.extend([(node.modname, n[0]) for n in node.names])
        self.documentable = None

    def visitAssName(self, node):

        if not self.in_ass_tuple:
            meta_tags = ['__docformat__']
            if len(self.scope) == 1 and node.name in meta_tags:
                expression_text = eval(self.token_parser.rhs(node.lineno))
                self.file.annotations[node.name] = expression_text
                self.docformat = expression_text

        qname = QName(self.scope_name() + (node.name,))
        if type(self.scope[-1]) in (ASG.Function, ASG.Operation):
            return
        elif type(self.scope[-1]) == ASG.Class:
            attribute = ASG.Variable(self.file, node.lineno, 'class attribute',
                                     qname, self.any_type, False)
        else:
            attribute = ASG.Variable(self.file, node.lineno, 'attribute',
                                     qname, self.any_type, False)
        if node.name.startswith('__'):
            attribute.accessibility = ASG.PRIVATE
        elif node.name.startswith('_'):
            attribute.accessibility = ASG.PROTECTED
        self.attributes.append(attribute)
        self.types[qname] = ASG.DeclaredTypeId('Python', attribute.name, attribute)

    def visitAssTuple(self, node):

        self.in_ass_tuple = True
        for a in node.nodes:
            self.visit(a)
        self.in_ass_tuple = False

    def visitAssAttr(self, node):
        self.default_visit(node, node.attrname)
        if type(self.scope[-1]) == ASG.Operation:
            # We only parse constructors, so look out for
            # self attributes defined here.
            # FIXME: There is no reason the 'self' argument actually has to be spelled 'self'.
            if self.name[0] == 'self':
                # FIXME: qualifying variables is ambiguous, since we don't distinguish
                #        class attributes and object attributes.
                qname = self.scope[-2].name + self.name[1:]
                self.attributes.append(ASG.Variable(self.file, node.lineno,
                                                    'attribute', qname, self.any_type, False))

    def visitGetattr(self, node, suffix):
        self.default_visit(node, node.attrname + '.' + suffix)

    def visitName(self, node, suffix=None):

        if suffix:
            self.name = QName((node.name,) + (suffix,))
        else:
            self.name = QName((node.name,))

    def visitFunction(self, node):

        if isinstance(self.scope[-1], ASG.Function):
            # Skip local functions.
            return
        qname = QName(self.scope_name() + (node.name,))
        if type(self.scope[-1]) == ASG.Class:
            function = ASG.Operation(self.file, node.lineno, 'method',
                                     [], self.any_type, [], qname, node.name)
        else:
            function = ASG.Function(self.file, node.lineno, 'function',
                                    [], self.any_type, [], qname, node.name)

        # The following attributes are special in that even though they are private they
        # match publicly accessible operations, so we exclude them from being
        # marked as private.
        special_attributes = ('__init__', '__str__', '__repr__', '__iter__', '__getitem__')

        if node.name.startswith('__'):
            if node.name not in special_attributes:
                function.accessibility = ASG.PRIVATE
        elif node.name.startswith('_'):
            function.accessibility = ASG.PROTECTED

        function.annotations['doc'] = DocString(node.doc or '', self.docformat)
        # Given that functions in Python are first-class citizens, should they be
        # treated like (named) types ?
        self.types[qname] = ASG.DeclaredTypeId('Python', function.name, function)

        self.scope.append(function)
        self.documentable = function
        function.parameters = self.parse_parameter_list(node)
        if node.name == '__init__':
            # Only parse constructors, to find member variables
            self.visit(node.code)
        self.scope.pop()
        self.scope[-1].declarations.append(function)

    def parse_parameter_list(self, node):
        parameters = []
        special = []
        argnames = list(node.argnames)
        if node.kwargs:
            special.append(ASG.Parameter('**', self.any_type, '', argnames[-1]))
            argnames.pop()
        if node.varargs:
            special.append(ASG.Parameter('*', self.any_type, '', argnames[-1]))
            argnames.pop()
        defaults = list(node.defaults)
        defaults = [None] * (len(argnames) - len(defaults)) + defaults
        values = self.token_parser.function_parameters(node.lineno)
        for argname, default in zip(node.argnames, defaults):
            if type(argname) is tuple:
                for a in argname:
                    # FIXME: It is generally impossible to match tuple parameters
                    # to defaults individually, we ignore default values for now.
                    # (We may try to match them, and only leave out those resulting
                    # from tuple-returning call expressions. But that's for another day.)
                    parameters.append(ASG.Parameter('', self.any_type, '', a))
            else:
                parameters.append(ASG.Parameter('', self.any_type, '', argname,
                                                values[argname]))
        if parameters or special:
            special.reverse()
            parameters.extend(special)
        return parameters

    def visitClass(self, node):

        if isinstance(self.scope[-1], ASG.Function):
            # Skip local classes.
            return
        bases = []
        for base in node.bases:
            self.visit(base)
            # FIXME: This logic is broken !
            #        It assumes that names are either local or fully qualified.
            if len(self.name) == 1 and self.scope:
                # Name is unqualified. Qualify it.
                base = QName(list(self.scope[-1].name) + list(self.name))
            else:
                base = self.name
            if self.types.has_key(base):
                base = self.types[base]
            else:
                base = ASG.UnknownTypeId('Python', base)
            bases.append(base)
        qname = QName(self.scope_name() + (node.name,))
        class_ = ASG.Class(self.file, node.lineno, 'class', qname)
        class_.parents = [ASG.Inheritance('', b, '') for b in bases]
        class_.annotations['doc'] = DocString(node.doc or '', self.docformat)
        self.types[qname] = ASG.DeclaredTypeId('Python', class_.name, class_)
        self.scope.append(class_)
        self.documentable = class_
        self.visit(node.code)
        self.scope.pop()
        self.scope[-1].declarations.append(class_)

    def visitGetattr(self, node, suffix=None):
        if suffix:
            name = node.attrname + '.' + suffix
        else:
            name = node.attrname
        self.default_visit(node, name)



#
# Copyright (C) 2005 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis import AST
from Transformer import Transformer
import re

class Grouper(Transformer):
    """A class that detects grouping tags and moves the enclosed nodes
    into a subnode (a 'Group')"""

    def __init__(self, **kwds):

        Transformer.__init__(self, **kwds)
        self.__group_stack = [[]]

    def strip_dangling_groups(self):
        """As groups must not overlap with 'real' scopes,
        make sure all groups created in the current scope are closed
        when leaving the scope."""

        if self.__group_stack[-1]:
            print 'Warning: group stack is non-empty !'
            while (self.__group_stack[-1]):
                group = self.__group_stack[-1][-1]
                print 'forcing closing of group %s (opened near %s:%d)'%(group.name(), group.file().filename(), group.line())
                self.pop_group()

    def finalize(self):
        """replace the AST with the newly created one"""

        self.strip_dangling_groups()
        super(Grouper, self).finalize()

    def push(self):
        """starts a new group stack to be able to validate group scopes"""

        Transformer.push(self)
        self.__group_stack.append([])

    def pop(self, decl):
        """Make sure the current group stack is empty."""

        self.strip_dangling_groups()
        self.__group_stack.pop()
        Transformer.pop(self, decl)

    def push_group(self, group):
        """Push new group scope to the stack."""

        self.__group_stack[-1].append(group)
        Transformer.push(self)

    def pop_group(self, decl=None):
        """Pop a group scope from the stack.

        decl -- an optional declaration from which to extract the context,
        used for the error message if needed.
        """

        if self.__group_stack[-1]:
            group = self.__group_stack[-1].pop()
            group.declarations()[:] = self.currscope()
            Transformer.pop(self, group)
        else:
            if decl:
                print "Warning: no group open in current scope (near %s:%d), ignoring."%(decl.file().filename(), decl.line())
            else:
                print "Warning: no group open in current scope, ignoring."

    def process_comments(self, decl):
        """Checks for grouping tags.
        If an opening tag is found in the middle of a comment, a new Group is generated, the preceeding
        comments are associated with it, and is pushed onto the scope stack as well as the groups stack.
        """
        pass

    def visitDeclaration(self, decl):

        self.process_comments(decl)
        self.add(decl)
        
    def visitScope(self, scope):
        """Visits all children of the scope in a new scope. The value of
        currscope() at the end of the list is used to replace scope's list of
        declarations - hence you can remove (or insert) declarations from the
        list."""

        self.process_comments(scope)
        self.push()
        for decl in scope.declarations(): decl.accept(self)
        scope.declarations()[:] = self.currscope()
        self.pop(scope)

    def visitEnum(self, enum):
        """Does the same as visitScope, but for the enum's list of
        enumerators"""

        self.process_comments(enum)
        self.push()
        for enumor in enum.enumerators(): enumor.accept(self)
        enum.enumerators()[:] = self.currscope()
        self.pop(enum)

    def visitEnumerator(self, enumor):
        """Removes dummy enumerators"""

        if enumor.type() == "dummy": return #This wont work since Core.AST.Enumerator forces type to "enumerator"
        if not len(enumor.name()): return # workaround.
        self.add(enumor)

class Grouper1(Grouper):
    """Grouper that detects ' @group {' and '}' group markup"""

    __re_group = r'^[ \t]*((?P<open>@group[ \t]*(?P<name>.*){)|(?P<close>[ \t]*}))[ \t]*\Z'

    def __init__(self, **kwds):

        Grouper.__init__(self, **kwds)
        self.re_group = re.compile(Grouper1.__re_group, re.M)

    def process_comments(self, decl):
        """Checks for grouping tags.
        If an opening tag is found in the middle of a comment, a new Group is generated, the preceeding
        comments are associated with it, and is pushed onto the scope stack as well as the groups stack.
        """

        comments = []
        while len(decl.comments()):
            c = decl.comments().pop(0)
            tag = self.re_group.search(c.text)
            if not tag:
                comments.append(c)
                continue
            elif tag.group('open'):

                if self.debug:
                    print 'found group open tag in', decl.name()

                # Open group. Name is remainder of line
                label = tag.group('name') or 'unnamed'
                # The comment before the open marker becomes the group comment
                if tag.start('open') > 0:
                    text = c.text[:tag.start('open')]
                    comments.append(AST.Comment(text, c.file(), c.line()))
                group = AST.Group(decl.file(), decl.line(), decl.language(), "group", [label])
                group.comments()[:] = comments
                comments = []
                self.push_group(group)

            elif tag.group('close'):

                if self.debug:
                    print 'found group close tag in', decl.name()

                self.pop_group(decl)

        decl.comments()[:] = comments

class Grouper2(Grouper):
    """Grouper that detects ' @group {' and ' @group }' group markup"""
    
    __re_group = r'^[ \t]*((?P<open>@group[ \t]*(?P<name>.*){)|(?P<close>@group[ \t]*}))(?P<remainder>.*)$'

    def __init__(self, **kwds):

        Grouper.__init__(self, **kwds)
        self.re_group = re.compile(Grouper2.__re_group, re.M)

    def process_comments(self, decl):
        """Checks for grouping tags.
        If an opening tag is found in the middle of a comment, a new Group is generated, the preceeding
        comments are associated with it, and is pushed onto the scope stack as well as the groups stack.
        """

        comments = []
        process_comments = decl.comments()
        while len(decl.comments()):
            c = decl.comments().pop(0)
            tag = self.re_group.search(c.text)
            if not tag:
                comments.append(c)
                continue
            elif tag.group('open'):

                if self.debug:
                    print 'found group open tag in', decl.name()

                # Open group. Name is remainder of line
                label = tag.group('name') or 'unnamed'
                # The comment before the open marker becomes the group comment
                if tag.start('open') > 0:
                    text = c.text[:tag.start('open')]
                    comments.append(AST.Comment(text, c.file(), c.line()))
                group = AST.Group(decl.file(), decl.line(), decl.language(), "group", [label])
                group.comments()[:] = comments
                comments = []
                # The comment after the open marker becomes the next comment to process
                if tag.group('remainder'):
                    decl.comments().insert(0, AST.Comment(tag.group('remainder'), c.file(), c.line()))
                self.push_group(group)
            elif tag.group('close'):

                if self.debug:
                    print 'found group close tag in', decl.name()

                self.pop_group(decl)

            # The comment before the close marker is ignored...? maybe post-comment?
            # The comment after the close marker becomes the next comment to process
            remainder = ''.join([tag.group('remainder'), c.text[tag.end():]])
            if remainder:
                decl.comments().insert(0, AST.Comment(remainder, c.file(), c.line()))
        decl.comments()[:] = comments


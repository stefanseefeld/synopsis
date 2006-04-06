#
# Copyright (C) 2005 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis import AST
from Synopsis.Processors.Transformer import Transformer
import re

class Grouper(Transformer):
    """A class that detects grouping tags and moves the enclosed nodes
    into a subnode (a 'Group')"""

    tags = r'^\s*((?P<open>@group\s*(?P<name>.*){)|(?P<close>\s*}))\s*\Z'

    def __init__(self, **kwds):

        Transformer.__init__(self, **kwds)
        self.__group_stack = [[]]
        self.tags = re.compile(Grouper.tags, re.M)

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
            group.declarations()[:] = self.current_scope()
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

        comments = []
        for c in decl.annotations.get('comments', []):
            if c is None:
                comments.append(None)
                continue
            tag = self.tags.search(c)
            if not tag:
                comments.append(c)
                continue
            elif tag.group('open'):

                if self.debug:
                    print 'found group open tag in', decl.name()

                # Open the group. <name> is remainder of line.
                label = tag.group('name') or 'unnamed'
                # The comment before the open marker becomes the group comment.
                if tag.start('open') > 0:
                    c = c[:tag.start('open')]
                    comments.append(c)
                group = AST.Group(decl.file(), decl.line(), decl.language(), "group", [label])
                group.annotations['comments'] = comments
                comments = []
                self.push_group(group)

            elif tag.group('close'):

                if self.debug:
                    print 'found group close tag in', decl.name()

                self.pop_group(decl)

        decl.annotations['comments'] = comments


    def visitDeclaration(self, decl):

        self.process_comments(decl)
        self.add(decl)
        
    def visitScope(self, scope):
        """Visits all children of the scope in a new scope. The value of
        current_scope() at the end of the list is used to replace scope's list of
        declarations - hence you can remove (or insert) declarations from the
        list."""

        self.process_comments(scope)
        self.push()
        for decl in scope.declarations(): decl.accept(self)
        scope.declarations()[:] = self.current_scope()
        self.pop(scope)

    def visitEnum(self, enum):
        """Does the same as visitScope, but for the enum's list of
        enumerators"""

        self.process_comments(enum)
        self.push()
        for enumor in enum.enumerators(): enumor.accept(self)
        enum.enumerators()[:] = self.current_scope()
        self.pop(enum)

    def visitEnumerator(self, enumor):
        """Removes dummy enumerators"""

        if enumor.type() == "dummy": return #This wont work since Core.AST.Enumerator forces type to "enumerator"
        if not len(enumor.name()): return # workaround.
        self.add(enumor)


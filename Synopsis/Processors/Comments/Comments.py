#
# Copyright (C) 2000 Stefan Seefeld
# Copyright (C) 2000 Stephen Davies
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.Processor import Processor, Parameter
from Synopsis import AST

import string, re

class CommentProcessor(Processor, AST.Visitor):
   """Base class for comment processors.

   This is an AST visitor, and by default all declarations call process()
   with the current declaration. Subclasses may override just the process
   method.
   """
   def process(self, ast, **kwds):
      
      self.set_parameters(kwds)
      self.ast = self.merge_input(ast)

      for decl in ast.declarations():
         decl.accept(self)

      self.finalize()
      return self.output_and_return_ast()

   def visit_comments(self, decl):
      """Process comments for the given declaration"""
      pass

   def finalize(self):
      """Do any finalization steps that may be necessary."""
      pass

   def visitDeclaration(self, decl):
      self.visit_comments(decl)

   def visitBuiltin(self, decl):
      if decl.type() == 'EOS': # treat it if it is an 'end of scope' marker
         self.visit_comments(decl)
        
class SSDComments(CommentProcessor):
   """A class that selects only //. comments."""

   __re_star = r'/\*(.*?)\*/'
   __re_ssd = r'^[ \t]*//\. ?(.*)$'

   def __init__(self):
      "Compiles the regular expressions"

      self.re_star = re.compile(SSDComments.__re_star, re.S)
      self.re_ssd = re.compile(SSDComments.__re_ssd, re.M)

   def visit_comments(self, decl):
      "Calls process_comment on all comments"

      map(self.process_comment, decl.comments())

   def process_comment(self, comment):
      """Replaces the text in the comment. It calls strip_star() first to
      remove all multi-line star comments, then follows with parse_ssd().
      """

      text = comment.text()
      text = self.parse_ssd(self.strip_star(text))
      comment.set_text(text)

   def strip_star(self, str):
      """Strips all star-format comments from the string"""

      mo = self.re_star.search(str)
      while mo:
         str = str[:mo.start()] + str[mo.end():]
         mo = self.re_star.search(str)
      return str

   def parse_ssd(self, str):
      """Filters str and returns just the lines that start with //."""

      return string.join(self.re_ssd.findall(str),'\n')

class JavaComments (CommentProcessor):
   """A class that formats java /** style comments"""

   __re_java = r"/\*\*[ \t]*(?P<text>.*)(?P<lines>(\n[ \t]*\*.*)*?)(\n[ \t]*)?\*/"
   __re_line = r"\n[ \t]*\*[ \t]*(?P<text>.*)"

   def __init__(self):
      "Compiles the regular expressions"

      self.re_java = re.compile(JavaComments.__re_java)
      self.re_line = re.compile(JavaComments.__re_line)

   def visit_comments(self, decl):
      "Calls process_comment on all comments"

      map(self.process_comment, decl.comments())

   def process_comment(self, comment):
      """Finds comments in the java format. The format is  /** ... */, and
      it has to cater for all four line forms: "/** ...", " * ...", " */" and
      the one-line "/** ... */".
      """

      text = comment.text()
      text_list = []
      mo = self.re_java.search(text)
      while mo:
         text_list.append(mo.group('text'))
         lines = mo.group('lines')
         if lines:
            mol = self.re_line.search(lines)
            while mol:
               text_list.append(mol.group('text'))
               mol = self.re_line.search(lines, mol.end())
         mo = self.re_java.search(text, mo.end())
      text = string.join(text_list,'\n')
      comment.set_text(text)

class SSComments (CommentProcessor):
   """A class that selects only // comments."""

   __re_star = r'/\*(.*?)\*/'
   __re_ss = r'^[ \t]*// ?(.*)$'

   def __init__(self):
      "Compiles the regular expressions"

      self.re_star = re.compile(SSComments.__re_star, re.S)
      self.re_ss = re.compile(SSComments.__re_ss, re.M)

   def visit_comments(self, decl):
      "Calls process_comment on all comments"

      map(self.process_comment, decl.comments())

   def process_comment(self, comment):
      """Replaces the text in the comment. It calls strip_star() first to
      remove all multi-line star comments, then follows with parse_ss().
      """

      text = comment.text()
      text = self.parse_ss(self.strip_star(text))
      comment.set_text(text)

   def strip_star(self, str):
      """Strips all star-format comments from the string"""

      mo = self.re_star.search(str)
      while mo:
         str = str[:mo.start()] + str[mo.end():]
         mo = self.re_star.search(str)
      return str

   def parse_ss(self, str):
      """Filters str and returns just the lines that start with //"""

      return string.join(self.re_ss.findall(str),'\n')

class QtComments (CommentProcessor):
   """A class that finds Qt style comments. These have two styles: //! ...
   and /*! ... */. The first means "brief comment" and there must only be
   one. The second type is the detailed comment."""

   __re_brief = r"[ \t]*//!(.*)"
   __re_detail = r"[ \t]*/\*!(.*)\*/[ \t\n]*"

   def __init__(self):
      "Compiles the regular expressions"

      self.re_brief = re.compile(self.__re_brief)
      self.re_detail = re.compile(self.__re_detail, re.S)

   def visit_comments(self, decl):
      "Calls process_comment on all comments"

      map(self.process_comment, decl.comments())

   def process_comment(self, comment):
      "Matches either brief or detailed comments"

      text = comment.text()
      mo = self.re_brief.match(text)
      if mo:
         comment.set_text(mo.group(1))
         return
      mo = self.re_detail.match(text)
      if mo:
         comment.set_text(mo.group(1))
         return
      comment.set_text('')

class Transformer (CommentProcessor):
   """A class that creates a new AST from an old one. This is a helper base for
   more specialized classes that manipulate the AST based on
   the comments in the nodes"""

   def __init__(self):
      """Constructor"""

      self.__scopestack = []
      self.__currscope = []

   def finalize(self):
      """replace the AST with the newly created one"""

      self.ast.declarations()[:] = self.__currscope

   def push(self):
      """Pushes the current scope onto the stack and starts a new one"""

      self.__scopestack.append(self.__currscope)
      self.__currscope = []

   def pop(self, decl):
      """Pops the current scope from the stack, and appends the given
      declaration to it"""

      self.__currscope = self.__scopestack.pop()
      self.__currscope.append(decl)

   def add(self, decl):
      """Adds the given decl to the current scope"""

      self.__currscope.append(decl)

   def currscope(self):
      """Returns the current scope: a list of declarations"""

      return self.__currscope
        
class Previous(CommentProcessor):
   """A class that maps comments that begin with '<' to the previous
   declaration"""

   def process(self, ast, **kwds):
      """decorates process() to initialise last and laststack"""

      self.set_parameters(kwds)
      self.ast = self.merge_input(ast)

      self.last = None
      self.__laststack = []
      for decl in ast.declarations():
         decl.accept(self)
         self.last = decl

      return self.output_and_return_ast()

   def push(self):
      """decorates push() to also push 'last' onto 'laststack'"""

      self.__laststack.append(self.last)
      self.last = None

   def pop(self):
      """decorates pop() to also pop 'last' from 'laststack'"""

      self.last = self.__laststack.pop()

   def visitScope(self, scope):
      """overrides visitScope() to set 'last' after each declaration"""

      self.push()
      for decl in scope.declarations():
         decl.accept(self)
         self.last = decl
      self.pop()

   def visit_comments(self, decl):
      """Checks a decl to see if the comment should be moved. If the comment
      begins with a less-than sign, then it is moved to the 'last'
      declaration"""

      if len(decl.comments()) and self.last:
         first = decl.comments()[0]
         if len(first.text()) and first.text()[0] == "<" and self.last:
            if self.debug:
               print 'found comment for previous in', decl.name()
            first.set_suspect(0) # Remove suspect flag
            first.set_text(first.text()[1:]) # Remove '<'
            self.last.comments().append(first)
            del decl.comments()[0]

   def visitEnum(self, enum):
      """Does the same as visitScope but for enum and enumerators"""

      self.push()
      for enumor in enum.enumerators():
         enumor.accept(self)
         self.last = enumor
      if enum.eos: enum.eos.accept(self)
      self.pop()

   def visitEnumerator(self, enumor):
      """Checks previous comment and removes dummies"""

      self.visit_comments(enumor)

class Grouper (Transformer):
   """A class that detects grouping tags and moves the enclosed nodes
   into a subnode (a 'Group')"""

   def __init__(self):

      Transformer.__init__(self)
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

   def visit_comments(self, decl):
      """Checks for grouping tags.
      If an opening tag is found in the middle of a comment, a new Group is generated, the preceeding
      comments are associated with it, and is pushed onto the scope stack as well as the groups stack.
      """
      pass

   def visitDeclaration(self, decl):
      self.visit_comments(decl)
      self.add(decl)
        
   def visitScope(self, scope):
      """Visits all children of the scope in a new scope. The value of
      currscope() at the end of the list is used to replace scope's list of
      declarations - hence you can remove (or insert) declarations from the
      list."""

      self.visit_comments(scope)
      self.push()
      for decl in scope.declarations(): decl.accept(self)
      scope.declarations()[:] = self.currscope()
      self.pop(scope)

   def visitEnum(self, enum):
      """Does the same as visitScope, but for the enum's list of
      enumerators"""

      self.visit_comments(enum)
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

   def __init__(self):

      Grouper.__init__(self)
      self.re_group = re.compile(Grouper1.__re_group, re.M)

   def visit_comments(self, decl):
      """Checks for grouping tags.
      If an opening tag is found in the middle of a comment, a new Group is generated, the preceeding
      comments are associated with it, and is pushed onto the scope stack as well as the groups stack.
      """

      comments = []
      while len(decl.comments()):
         c = decl.comments().pop(0)
         tag = self.re_group.search(c.text())
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
               text = c.text()[:tag.start('open')]
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

   def __init__(self):

      Grouper.__init__(self)
      self.re_group = re.compile(Grouper2.__re_group, re.M)

   def visit_comments(self, decl):
      """Checks for grouping tags.
      If an opening tag is found in the middle of a comment, a new Group is generated, the preceeding
      comments are associated with it, and is pushed onto the scope stack as well as the groups stack.
      """

      comments = []
      process_comments = decl.comments()
      while len(decl.comments()):
         c = decl.comments().pop(0)
         tag = self.re_group.search(c.text())
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
               text = c.text()[:tag.start('open')]
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
         remainder = string.join([tag.group('remainder'), c.text()[tag.end():]], '')
         if remainder:
            decl.comments().insert(0, AST.Comment(remainder, c.file(), c.line()))
      decl.comments()[:] = comments

class CommentStripper(CommentProcessor):
   """Strip off all but the last comment."""

   def visit_comments(self, decl):
      """Summarize the comment of this declaration."""

      if not len(decl.comments()):
         return
      if decl.comments()[-1].is_suspect():
         decl.comments()[:] = []
      else:
         decl.comments()[:] = [decl.comments()[-1]]

class Summarizer(CommentProcessor):
   """Splits comments into summary/detail parts."""

   re_summary = r"[ \t\n]*(.*?\.)([ \t\n]|$)"

   def __init__(self):

      self.re_summary = re.compile(Summarizer.re_summary, re.S)

   def visit_comments(self, decl):
      """Summarize the comment of this declaration."""

      comments = decl.comments()
      if not len(comments):
         return
      # Only use last comment
      comment = comments[-1]
      tags = comment.tags()
      # Decide how much of the comment is the summary
      text = comment.text()
      mo = self.re_summary.match(text)
      if mo:
         # Set summary to the sentence
         comment.set_summary(mo.group(1))
      else:
         # Set summary to whole text
         comment.set_summary(text)

class JavaTags(CommentProcessor):
   """Extracts javadoc-style @tags from the end of comments."""

   _re_tags = '\n[ \t]*(?P<tags>@[a-zA-Z]+[ \t]+.*)'

   def __init__(self):

      self.re_tags = re.compile(self._re_tags,re.M|re.S)

   def visit_comments(self, decl):
      """Extract tags from each comment of the given decl"""

      for comment in decl.comments():
         # Find tags
         text = comment.text()
         mo = self.re_tags.search(text)
         if not mo:
            continue
         # A lambda to use in the reduce expression
         joiner = lambda x,y: len(y) and y[0]=='@' and x+[y] or x[:-1]+[x[-1]+' '+y]

         tags = mo.group('tags')
         text = text[:mo.start('tags')]
         # Split the tag section into lines
         tags = map(string.strip, string.split(tags,'\n'))
         # Join non-tag lines to the previous tag
         tags = reduce(joiner, tags, [])
         # Split the tag lines into @name, rest-of-line pairs
         tags = map(lambda line: string.split(line,' ',1), tags)
         # Convert the pairs into CommentTag objects
         tags = map(lambda pair: AST.CommentTag(pair[0], pair[1]), tags)
         # Store back in comment
         comment.set_text(text)
         comment.tags().extend(tags)


# $Id: Javadoc.py,v 1.2 2003/12/05 22:40:44 stefan Exp $
#
# Copyright (C) 2000 Stephen Davies
# Copyright (C) 2000 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis import AST, Type
from Synopsis.Formatters.HTML.Tags import *
from Formatter import Formatter
import string, re

class Javadoc(Formatter):
   """A formatter that formats comments similar to Javadoc @tags"""

   # @see IDL/Foo.Bar
   _re_see = '@see (([A-Za-z+]+)/)?(([A-Za-z_]+\.?)+)'
   _re_see_line = '^[ \t]*@see[ \t]+(([A-Za-z+]+)/)?(([A-Za-z_]+\.?)+)(\([^)]*\))?([ \t]+(.*))?$'
   _re_param = '^[ \t]*@param[ \t]+(?P<name>(A-Za-z+]+)([ \t]+(?P<desc>.*))?$'

   def __init__(self):
      """Create regex objects for regexps"""

      self.re_see = re.compile(self._re_see)
      self.re_see_line = re.compile(self._re_see_line,re.M)

   def extract(self, regexp, str):
      """Extracts all matches of the regexp from the text. The MatchObjects
      are returned in a list"""

      mo = regexp.search(str)
      ret = []
      while mo:
         ret.append(mo)
         start, end = mo.start(), mo.end()
         str = str[:start] + str[end:]
         mo = regexp.search(str, start)
      return str, ret

   def format(self, page, decl, text):
      """Format any @tags in the text, and any @tags stored by the JavaTags
      CommentProcessor in the Linker stage."""

      if text is None: return text
      see_tags, attr_tags, param_tags, return_tag = [], [], [], None
      tags = decl.comments()[0].tags()
      # Parse each of the tags
      for tag in tags:
         name, rest = tag.name(), tag.text()
         if name == '@see':
            see_tags.append(string.split(rest,' ',1))
         elif name == '@param':
            param_tags.append(string.split(rest,' ',1))
         elif name == '@return':
            return_tag = rest
         elif name == '@attr':
            attr_tags.append(string.split(rest,' ',1))
         else:
            # unknown tag
            pass
      return "%s%s%s%s%s"%(self.format_inline_see(page, decl, text),
                           self.format_params(param_tags),
                           self.format_attrs(attr_tags),
                           self.format_return(return_tag),
                           self.format_see(page, see_tags, decl))

   def format_inline_see(self, page, decl, text):
      """Formats inline @see tags in the text"""

      #TODO change to link or whatever javadoc uses
      mo = self.re_see.search(text)
      while mo:
         groups, start, end = mo.groups(), mo.start(), mo.end()
         lang = groups[1] or ''
         link = self.find_link(page, groups[2], decl)
         text = text[:start] + link + text[end:]
         end = start + len(link)
         mo = self.re_see.search(text, end)
      return text

   def format_params(self, param_tags):
      """Formats a list of (param, description) tags"""

      if not len(param_tags): return ''
      return div('tag-heading',"Parameters:") + \
             div('tag-section', string.join(
         map(lambda p:"<b>%s</b> - %s"%(p[0],p[1]), param_tags),
         '<br>'))

   def format_attrs(self, attr_tags):
      """Formats a list of (attr, description) tags"""

      if not len(attr_tags): return ''
      table = '<table border=1 class="attr-table">%s</table>'
      row = '<tr><td valign="top" class="attr-table-name">%s</td><td class="attr-table-desc">%s</td></tr>'
      return div('tag-heading',"Attributes:") + \
             table%string.join(map(lambda p,row=row:row%(p[0],p[1]), attr_tags))
   
   def format_return(self, return_tag):
      """Formats a since description string"""

      if not return_tag: return ''
      return div('tag-heading',"Return:")+div('tag-section',return_tag)

   def format_see(self, page, see_tags, decl):
      """Formats a list of (ref,description) tags"""

      if not len(see_tags): return ''
      seestr = div('tag-heading', "See Also:")
      seelist = []
      for see in see_tags:
         ref,desc = see[0], len(see)>1 and see[1] or ''
         link = self.find_link(page, ref, decl)
         seelist.append(link + desc)
      return seestr + div('tag-section', string.join(seelist,'\n<br>\n'))

   def find_link(self, page, ref, decl):
      """Given a "reference" and a declaration, returns a HTML link.
      Various methods are tried to resolve the reference. First the
      parameters are taken off, then we try to split the ref using '.' or
      '::'. The params are added back, and then we try to match this scoped
      name against the current scope. If that fails, then we recursively try
      enclosing scopes.
      """

      # Remove params
      index, label = string.find(ref,'('), ref
      if index >= 0:
         params = ref[index:]
         ref = ref[:index]
      else:
         params = ''
      # Split ref
      ref = string.split(ref, '.')
      if len(ref) == 1:
         ref = string.split(ref[0], '::')
      # Add params back
      ref = ref[:-1] + [ref[-1]+params]
      # Find in all scopes
      scope = list(decl.name())
      while 1:
         entry = self._find_link_at(ref, scope)
         if entry:
            url = rel(page.filename(), entry.link)
            return href(url, label)
         if len(scope) == 0: break
         del scope[-1]
      # Not found
      return label+" "

   def _find_link_at(self, ref, scope):
      # Try scope + ref[0]
      entry = self.processor.toc.lookup(scope+ref[:1])
      if entry:
         # Found.
         if len(ref) > 1:
            # Find sub-refs
            entry = self._find_link_at(ref[1:], scope+ref[:1])
            if entry:
               # Recursive sub-ref was okay!
               return entry 
         else:
            # This was the last scope in ref. Done!
            return entry
      # Try a method name match:
      if len(ref) == 1:
         entry = self._find_method_entry(ref[0], scope)
         if entry: return entry
      # Not found at this scope
      return None

   def _find_method_entry(self, name, scope):
      """Tries to find a TOC entry for a method adjacent to decl. The
      enclosing scope is found using the types dictionary, and the
      realname()'s of all the functions compared to ref."""

      try:
         scope = self.processor.ast.types()[scope]
      except KeyError:
         #print "No parent scope:",decl.name()[:-1]
         return None
      if not scope: return None
      if not isinstance(scope, Type.Declared): return None
      scope = scope.declaration()
      if not isinstance(scope, AST.Scope): return None
      for decl in scope.declarations():
         if isinstance(decl, AST.Function):
            if decl.realname()[-1] == name:
               return self.processor.toc.lookup(decl.name())
      # Failed
      return None

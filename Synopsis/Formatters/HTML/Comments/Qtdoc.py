#
# Copyright (C) 2000 Stephen Davies
# Copyright (C) 2000 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Javadoc import Javadoc

class Qtdoc(Javadoc):
   """A formatter that uses Qt-style doc tags."""

   _re_see = '@see (([A-Za-z+]+)/)?(([A-Za-z_]+\.?)+)'
   _re_tags = r'((?P<text>.*?)\n)?[ \t]*(?P<tags>\\[a-zA-Z]+[ \t]+.*)'
   _re_seealso = '[ \t]*(,|and|,[ \t]*and)[ \t]*'

   def __init__(self):
      Javadoc.__init__(self)
      self.re_seealso = re.compile(self._re_seealso)

   def parse_text(self, str, decl):

      if str is None: return str
      #str, see = self.extract(self.re_see_line, str)
      see_tags, param_tags, return_tag = [], [], None
      joiner = lambda x,y: len(y) and y[0]=='\\' and x+[y] or x[:-1]+[x[-1]+y]
      str, tags = self.parse_tags(str, joiner)
      # Parse each of the tags
      for line in tags:
         tag, rest = string.split(line,' ',1)
         if tag == '\\sa':
            see_tags.extend(map(lambda x: [x,''], self.re_seealso.split(rest)))
         elif tag == '\\param':
            param_tags.append(string.split(rest,' ',1))
         elif tag == '\\return':
            return_tag = rest
         else:
            # Warning: unknown tag
            pass
      return "%s%s%s%s"%(self.parse_see(str, decl),
                         self.format_params(param_tags),
                         self.format_return(return_tag),
                         self.format_see(see_tags, decl))

   def format_see(self, see_tags, decl):
      """Formats a list of (ref,description) tags"""

      if not len(see_tags): return ''
      seestr = div('tag-see-header', "See Also:")
      seelist = []
      for see in see_tags:
         ref,desc = see[0], len(see)>1 and see[1] or ''
         tag = self.re_seealso.match(ref) and ' %s '%ref or self.find_link(ref, decl)
         seelist.append(span('tag-see', tag+desc))
      return seestr + string.join(seelist,'')

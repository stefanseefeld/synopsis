# $Id: Formatter.py,v 1.21 2003/11/11 06:01:13 stefan Exp $
#
# This file is a part of Synopsis.
# Copyright (C) 2000, 2001 Stephen Davies
# Copyright (C) 2000, 2001 Stefan Seefeld
#
# Synopsis is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA.
#
# $Log: Formatter.py,v $
# Revision 1.21  2003/11/11 06:01:13  stefan
# adjust to directory/package layout changes
#
# Revision 1.20  2003/01/20 06:43:02  chalky
# Refactored comment processing. Added AST.CommentTag. Linker now determines
# comment summary and extracts tags. Increased AST version number.
#
# Revision 1.19  2003/01/16 18:54:03  chalky
# Simplified re_tags regexp
#
# Revision 1.18  2002/11/01 07:18:15  chalky
# Added the QuoteHTML formatter
#
# Revision 1.17  2002/04/26 01:21:14  chalky
# Bugs and cleanups
#
# Revision 1.16  2001/06/28 07:22:18  stefan
# more refactoring/cleanup in the HTML formatter
#
# Revision 1.15  2001/04/17 13:35:37  chalky
# Slightly more robust
#
# Revision 1.14  2001/04/05 14:03:21  chalky
# Very small change
#
# Revision 1.13  2001/04/03 23:18:47  chalky
# Fixed has_detail for decls with no comments
#
# Revision 1.12  2001/04/03 11:36:24  chalky
# Made comments show detail by default
#
# Revision 1.11  2001/03/29 14:09:55  chalky
# newlines were getting eaten in tags. made attr list a table
#
# Revision 1.10  2001/03/28 13:11:04  chalky
# Added @attr tag to Javadoc formatter - very similar to @param tags :)
#
# Revision 1.9  2001/03/28 12:55:19  chalky
# Sanity checks
#
# Revision 1.8  2001/02/13 05:19:31  chalky
# @see links are done a bit more methodically
#
# Revision 1.7  2001/02/12 04:08:09  chalky
# Added config options to HTML and Linker. Config demo has doxy and synopsis styles.
#
# Revision 1.6  2001/02/07 17:00:43  chalky
# Added Qt-style comments support
#
# Revision 1.5  2001/02/07 16:14:40  chalky
# Fixed @see linking to be more stubborn (still room for improv. tho)
#
# Revision 1.4  2001/02/07 15:31:06  chalky
# Rewrite javadoc formatter. Now all tags must be at the end of the comment,
# except for inline tags
#
# Revision 1.3  2001/02/01 16:47:48  chalky
# Added CommentFormatter as base of the Comment Formatters ...
#
# Revision 1.2  2001/02/01 15:23:24  chalky
# Copywritten brown paper bag edition.
#
#

"""CommentParser, CommentFormatter and derivatives."""

# System modules
import re, string

# Synopsis modules
from Synopsis import AST, Type, Util

# HTML modules
import core
from core import config
from Tags import *

class CommentFormatter:
    """A class that takes a Declaration and formats its comments into a string."""
    def __init__(self):
	self.__formatters = core.config.commentFormatterList
	# Cache the bound methods
	self.__format_methods = map(lambda f:f.format, self.__formatters)
	self.__format_summary_methods = map(lambda f:f.format_summary, self.__formatters)
	# Weed out the unneccessary calls to the empty base methods
	base = CommentFormatterStrategy.format.im_func
	self.__format_methods = filter(
	    lambda m, base=base: m.im_func is not base, self.__format_methods)
	base = CommentFormatterStrategy.format_summary.im_func
	self.__format_summary_methods = filter(
	    lambda m, base=base: m.im_func is not base, self.__format_summary_methods)

    def format(self, page, decl):
	"""Formats the first comment of the given AST.Declaration.
	Note that the Linker.Comments.Summarizer CommentProcessor is supposed
	to have combined all comments first in the Linker stage.
	@return the formatted text
	"""
	comments = decl.comments()
	if len(comments) == 0: return ''
	text = comments[0].text()
	if not text: return ''
	# Let each strategy format the text in turn
	for method in self.__format_methods:
	    text = method(page, decl, text)
	return text

    def format_summary(self, page, decl):
	"""Formats the summary of the first comment of the given
	AST.Declaration.
	Note that the Linker.Comments.Summarizer CommentProcessor is supposed
	to have combined all comments first in the Linker stage.
	@return the formatted summary text
	"""
	comments = decl.comments()
	if len(comments) == 0: return ''
	text = comments[0].summary()
	if not text: return ''
	# Let each strategy format the text in turn
	for method in self.__format_summary_methods:
	    text = method(page, decl, text)
	return text

class CommentFormatterStrategy:
    """Interface class that takes a comment and formats its summary and/or
    detail strings."""

    def format(self, page, decl, text):
	"""Format the given comment
	@param page the Page to use for references and determining the correct
	relative filename.
	@param decl the declaration
	@param text the comment text to format
	"""
	pass
    def format_summary(self, page, decl, summary):
	"""Format the given comment summary
	@param page the Page to use for references and determining the correct
	relative filename.
	@param decl the declaration
	@param summary the comment summary to format
	"""
	pass

class QuoteHTML (CommentFormatterStrategy):
    """A formatter that quotes HTML characters like the angle brackets and the
    ampersand. Formats both text and summary."""
    def format(self, page, decl, text):
	"""Replace angle brackets with HTML codes"""
	text = text.replace('&', '&amp;')
	text = text.replace('<', '&lt;')
	text = text.replace('>', '&gt;')
	return text
    def format_summary(self, page, decl, text):
	"""Replace angle brackets with HTML codes"""
	text = text.replace('&', '&amp;')
	text = text.replace('<', '&lt;')
	text = text.replace('>', '&gt;')
	return text

class JavadocFormatter (CommentFormatterStrategy):
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
	return "%s%s%s%s%s"%(
	    self.format_inline_see(page, decl, text),
	    self.format_params(param_tags),
	    self.format_attrs(attr_tags),
	    self.format_return(return_tag),
	    self.format_see(page, see_tags, decl)
	)
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
		    '<br>'
		)
	    )
    def format_attrs(self, attr_tags):
	"""Formats a list of (attr, description) tags"""
	if not len(attr_tags): return ''
	table = '<table border=1 class="attr-table">%s</table>'
	row = '<tr><td valign="top" class="attr-table-name">%s</td><td class="attr-table-desc">%s</td></tr>'
	return div('tag-heading',"Attributes:") + \
		table%string.join(
		    map(lambda p,row=row:row%(p[0],p[1]), attr_tags)
		)
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
	entry = config.toc.lookup(scope+ref[:1])
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
	    scope = config.types[scope]
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
		    return config.toc.lookup(decl.name())
	# Failed
	return None

class QtDocFormatter (JavadocFormatter):
    """A formatter that uses Qt-style doc tags."""
    _re_see = '@see (([A-Za-z+]+)/)?(([A-Za-z_]+\.?)+)'
    _re_tags = r'((?P<text>.*?)\n)?[ \t]*(?P<tags>\\[a-zA-Z]+[ \t]+.*)'
    _re_seealso = '[ \t]*(,|and|,[ \t]*and)[ \t]*'
    def __init__(self):
	JavadocFormatter.__init__(self)
	self.re_seealso = re.compile(self._re_seealso)

    def parseText(self, str, decl):
	if str is None: return str
	#str, see = self.extract(self.re_see_line, str)
	see_tags, param_tags, return_tag = [], [], None
	joiner = lambda x,y: len(y) and y[0]=='\\' and x+[y] or x[:-1]+[x[-1]+y]
	str, tags = self.parseTags(str, joiner)
	# Parse each of the tags
	for line in tags:
	    tag, rest = string.split(line,' ',1)
	    if tag == '\\sa':
		see_tags.extend(
		    map(lambda x: [x,''], self.re_seealso.split(rest))
		)
	    elif tag == '\\param':
		param_tags.append(string.split(rest,' ',1))
	    elif tag == '\\return':
		return_tag = rest
	    else:
		# Warning: unknown tag
		pass
	return "%s%s%s%s"%(
	    self.parse_see(str, decl),
	    self.format_params(param_tags),
	    self.format_return(return_tag),
	    self.format_see(see_tags, decl)
	)
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

class SectionFormatter (CommentFormatterStrategy):
    """A test formatter"""
    __re_break = '\n[ \t]*\n'

    def __init__(self):
	self.re_break = re.compile(SectionFormatter.__re_break)
    def format(self, page, decl, text):
	if text is None: return text
	para = '</p>\n<p>'
	mo = self.re_break.search(text)
	while mo:
	    start, end = mo.start(), mo.end()
	    text = text[:start] + para + text[end:]
	    end = start + len(para)
	    mo = self.re_break.search(text, end)
	return '<p>%s</p>'%text



commentFormatters = {
    'none' : CommentFormatterStrategy,
    'ssd' : CommentFormatterStrategy,
    'java' : CommentFormatterStrategy,
    'quotehtml' : QuoteHTML,
    'summary' : CommentFormatterStrategy,
    'javadoc' : JavadocFormatter,
    'qtdoc' : QtDocFormatter,
    'section' : SectionFormatter,
}



# $Id: doxygen.py,v 1.7 2003/12/08 00:39:23 stefan Exp $
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
# $Log: doxygen.py,v $
# Revision 1.7  2003/12/08 00:39:23  stefan
# s/page/view/g
#
# Revision 1.6  2003/11/11 06:01:13  stefan
# adjust to directory/package layout changes
#
# Revision 1.5  2003/01/20 06:43:02  chalky
# Refactored comment processing. Added AST.CommentTag. Linker now determines
# comment summary and extracts tags. Increased AST version number.
#
# Revision 1.4  2001/07/10 06:47:45  chalky
# Doxygen stuff works again
#
# Revision 1.3  2001/04/06 06:27:09  chalky
# Change from import * since our py parser cant handle that too well
#
# Revision 1.2  2001/04/05 09:57:49  chalky
# Add pre and post summary div, so the [Source] link can go inside it
#
# Revision 1.1  2001/02/12 04:55:45  chalky
# Initial commit
#
#

"""Doxygen emulation mode classes."""

import string

from Synopsis import AST
from Synopsis.Formatters.HTML import ScopeSorter, core
from Synopsis.Formatters.HTML.core import config
from Synopsis.Formatters.HTML.Tags import *
from Synopsis.Formatters.HTML import ASTFormatter, FormatStrategy
	    

class DOScopeSorter (ScopeSorter.ScopeSorter):
    def _section_of(self, decl):
	_axs_str = ('','Public ','Protected ','Private ')
	_types = {'enum':'Type', 'function':'Method', 'operation':'Method',
	    'variable':'Attribute', 'attribute':'Attribute'
	}
	if _types.has_key(decl.type()): section = _types[decl.type()]
	else: section = string.capitalize(decl.type())
	if decl.accessibility != AST.DEFAULT:
	    section = _axs_str[decl.accessibility()]+section
	return section

    def sort_section_names(self):
	"Sort by a known order.."
	_axs_str = ['Public ','Protected ','Private ']
	_axs_order = _axs_str
	_sec_order = ['Type', 'Method', 'Attribute']
	# Split into (Section, Name) tuples
	def totuple(name, axs_l=_axs_str):
	    for axs in axs_l:
		if name[:len(axs)] == axs:
		    return axs, name[len(axs):]
	    return '',name
	names = map(totuple, self.sections())
	def order(name, axs_o = _axs_order, sec_o = _sec_order):
	    "return a sort order for given name tuple"
	    o1 = (name[0] in axs_o) and axs_o.index(name[0])+1 or len(axs_o)
	    o2 = (name[1] in sec_o) and sec_o.index(name[1])+1 or len(sec_o)
	    return o1 * 1000 + o2
	names.sort(lambda a, b, order=order: cmp(order(a),order(b)))
	self._set_section_names(map(lambda x: string.join(x,''), names))
	
class DOSummaryAST (FormatStrategy.SummaryAST):
    def formatEnum(self, decl):
	"(enum, enum { list of enumerator names })"
	ename = self.label(decl.name())
	enumer = lambda enumor, ref=self.reference: ref(enumor.name())
	divver = lambda text: div("enumerator", text)
	commer = lambda text: text+","
	name = map(enumer, decl.enumerators())
	name = map(commer, name[:-1]) + name[-1:]
	name = "%s {%s}"%(ename, string.join(map(divver, name)))
	return "enum" + self.col_sep + name

class DOSummaryCommenter (FormatStrategy.SummaryCommenter):
    """Adds summary comments to all declarations"""
    def formatDeclaration(self, decl):
	style = config.decl_style[decl]
	more = (style != core.DeclStyle.SUMMARY) and ' '+self.reference(decl.name(), 'More...') or ''
	summary = config.comments.format_summary(self.view, decl)
	return span('summary', summary) + more

class DODetailAST (FormatStrategy.DetailAST):
    def formatFunction(self, decl):
	premod = self.formatModifiers(decl.premodifier())
	type = self.formatType(decl.returnType())
	name = self.label(decl.name(), decl.realname())
	# Special C++ functions  TODO: maybe move to a separate AST formatter...
	if decl.language() == 'C++' and len(decl.realname())>1:
	    if decl.realname()[-1] == decl.realname()[-2]: type = '<i>constructor</i>'
	    elif decl.realname()[-1] == "~"+decl.realname()[-2]: type = '<i>destructor</i>'
	    elif decl.realname()[-1] == "(conversion)":
		name = "(%s)"%type
	params = map(self.formatParameter, decl.parameters())
	postmod = self.formatModifiers(decl.postmodifier())
	raises = self.formatOperationExceptions(decl)
	type = '%s %s'%(premod,type)
        if decl.type() == "attribute": name = '%s %s %s %s'%(type, name, postmod, raises)
	else:
	    str = '<table border=0 cellspacing=0 cellpadding=0>'\
		    '<tr><td valign=top class="func">%s %s (</td>'\
		    '<td class="func">%s ) %s %s</td></tr></table>'
	    str2 = ',</td></tr><tr><td></td><td class="func">'
	    params = string.join(params, str2)
	    name = str%(type, name, params, postmod, raises)
	return self.col_sep + name

class PreDivFormatter (FormatStrategy.Default):
    def formatDeclaration(self, decl):
	return '<div class="preformat">'

class PostDivFormatter (FormatStrategy.Default):
    def formatDeclaration(self, decl):
	return '</div>'

class PreSummaryDiv (FormatStrategy.Default):
    def formatDeclaration(self, decl):
	return '<div class="summary">'

class PostSummaryDiv (FormatStrategy.Default):
    def formatDeclaration(self, decl):
	return '</div>'


class DOSummary (ASTFormatter.Summary):
    def old_init_formatters(self):
	self.addFormatter( DOSummaryAST)
	self.addFormatter( DOSummaryCommenter )

    def write_start(self):
	str = '<table width="100%%">'
	self.write(str)

    def writeSectionStart(self, heading):
	str = '<tr><td colspan="2" class="heading">%s</td></tr>'
	self.write(str%heading)

    def writeSectionEnd(self, heading):
	str = '<tr><td colspan="2" class="gap" height="20px">&#160;</td></tr>'
	self.write(str)

    def write_end(self):
	"""Closes the table entity and adds a break."""
	str = '</table><br/>'
	self.write(str)

    def writeSectionItem_Foo(self, type, name):
	"""Adds a table row with one or two data elements. If type is None
	then there is only one td with a colspan of 2."""
	if not len(type):
	    str = '<tr><td colspan="2">%s</td></tr>'
	    self.write(str%name)
	else:
	    str = '<tr><td valign="top" align="right">%s</td><td>%s</td></tr>'
	    self.write(str%(type,name))


class DODetail (ASTFormatter.Detail):
    def old_init_formatters(self):
	self.addFormatter( PreDivFormatter )
	self.addFormatter( DODetailAST )
	self.addFormatter( PostDivFormatter )
	self.addFormatter( FormatStrategy.ClassHierarchyGraph )
	self.addFormatter( FormatStrategy.DetailCommenter )

    def writeSectionEnd(self, heading):
	self.write('<hr/>')
    def writeSectionItem_Foo(self, text):
	"""Joins text1 and text2"""
	self.write(text)

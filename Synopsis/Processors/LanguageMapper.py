# $Id: LanguageMapper.py,v 1.1 2002/08/23 04:37:26 chalky Exp $
#
# This file is a part of Synopsis.
# Copyright (C) 2000, 2001 Stefan Seefeld
# Copyright (C) 2000, 2001 Stephen Davies
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
# $Log: LanguageMapper.py,v $
# Revision 1.1  2002/08/23 04:37:26  chalky
# Huge refactoring of Linker to make it modular, and use a config system similar
# to the HTML package
#

import string

from Synopsis.Core import AST, Type, Util

from Linker import config, Operation

class LanguageMapper(Operation):
    def execute(self, ast):
	declarations = ast.declarations()
	langs = {}
	for decl in declarations:
	    lang = decl.language()
	    if langs.has_key(lang):
		scope = langs[lang]
	    else:
		scope = AST.Module('',-1,lang,'Language',('%s Namespace'%lang,))
		langs[lang] = scope
	    scope.declarations().append(decl)
	keys = langs.keys()
	# TODO: allow user to specify sort order here
	keys.sort()
	declarations[:] = map(lambda key, dict=langs: dict[key], keys)

linkerOperation = LanguageMapper

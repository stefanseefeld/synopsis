#!/usr/bin/env python
#
# $Id: setup.py,v 1.1 2002/08/23 00:39:35 stefan Exp $
#
# Setup script for synopsis
#
# Usage: python setup.py install
#

from distutils.core import setup, Extension

import os, sys, re

def prefix(list, pref): return map(lambda x, p=pref: p + x, list)

ext_modules = []
py_packages = ["Synopsis.Core",
               "Synopsis.Parser.IDL", "Synopsis.Parser.Python",
               "Synopsis.Linker",
               "Synopsis.Formatter"] 

occ_src = ["buffer.cc", "hash.cc", "token.cc", "ptree.cc", "ptree-core.cc",
           "encoding.cc", "env.cc", "pattern.cc", "walker.cc", "typeinfo.cc",
           "parse.cc", "mop.cc", "classwalk.cc", "metaclass.cc", "quote-class.cc",
           "member.cc", "cbodywalk.cc"]

syn_src = ["synopsis.cc", "occ.cc", "swalker.cc", "ast.cc",
           "builder.cc", "type.cc", "dict.cc",
           "dumper.cc", "decoder.cc", "swalker-syntax.cc",
           "link_map.cc", "linkstore.cc", "lookup.cc"]

ucpp_src = ["mem.c", "hashtable.c", "cpp.c", "lexer.c", "assert.c",
            "macro.c", "eval.c"]

src = prefix(occ_src, "Synopsis/Parser/C++/occ/")
src.extend(prefix(syn_src, "Synopsis/Parser/C++/syn/"))
src.extend(prefix(ucpp_src, "Synopsis/Parser/C++/ucpp/"))

occ_macros = [("DONT_GC", 1)]
includes = ["Synopsis/Parser/C++"]
occ = Extension("occ", src, include_dirs=includes, define_macros=occ_macros)

ext_modules.append(occ)

setup(
    name="synopsis",
    version="0.5",
    author="Stefan Seefeld & Stephen Davies",
    author_email="synopsis-devel@lists.sf.net",
    description="source code inspection tool",
    url="http://synopsis.sf.net",
    packages=py_packages,
    ext_modules=ext_modules,
    )

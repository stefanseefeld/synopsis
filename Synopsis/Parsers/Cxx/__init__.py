# $Id: __init__.py,v 1.9 2003/11/05 17:36:55 stefan Exp $
#
# Copyright (C) 2003 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Parser import *

try:
    import occ
except:
    import sys
    print sys.exc_type, sys.exc_value
import emul

def usage():
    """Prints a usage message"""
    print """
  -I<path>                             Specify include path to be used by the preprocessor
  -D<macro>                            Specify macro to be used by the preprocessor
  -m                                   Only keep declarations from the main file
  -b basepath                          Strip basepath from start of filenames"""

def parse(file, extra_files, args, config):
    # ignore the config, as it is phased out and will be replaced
    # by the processor interface

    import getopt
    import occ
    from Synopsis.Core import AST

    cppflags = []
    main_file = False
    basepath = None
    verbose = False
    extract_tails = False
    xref_prefix = None
    syntax_prefix = None
    preprocessor = None
    opts,remainder = getopt.getopt(args, "I:D:mb:vtx:s:g")
    for o,a in opts:
        if o == '-I': cppflags.extend(['-I', a])
        elif o == '-D': cppflags.extend(['-D', a])
        elif o == '-m': main_file = True
        elif o == '-b': basepath = a
        elif o == '-v': verbose = True
        elif o == '-t': extract_tails = True
        elif o == '-x': xref_prefix = a
        elif o == '-s': syntax_prefix = a
        elif o == '-g': preprocessor = 'gcc'
        
    ast = AST.AST()
    
    ast = occ.parse(ast, file, [], verbose, main_file, basepath, preprocessor,
                    cppflags, extract_tails, syntax_prefix, xref_prefix)

    return ast

# THIS-IS-A-PARSER

# $Id: Parser.py,v 1.2 2003/11/05 17:21:50 stefan Exp $
#
# Copyright (C) 2003 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.Core.Processor import Processor
from Synopsis.Core import AST
from python import parse

class Parser(Processor):

    basename=''
    
    def process(self, ast, **kwds):
        for file in kwds['input']:
            ast.merge(parse(file, 0, {}, None))
        output = kwds.get('output')
        if output:
            AST.save(output, ast)
        return ast

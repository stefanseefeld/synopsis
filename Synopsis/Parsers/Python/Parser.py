# $Id: Parser.py,v 1.1 2003/10/30 05:54:36 stefan Exp $
#
# Copyright (C) 2003 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU GPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.Core.Processor import Processor
from python import parse

class Parser(Processor):

    basename=''
    
    def process(self, **kwds):
        for file in kwds['input']:
            print file
            parse(file, 0, {}, None)
#
# $Log: Parser.py,v $
# Revision 1.1  2003/10/30 05:54:36  stefan
# add Processor base class
#

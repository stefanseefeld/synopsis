#
# Copyright (C) 2008 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#
from distutils.command.build import build as base

class build(base):

    user_options = base.user_options + [
        ('with-boost', None, 'whether to use boost libraries in backends.')]
    boolean_options = base.boolean_options + ['with-boost']

    def initialize_options(self):
        base.initialize_options(self)
        self.with_boost = None

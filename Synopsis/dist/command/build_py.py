#
# Copyright (C) 2007 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#
from distutils.command.build_py import build_py as base
from Synopsis.dist.command import reset_config_variables
import os

class build_py(base):

    def run(self):

        # Do the default actions.
        base.run(self)

        config_file = os.path.join(self.build_lib, 'Synopsis', 'config.py')
        self.announce("adjusting config parameters")
        reset_config_variables(config_file, prefix=os.getcwd(),
                               revision=self.distribution.revision)


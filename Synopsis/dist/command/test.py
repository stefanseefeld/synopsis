#
# Copyright (C) 2004 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

import os, sys, string

from distutils.core import setup
from distutils.command.build_ext import build_ext
from distutils.util import get_platform
from distutils.dir_util import mkpath
from distutils.spawn import spawn, find_executable
from shutil import *

class test(build_ext):
    """derive from build since we use almost all the same options"""

    description = "run unit test"

    user_options = build_ext.user_options[:] + [
        ('suite=', 's',
         "the id of a test suite to run.")]

    def initialize_options (self):
        build_ext.initialize_options(self)
        self.suite = ''

    def finalize_options (self):
        build_ext.finalize_options(self)

    def run(self):

        cwd = os.getcwd()
        test_dir = os.path.join(self.build_temp, 'tests')
        os.chdir(test_dir)
        
        command = "qmtest run %s"%self.suite
        self.announce(command)
        spawn(['sh', '-c', command], self.verbose, self.dry_run)
        os.chdir(cwd)

#
# Copyright (C) 2004 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

import os, sys, string

from distutils.errors import *
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
        ('suite=', 's', "the id of a test suite to run."),
        ('expectation=', 'e', "an expectation file to compare the results against."),
        ('interactive', None, "run the web iterface to the testing engine.")]

    boolean_options = build_ext.boolean_options[:] + ['interactive']

    def initialize_options (self):
        build_ext.initialize_options(self)
        self.suite = ''
        self.expectation = ''
        self.interactive = False

    def finalize_options (self):
        build_ext.finalize_options(self)

    def run(self):

        cwd = os.getcwd()
        if self.expectation:
            self.expectation = os.path.join(cwd, self.expectation)
        os.environ['QMTEST_CLASS_PATH'] = os.path.join(cwd, 'tests', 'QMTest')
        test_dir = os.path.join(self.build_temp, 'tests')
        os.chdir(test_dir)

        command = 'qmtest '
        command += self.interactive and 'gui ' or 'run '
        if self.expectation:
            command += '-O %s '%self.expectation
        command += self.suite
                
        self.announce(command)
        try:
            spawn(['sh', '-c', command], self.verbose, self.dry_run)
        # As qmtest returns a non-zero exit code if some tests fail, we
        # simply ignore exec errors here.
        except DistutilsExecError, error:
            pass

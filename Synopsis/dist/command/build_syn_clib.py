#
# Copyright (C) 2004 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

import os, string
from distutils.core import Command
from distutils.dir_util import mkpath
from distutils.file_util import copy_file
from distutils.spawn import spawn, find_executable
from shutil import *

class build_syn_clib (Command):

    description = "build common C/C++ stuff used by all extensions"

    user_options = [('build-clib', 'b',
                     "directory to build C/C++ libraries to"),
                    ('build-temp', 't',
                     "directory to put temporary build by-products")]

    def initialize_options (self):

        self.build_clib = None
        self.build_temp = None

    def finalize_options (self):

        self.set_undefined_options('build_ext',
                                   ('build_temp', 'build_clib'),
                                   ('build_temp', 'build_temp'))

    def run (self):

        self.announce("building 'libSynopsis.a'")
        if os.name == 'nt': 
            # same as in config.py here: even on 'nt' we have to
            # use posix paths because we run in a cygwin shell at this point
            path = string.replace(self.build_temp, '\\', '/') + '/Cxx-API/src'
        else:
            path = os.path.join(self.build_temp, 'Cxx-API', 'src')
        
        make = os.environ.get('MAKE', 'make')

        command = "%s -C %s %s"%(make, path, 'all')
        spawn(['sh', '-c', command], self.verbose, self.dry_run)

    def get_source_files(self):

        def collect(arg, path, files):
            files.extend(os.listdir(path))
            print path, os.listdir(path)
        files = []
        os.path.walk('Cxx-API', collect, files)
        return files

    def get_outputs(self):

        return [] # no output, only temporaries

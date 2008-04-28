#
# Copyright (C) 2004 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

import os, dircache
from distutils.core import Command
from distutils import sysconfig
from distutils.dir_util import mkpath
from distutils.file_util import copy_file
from distutils.util import get_platform
from distutils.spawn import spawn
from shutil import *
from Synopsis.dist.command import copy_shared_library

def collect_headers(arg, path, files):
    
    # For now at least the following are not part of the public API
    if os.path.split(path)[1] in ['Support', 'Python', 'ASG']:
        return
    arg.extend([os.path.join(path, f) for f in files
                if f.endswith('.hh') or f.endswith('.h')])


class build_clib(Command):

    description = "build common C/C++ stuff used by all extensions"

    user_options = [('build-clib', 'b',
                     "directory to build C/C++ libraries to"),
                    ('build-temp', 't',
                     "directory to put temporary build by-products")]

    def initialize_options(self):

        self.build_base = 'build'
        self.build_clib = None
        self.build_ctemp = None


    def finalize_options(self):

        if self.build_ctemp is None:
            self.build_ctemp = os.path.join(self.build_base,
                                           'ctemp.' + get_platform())
        if self.build_clib is None:
            self.build_clib = os.path.join(self.build_base,
                                           'clib.' + get_platform())


    def run(self):

        version = self.distribution.get_version()
        self.announce("building 'Synopsis' shared library")
        if os.name == 'nt': 
            # same as in config.py here: even on 'nt' we have to
            # use posix paths because we run in a cygwin shell at this point
            path = self.build_ctemp.replace('\\', '/') + '/src'
        else:
            path = os.path.join(self.build_ctemp, 'src')
        
        make = os.environ.get('MAKE', 'make')

        command = '%s -C "%s" %s'%(make, path, 'all')
        spawn(['sh', '-c', command], self.verbose, self.dry_run)

        # copy library
        build_path = os.path.join(self.build_clib, 'lib')
        copy_shared_library('Synopsis', version, path, build_path,
                            self.verbose, self.dry_run)
            
        # copy headers
        headers = []
        os.path.walk(os.path.join(self.build_ctemp, 'src', 'Synopsis'),
                     collect_headers,
                     headers)
        incdir = os.path.join(self.build_clib, 'include')
        for header in headers:
            target = os.path.join(incdir, header[len(self.build_ctemp) + 5:])
            mkpath (os.path.dirname(target), 0777, self.verbose, self.dry_run)
            copy_file(header, target, 1, 1, 0, None, self.verbose, self.dry_run)

        # copy tools
        build_path = os.path.join(self.build_clib, 'bin')
        mkpath (build_path, 0777, self.verbose, self.dry_run)
        for tool in dircache.listdir(os.path.join(path, 'bin')):
            copy_file(os.path.join(path, 'bin', tool),
                      os.path.join(build_path, tool),
                      1, 1, 0, None, self.verbose, self.dry_run)
        # copy pkgconfig file
        copy_file(os.path.join(path, 'synopsis.pc'),
                  os.path.join(self.build_clib, 'synopsis.pc'),
                  1, 1, 0, None, self.verbose, self.dry_run)

    def get_source_files(self):

        def collect(arg, path, files):
            arg.extend([os.path.join(path, f)
                        for f in files if f[0] != '.']) # skip hidden files
        files = []
        os.path.walk('src', collect, files)
        return files

    def get_outputs(self):

        return [] # nothing to report here

#
# Copyright (C) 2003 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

import os, sys, string

from distutils.command.build_ext import build_ext
from distutils.dir_util import mkpath
from distutils.file_util import copy_file
from distutils.spawn import spawn, find_executable
from shutil import *

class build_syn_ext(build_ext):

    description = "build C++ extension packages"

    user_options = [('build-lib=', 'b', "directory for compiled extension modules"),
                    ('build-temp=', 't', "directory for temporary files (build by-products)"),
                    ('inplace', 'i', "ignore build-lib and put compiled extensions into the source " +
                     "directory alongside your pure Python modules")]

    boolean_options = ['inplace']

    def run(self):

        if not os.path.exists(self.build_temp):
            self.run_command('config')
        self.run_command('build_clib')
        for ext in self.extensions:
            self.build_extension(ext)

    def check_extensions_list (self, extensions):
        pass # do nothing, trust that the extensions are correct

    def get_source_files(self):

        def collect(arg, path, files):
            arg.extend([os.path.join(path, file) for file in files])
        sources = []
        for ext in self.extensions:
            os.path.walk(ext[0], collect, sources)
        return sources

    def get_outputs(self):

        output = []
        for ext in self.extensions:
            # FIXME: this ugly hack is needed since the ucpp module
            # should be installed in the Cpp package, not Cpp.ucpp
            if ext[1][:4] in ['ucpp', 'wave']:
                path = os.path.join(self.build_lib, os.path.dirname(ext[0]), ext[1])
            else:
                path = os.path.join(self.build_lib, ext[0], ext[1])
            #only append the files that actually could be built
            if os.path.isfile(path):
                output.append(path)
        return output

    def build_extension(self, ext, copy=True):

        self.announce("building '%s' in %s"%(ext[1], ext[0]))

        if os.name == 'nt': 
            # same as in config.py here: even on 'nt' we have to
            # use posix paths because we run in a cygwin shell at this point
            path = string.replace(self.build_temp, '\\', '/') + '/' + ext[0]
            temp_target = string.replace(self.build_temp, '\\', '/') + '/' + ext[0]
        else:
            path = os.path.join(self.build_temp, ext[0])
            temp_target = os.path.join(self.build_temp, ext[0])
        
        make = os.environ.get('MAKE', 'make')

        command = '%s -C "%s" %s'%(make, path, ext[1])
        spawn(['sh', '-c', command], self.verbose, self.dry_run)

        #The extension may not be compiled. For now just skip it.
        if copy and os.path.isfile(os.path.join(temp_target, ext[1])):
            
            if self.inplace: build_path = ext[0]
            else: build_path = os.path.join(self.build_lib, ext[0])            
            mkpath (build_path, 0777, self.verbose, self.dry_run)
            copy_file(os.path.join(path, ext[1]),
                      os.path.join(build_path, ext[1]),
                      1, 1, 0, None, self.verbose, self.dry_run)

import os, sys, string

from distutils.command import build_ext
from distutils.dir_util import mkpath
from distutils.file_util import copy_file
from distutils.spawn import spawn, find_executable
from distutils import sysconfig
from shutil import *

module_ext = sysconfig.get_config_var('SO')

class build_ext(build_ext.build_ext):

    extensions = [('Synopsis/Parser/C', 'ctool' + module_ext),
                  ('Synopsis/Parser/C++', 'occ' + module_ext),
                  ('Synopsis/Parser/C++', 'link' + module_ext),
                  ]

    def run(self):

        for ext in build_ext.extensions:
            self.build_extension(ext)

    def get_source_files(self):

        def collect(arg, path, files):
            files.extend(os.listdir(path))
            print path, os.listdir(path)
        files = []
        for ext in build_ext.extensions:
            print 'walking', ext
            os.path.walk(ext,
                         #lambda arg, path, files : print path, os.listdir(path),
                         collect,
                         files)
            print files
        return files
    
    def build_extension(self, ext):

        self.announce("building '%s' in %s"%(ext[1], ext[0]))

        path = os.path.join(self.build_temp, ext[0])
        if not os.path.exists(path):
            self.run_command('config')

        command = "make -C %s %s"%(path, ext[1])
        spawn(['sh', '-c', command], self.verbose, self.dry_run)
        #The extension may not be compiled. For now just skip it.
        if os.path.isfile(os.path.join(path, ext[1])):
            
            build_path = os.path.join(self.build_lib, ext[0])
            mkpath (build_path, 0777, self.verbose, self.dry_run)
            copy_file(os.path.join(path, ext[1]),
                      os.path.join(build_path, ext[1]),
                      1, 1, 0, None, self.verbose, self.dry_run)

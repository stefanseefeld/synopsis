import os, sys, string

from distutils.command import build_ext
from distutils.dir_util import mkpath
from distutils.spawn import spawn, find_executable
from shutil import *

class build_ext(build_ext.build_ext):

    extensions = ['Synopsis/Parser/C', 'Synopsis/Parser/C++']

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

        self.announce("building '%s'" % ext)

        path = os.path.join(self.build_temp, ext)
        if not os.path.exists(path):
            self.run_command('config')

        command = "make -C %s"%(path)
        spawn(['sh', '-c', command], self.verbose, self.dry_run)
        #if self.build_temp != self.build_lib:

import os, sys, string

from distutils.command import build_ext
from distutils.dir_util import mkpath
from distutils.file_util import copy_file
from distutils.spawn import spawn, find_executable
from shutil import *

class build_ext(build_ext.build_ext):

    def run(self):

        if not os.path.exists(self.build_temp):
            self.run_command('config')
        for ext in self.extensions:
            self.build_extension(ext)

    def check_extensions_list (self, extensions):
        pass # do nothing, trust that the extensions are correct

    def get_source_files(self):

        def collect(arg, path, files):
            files.extend(os.listdir(path))
            print path, os.listdir(path)
        files = []
        for ext in self.extensions:
            os.path.walk(ext[0], collect, files)
            print files
        return files

    def get_outputs(self):

        output = []
        for ext in self.extensions:
            #only append the files that actually could be built
            path = os.path.join(self.build_temp, ext[0], ext[1])
            if os.path.isfile(path):
                output.append(os.path.join(self.build_lib, ext[0], ext[1]))


        return output

    def build_extension(self, ext):

        self.announce("building '%s' in %s"%(ext[1], ext[0]))

        path = os.path.join(self.build_temp, ext[0])
        if not os.path.exists(path):
            self.announce("...not configured, skipping")
            return
        
        command = "make -C %s %s"%(path, ext[1])
        spawn(['sh', '-c', command], self.verbose, self.dry_run)
        #The extension may not be compiled. For now just skip it.
        if os.path.isfile(os.path.join(path, ext[1])):
            
            build_path = os.path.join(self.build_lib, ext[0])
            mkpath (build_path, 0777, self.verbose, self.dry_run)
            copy_file(os.path.join(path, ext[1]),
                      os.path.join(build_path, ext[1]),
                      1, 1, 0, None, self.verbose, self.dry_run)

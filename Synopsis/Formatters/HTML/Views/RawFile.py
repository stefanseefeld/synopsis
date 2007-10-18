#
# Copyright (C) 2000 Stephen Davies
# Copyright (C) 2000 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.Processor import Parameter
from Synopsis.Formatters.HTML.View import View
from Synopsis.Formatters.HTML.Tags import *
from Directory import compile_glob

import time, os, stat, os.path

class RawFile(View):
    """A module for creating a view for each file with hyperlinked source"""

    src_dir = Parameter('', 'starting point for directory listing')
    base_path = Parameter('', 'path prefix to strip off of the file names')
    exclude = Parameter([], 'TODO: define an exclusion mechanism (glob based ?)')

    def register(self, frame):

        super(RawFile, self).register(frame)
        self._exclude = [compile_glob(e) for e in self.exclude]
        self.__files = None

    def filename(self):

        return self.__filename

    def title(self):

        return self.__title

    def _get_files(self):
        """Returns a list of (path, output_filename) for each file."""

        if self.__files is not None: return self.__files
        self.__files = []
        dirs = [self.src_dir]
        while dirs:
            dir = dirs.pop(0)
            for entry in os.listdir(os.path.abspath(dir)):
                exclude = 0
                for re in self._exclude:
                    if re.match(entry):
                        exclude = 1
                        break
                if exclude:
                    continue
                entry_path = os.path.join(dir, entry)
                info = os.stat(entry_path)
                if stat.S_ISDIR(info[stat.ST_MODE]):
                    dirs.append(entry_path)
                else:
                    # strip of base_path
                    path = entry_path[len(self.base_path):]
                    if path[0] == '/': path = path[1:]
                    filename = self.directory_layout.file_source(path)
                    self.__files.append((entry_path, filename))
        return self.__files

    def process(self):
        """Creates a view for every file."""

        for path, filename in self._get_files():
            self.process_file(path, filename)

    def register_filenames(self):

        for path, filename in self._get_files():
            self.processor.register_filename(filename, self, path)

    def process_file(self, original, filename):
        """Creates a view for the given filename."""

        # Check that we got the rego
        reg_view, reg_scope = self.processor.filename_info(filename)
        if reg_view is not self: return

        self.__filename = filename
        self.__title = original[len(self.base_path):]
        self.start_file()
        self.write_navigation_bar()
        self.write('File: '+entity('b', self.__title))
        try:
            lines = open(original, 'rt').readlines()
            lineno_template = '%%%ds' % len(`len(lines)`)
            lines = ['<span class="lineno">%s</span><span class="line">%s</span>\n'
                     %(lineno_template % (i + 1), escape(l[:-1]))
                     for i, l in enumerate(lines)]
            self.write('<pre class="sxr">')
            self.write(''.join(lines))
            self.write('</pre>')
        except:
            self.write('An error occurred')
        self.end_file()


#
# Copyright (C) 2005 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from distutils.core import Command
from distutils.dir_util import remove_tree
from distutils import log
import os

from distutils.command.clean import clean as base

class clean(base):

    def run(self):

        base.run(self)

        if self.all:

            # Remove the build/clib.<plat> directory (unless it's already
            # gone).
            libdir = os.path.join(self.build_clib, 'lib')
            incdir = os.path.join(self.build_clib, 'include')
            bindir = os.path.join(self.build_clib, 'bin')
            for d in [libdir, incdir, bindir]:
                if os.path.exists(d):
                    remove_tree(d, dry_run=self.dry_run)
            pc = os.path.join(self.build_clib, 'synopsis.pc')
            if os.path.exists(pc) and not self.dry_run:
                os.remove(pc)

            # Remove the generated documentation.
            prefix = 'share/doc/synopsis/html/Manual'
            for d in ['cxx', 'python', 'sxr']:
                if os.path.exists(os.path.join(prefix, d)):
                    remove_tree(os.path.join(prefix, d), dry_run=self.dry_run)

            prefix = 'share/doc/synopsis/html'
            for d in ['Tutorial', 'DevGuide']:
                if os.path.exists(os.path.join(prefix, d)):
                    remove_tree(os.path.join(prefix, d), dry_run=self.dry_run)

            prefix = 'share/doc/synopsis'
            if os.path.exists(os.path.join(prefix, 'examples')):
                remove_tree(os.path.join(prefix, 'examples'), dry_run=self.dry_run)

        

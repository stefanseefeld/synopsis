#
# Copyright (C) 2005 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

import os
from distutils.core import Command
from distutils.dir_util import remove_tree
from distutils import log

from distutils.command.clean import clean

class clean_syn(clean):

    def finalize_options(self):
        
        clean.finalize_options(self)
        
        build_clib = self.distribution.get_command_obj('build_clib')
        build_clib.ensure_finalized()
        self.build_ctemp = build_clib.build_ctemp

    def run(self):

        clean.run(self)

        # Remove the build/ctemp.<plat> directory (unless it's already
        # gone).
        if os.path.exists(self.build_ctemp):
            remove_tree(self.build_ctemp, dry_run=self.dry_run)

        # Remove installed clib, if installed locally.
        for d in ['include', 'lib', 'bin']:
            if os.path.exists(d):
                remove_tree(d, dry_run=self.dry_run)

        # Remove the generated documentation.
        prefix = 'share/doc/Synopsis/html/Manual'
        for d in ['cxx', 'python', 'sxr']:
            if os.path.exists(os.path.join(prefix, d)):
                remove_tree(os.path.join(prefix, d), dry_run=self.dry_run)

        prefix = 'share/doc/Synopsis/html'
        for d in ['Tutorial', 'DevGuide']:
            if os.path.exists(os.path.join(prefix, d)):
                remove_tree(os.path.join(prefix, d), dry_run=self.dry_run)

        prefix = 'share/doc/Synopsis'
        if os.path.exists(os.path.join(prefix, 'examples')):
            remove_tree(os.path.join(prefix, 'examples'), dry_run=self.dry_run)

        

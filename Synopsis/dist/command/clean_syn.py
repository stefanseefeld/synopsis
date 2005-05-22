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

        # remove the build/ctemp.<plat> directory (unless it's already
        # gone)
        if os.path.exists(self.build_ctemp):
            remove_tree(self.build_ctemp, dry_run=self.dry_run)
        else:
            log.debug("'%s' does not exist -- can't clean it",
                      self.build_ctemp)

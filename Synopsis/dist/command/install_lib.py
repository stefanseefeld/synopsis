#
# Copyright (C) 2007 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#
from distutils.command.install_lib import install_lib as base
from Synopsis.dist.command import reset_config_variables
import os.path

class install_lib(base):

    def run(self):
        
        # Do the standard installation.
        base.run(self)
        
        config_file = os.path.join(self.install_dir, 'Synopsis', 'config.py')
        self.announce("adjusting config parameters")
        i = self.distribution.get_command_obj('install')
        prefix = i.root or i.prefix
        version = self.distribution.get_version()
        datadir=os.path.join(prefix, 'share', 'Synopsis-%s'%version)
        reset_config_variables(config_file,
                               prefix=prefix,
                               version=version,
                               datadir=datadir)

        # Make sure the new config file gets recompiled, or else python may
        # not notice it is in fact different from the original config file.
        files = [config_file]

        from distutils.util import byte_compile
        install_root = self.get_finalized_command('install').root

        if self.compile:
            byte_compile(files, optimize=0,
                         force=1, prefix=install_root,
                         dry_run=self.dry_run)
        if self.optimize > 0:
            byte_compile(files, optimize=self.optimize,
                         force=1, prefix=install_root,
                         verbose=self.verbose, dry_run=self.dry_run)


#
# Copyright (C) 2007 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#
from distutils.command.install import install as base

class install(base):

    sub_commands = base.sub_commands + [('install_clib', None)]

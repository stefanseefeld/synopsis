import os, sys, string

from distutils.command import build

class build(build.build):

    sub_commands = build.build.sub_commands[:] + [('build_doc', None)]

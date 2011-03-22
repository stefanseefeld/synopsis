#
# Copyright (C) 2011 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

class Error(Exception):
    """An exception a processor may raise during processing."""

    def __init__(self, what):

        self.what = what

    def __str__(self):
        return "%s: %s"%(self.__class__.__name__, self.what)

class InvalidArgument(Error): pass
class MissingArgument(Error): pass
class InvalidCommand(Error): pass
class InternalError(Error): pass


#
# Copyright (C) 2003 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from __future__ import generators
from Processor import Error
import sys

class CommandLineError(Error): pass

def parse_option(arg):
    """The required format is '--option[=[arg]]' or 'option=arg'.
    In the former case the optional argument is interpreted as
    a string (only '--option' sets the value to True, '--option='
    sets it to the empty string), in the latter case the argument
    is evaluated as a python expression.
    Returns (None, None) for non-option argument"""
   
    if arg.find('=') == -1 and not arg.startswith('--'):
        return None, None # we are done
    attribute = arg.split('=', 1)
    if len(attribute) == 2:
        name, value = attribute
        if name.startswith('--'):
            name = name[2:] # value is a string
        else:
            try:
                value = eval(value) # it's a python expression
            except:
                sys.stderr.write("""an error occured trying to evaluate the value of \'%s\' (\'%s\')
to pass this as a string, please use %s="'%s'" \n"""%(name, value, name, value))
                sys.exit(-1)
    else:
        name, value = attribute[0][2:], True # flag the attribute as 'set'

    return name, value


def get_options(args, parse_arg = parse_option, expect_non_options = True):
    """provide an iterator over the options in args.
    All found options are stripped, such that args will
    contain the remainder, i.e. non-option arguments.
    Pass each argument to the parse_option function to
    extract the (name,value) pair. Returns as soon as
    the first non-option argument was detected.
    """

    while args:
        name, value = parse_arg(args[0])
        if name:
            args[:] = args[1:]
            yield name, value
        elif not expect_non_options:
            raise CommandLineError("expected option, got '%s'"%args[0])
        else:
            return
      

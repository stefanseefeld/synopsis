"""Parser for C++ using OpenC++ for low-level parsing.
This parser is written entirely in C++, and compiled into shared libraries for
use by python.
@see C++/Synopsis
@see C++/SWalker
"""
#
# configure for the parser you want here...
#
try:
    from occ import parse, usage
except:
    import sys
    print sys.exc_type, sys.exc_value
import emul

# THIS-IS-A-PARSER

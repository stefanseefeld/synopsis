"""Parser for C using CTool for low-level parsing.
This parser is written entirely in C++, and compiled into shared libraries for
use by python.
"""
#
# configure for the parser you want here...
#
try:
    from ctool import parse, usage
except:
    import sys
    print sys.exc_type, sys.exc_value

# THIS-IS-A-PARSER

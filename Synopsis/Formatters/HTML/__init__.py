"""HTML Formatter"""
#
# __init__.py
#
# The formatter has been split into a very modular format.
# The modules in the HTML package now contain the classes which may be
# overridden by user-specified modules

try:
    import core

    from core import format

except:
    print "An error occurred loading the HTML module:"
    import traceback
    traceback.print_exc()
    raise

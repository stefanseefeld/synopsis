"""ASG processors.

There is much that can be done to an ASG to manipulate it. This includes:
merging multiple ASG trees, mapping links to different languages, pruning the
tree, mapping names to different scopes, and dealing with comment prefixes and
groupings."""

from Linker import *
from ScopeStripper import *
from NameMapper import *
from MacroFilter import *
from ModuleFilter import *
from SXRCompiler import *
from AccessRestrictor import *
from TypedefFolder import *
from TypeMapper import *
import Comments

"""Package for linking modules.

There is much that can be done to an AST to manipulate it. This includes:
merging multiple AST trees, mapping links to different languages, pruning the
tree, mapping names to different scopes, and dealing with comment prefixes and
groupings."""

from Linker import *
from Stripper import *
from NameMapper import *
from Comments import *
from XRefCompiler import *
from EmptyModuleRemover import *
from AccessRestrictor import *
from TypeMapper import *
from LanguageMapper import *

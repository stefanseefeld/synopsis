"""Simple code to extract class & function docstrings from a module.

This code is used as an example in the library reference manual in the
section on using the parser module.  Refer to the manual for a thorough
discussion of the operation of this code.

The code has been extended by Stephen Davies for the Synopsis project. It now
also recognises parameter names and values, and baseclasses. Names are now
returned in order also.
"""

import symbol
import token
import types
import string

import pprint

def format(tree, depth=-1):
    """Format the given tree up to the given depth.
    Numbers are replaced with their symbol or token names."""
    if type(tree) == types.IntType:
	try:
	    return symbol.sym_name[tree]
	except KeyError:
	    try:
		return token.tok_name[tree]
	    except KeyError:
		return tree
    if type(tree) != types.TupleType:
	return tree
    if depth == 0: return '...'
    ret = [format(tree[0])]
    for branch in tree[1:]:
	ret.append(format(branch, depth-1))
    return tuple(ret)

def stringify(tree):
    """Convert the given tree to a string"""
    if type(tree) == types.IntType: return ''
    if type(tree) != types.TupleType:
	return str(tree)
    strs = []
    for elem in tree:
	strs.append(stringify(elem))
    return string.join(strs, '')

def get_docs(fileName):
    """Retrieve information from the parse tree of a source file.

    fileName
        Name of the file to read Python source code from.
    """
    source = open(fileName).read()
    import os
    basename = os.path.basename(os.path.splitext(fileName)[0])
    import parser
    ast = parser.suite(source)
    tup = parser.ast2tuple(ast)
    return ModuleInfo(tup, basename)


class SuiteInfoBase:
    _docstring = ''
    _name = ''

    def __init__(self, tree = None):
        self._class_info = {}
	self._class_names = []
        self._function_info = {}
	self._function_names = []
        if tree:
            self._extract_info(tree)

    def _extract_info(self, tree):
        # extract docstring
        if len(tree) == 2:
            found, vars = match(DOCSTRING_STMT_PATTERN[1], tree[1])
        else:
            found, vars = match(DOCSTRING_STMT_PATTERN, tree[3])
        if found:
            self._docstring = eval(vars['docstring'])
        # discover inner definitions
        for node in tree[1:]:
            found, vars = match(COMPOUND_STMT_PATTERN, node)
            if found:
                cstmt = vars['compound']
                if cstmt[0] == symbol.funcdef:
                    name = cstmt[2][1]
                    self._function_info[name] = FunctionInfo(cstmt)
		    self._function_names.append(name)
                elif cstmt[0] == symbol.classdef:
                    name = cstmt[2][1]
                    self._class_info[name] = ClassInfo(cstmt)
		    self._class_names.append(name)

    def get_docstring(self):
        return self._docstring

    def get_name(self):
        return self._name

    def get_class_names(self):
        return self._class_names

    def get_class_info(self, name):
        return self._class_info[name]

    def __getitem__(self, name):
        try:
            return self._class_info[name]
        except KeyError:
            return self._function_info[name]


class SuiteFuncInfo:
    #  Mixin class providing access to function names and info.

    def get_function_names(self):
        return self._function_names

    def get_function_info(self, name):
        return self._function_info[name]


class FunctionInfo(SuiteInfoBase, SuiteFuncInfo):
    def __init__(self, tree = None):
        self._name = tree[2][1]
        SuiteInfoBase.__init__(self, tree and tree[-1] or None)
	self._params = []
	self._param_defaults = []
	if tree[3][0] == symbol.parameters:
	    if tree[3][2][0] == symbol.varargslist:
		args = list(tree[3][2][1:])
		while args:
		    if args[0][0] == token.COMMA:
			pass
		    elif args[0][0] == symbol.fpdef:
			self._params.append(stringify(args[0]))
			self._param_defaults.append('')
		    elif args[0][0] == token.EQUAL:
			del args[0]
			self._param_defaults[-1] = stringify(args[0])
		    elif args[0][0] == token.DOUBLESTAR:
			del args[0]
			self._params.append('**'+stringify(args[0]))
			self._param_defaults.append('')
		    else:
			print "Unknown symbol:",args[0]
		    del args[0]
    
    def get_params(self): return self._params
    def get_param_defaults(self): return self._param_defaults


class ClassInfo(SuiteInfoBase):
    def __init__(self, tree = None):
        self._name = tree[2][1]
        SuiteInfoBase.__init__(self, tree and tree[-1] or None)
	self._bases = []
	if tree[4][0] == symbol.testlist:
	    for test in tree[4][1:]:
		found, vars = match(TEST_NAME_PATTERN, test)
		if found and vars.has_key('name'):
		    self._bases.append(vars['name'])

    def get_method_names(self):
        return self._function_names

    def get_method_info(self, name):
        return self._function_info[name]

    def get_base_names(self):
	return self._bases


class ModuleInfo(SuiteInfoBase, SuiteFuncInfo):
    def __init__(self, tree = None, name = "<string>"):
        self._name = name
        SuiteInfoBase.__init__(self, tree)
        if tree:
            found, vars = match(DOCSTRING_STMT_PATTERN, tree[1])
            if found:
                self._docstring = vars["docstring"]


from types import ListType, TupleType

def match(pattern, data, vars=None):
    """Match `data' to `pattern', with variable extraction.

    pattern
        Pattern to match against, possibly containing variables.

    data
        Data to be checked and against which variables are extracted.

    vars
        Dictionary of variables which have already been found.  If not
        provided, an empty dictionary is created.

    The `pattern' value may contain variables of the form ['varname'] which
    are allowed to match anything.  The value that is matched is returned as
    part of a dictionary which maps 'varname' to the matched value.  'varname'
    is not required to be a string object, but using strings makes patterns
    and the code which uses them more readable.

    This function returns two values: a boolean indicating whether a match
    was found and a dictionary mapping variable names to their associated
    values.
    """
    if vars is None:
        vars = {}
    if type(pattern) is ListType:       # 'variables' are ['varname']
        vars[pattern[0]] = data
        return 1, vars
    if type(pattern) is not TupleType:
        return (pattern == data), vars
    if len(data) != len(pattern):
        return 0, vars
    for pattern, data in map(None, pattern, data):
        same, vars = match(pattern, data, vars)
        if not same:
            break
    return same, vars


def dmatch(pattern, data, vars=None):
    """Debugging match """
    if vars is None:
        vars = {}
    if type(pattern) is ListType:       # 'variables' are ['varname']
        vars[pattern[0]] = data
	print "dmatch: pattern is list,",pattern[0],"=",data
        return 1, vars
    if type(pattern) is not TupleType:
	print "dmatch: pattern is not tuple, pattern =",format(pattern)," data =",format(data)
        return (pattern == data), vars
    if len(data) != len(pattern):
	print "dmatch: bad length."
        return 0, vars
    for pattern, data in map(None, pattern, data):
        same, vars = dmatch(pattern, data, vars)
        if not same:
	    print "dmatch: not same"
            break
	print "dmatch: same so far"
    print "dmatch: returning",same,vars
    return same, vars


#  This pattern identifies compound statements, allowing them to be readily
#  differentiated from simple statements.
#
COMPOUND_STMT_PATTERN = (
    symbol.stmt,
    (symbol.compound_stmt, ['compound'])
    )


#  This pattern will match a 'stmt' node which *might* represent a docstring;
#  docstrings require that the statement which provides the docstring be the
#  first statement in the class or function, which this pattern does not check.
#
DOCSTRING_STMT_PATTERN = (
    symbol.stmt,
    (symbol.simple_stmt,
     (symbol.small_stmt,
      (symbol.expr_stmt,
       (symbol.testlist,
        (symbol.test,
         (symbol.and_test,
          (symbol.not_test,
           (symbol.comparison,
            (symbol.expr,
             (symbol.xor_expr,
              (symbol.and_expr,
               (symbol.shift_expr,
                (symbol.arith_expr,
                 (symbol.term,
                  (symbol.factor,
                   (symbol.power,
                    (symbol.atom,
                     (token.STRING, ['docstring'])
                     )))))))))))))))),
     (token.NEWLINE, '')
     ))

#  This pattern will match a 'test' node which is a base class
#
TEST_NAME_PATTERN = (
        symbol.test,
         (symbol.and_test,
          (symbol.not_test,
           (symbol.comparison,
            (symbol.expr,
             (symbol.xor_expr,
              (symbol.and_expr,
               (symbol.shift_expr,
                (symbol.arith_expr,
                 (symbol.term,
                  (symbol.factor,
                   (symbol.power,
                    (symbol.atom,
                     (token.NAME, ['name'])
                     ))))))))))))
     )

#
#  end of file

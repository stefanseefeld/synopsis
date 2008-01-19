import token
import symbol
import sys

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
    if type(pattern) is list:       # 'variables' are ['varname']
        vars[pattern[0]] = data
        return 1, vars
    if type(pattern) is not tuple:
        return (pattern == data), vars
    if len(data) != len(pattern):
        return 0, vars
    for pattern, data in map(None, pattern, data):
        same, vars = match(pattern, data, vars)
        if not same:
            break
    return same, vars


COMPOUND_STMT_PATTERN = (
    symbol.stmt,
    (symbol.compound_stmt, ['compound'])
    )
"""This pattern identifies compound statements, allowing them to be readily
differentiated from simple statements."""


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
"""This pattern will match a 'stmt' node which *might* represent a docstring;
docstrings require that the statement which provides the docstring be the
first statement in the class or function, which this pattern does not check."""

if sys.version_info[:2] < (2, 5):

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
                  ['power']
                  ))))))))))
        )
    """This pattern will match a 'test' node which is a base class."""

else:

    TEST_NAME_PATTERN = (
        symbol.test,
        (symbol.or_test,
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
                   ['power']
                   )))))))))))
        )
    """This pattern will match a 'test' node which is a base class."""

ATOM_PATTERN = (
    symbol.testlist,
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
                 ['atom']
                 )))))))))))))
    )

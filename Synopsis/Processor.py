#
# Copyright (C) 2003 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

import AST

class Error(Exception):
   """An exception a processor may raise during processing."""

   def __init__(self, what):

      self.what = what

   def __str__(self):
      return "%s: %s"%(self.__class__.__name__, self.what)

class InvalidArgument(Error): pass
class MissingArgument(Error): pass
class InvalidCommand(Error): pass

class Parameter(object):
   """A Parameter is a documented value, kept inside a Processor."""
   def __init__(self, value, doc):
      self.value = value
      self.doc = doc

class Type(type):
   """Type is the Processor's __metaclass__."""
   def __init__(cls, name, bases, dict):
      """Generate a '_parameters' dictionary holding all the 'Parameter' objects.
      Then replace 'Parameter' objects by their values for convenient use inside
      the code."""
      parameters = {}
      for i in dict:
         if isinstance(dict[i], Parameter):
            parameters[i] = dict[i]
      for i in parameters:
         setattr(cls, i, dict[i].value)
      setattr(cls, '_parameters', parameters)

class Parametrized(object):
   """Parametrized implements handling of Parameter attributes."""

   __metaclass__ = Type
   
   def __new__(cls, *args, **kwds):
      """merge all parameter catalogs for easy access to documentation,
      then use keyword arguments to override default values."""
      instance = object.__new__(cls)
      # iterate over all base classes, starting at the 'Parametrized' base class
      # i.e. remove mixin classes
      hierarchy = list(filter(lambda i:issubclass(i, Parametrized), cls.__mro__))
      hierarchy.reverse()
      parameters = {}
      for c in hierarchy:
         parameters.update(c._parameters)
      setattr(instance, '_parameters', parameters)

      for p in kwds:
         if not p in instance._parameters:
            raise InvalidArgument('"%s.%s" processor does not have "%s" parameter'
                                  %(cls.__module__, cls.__name__, p))
         else:
            setattr(instance, p, kwds[p])

      return instance

   def __init__(self, **kwds):
      """The constructor uses the keywords to update the parameter list."""
      
      self.set_parameters(kwds)

   def get_parameters(self):

      return self._parameters

   def set_parameters(self, kwds):
      """Sets the given parameters to override the default values."""
      for i in kwds:
         if i in self._parameters:
            setattr(self, i, kwds[i])
         else:
            raise InvalidArgument, "No parameter '%s' in '%s'"%(i, self.__class__.__name__)


class Processor(Parametrized):
   """Processor documentation..."""

   verbose = Parameter(False, "operate verbosely")
   debug = Parameter(False, "generate debug traces")
   profile = Parameter(False, "output profile data")
   input = Parameter([], "input files to process")
   output = Parameter('', "output file to save the ast to")

   def merge_input(self, ast):
      """Join the given ast with a set of asts to be read from 'input' parameter"""
      input = getattr(self, 'input', [])
      for file in input:
         ast.merge(AST.load(file))
      return ast

   def output_and_return_ast(self):
      """writes output if the 'output' attribute is set, then returns"""
      output = getattr(self, 'output', None)
      if output:
         self.ast.save(output)
      return self.ast

   def process(self, ast, **kwds):
      """The process method provides the interface to be implemented by subclasses.
      
      Commonly used arguments are 'input' and 'output'. If 'input' is defined,
      it is interpreted as one or more input file names. If 'output' is defined, it
      is interpreted as an output file (or directory) name.
      This implementation may serve as a template for real processors."""

      # override default parameter values
      self.set_parameters(kwds)
      # merge in ast from 'input' parameter if given
      self.ast = self.merge_input(ast)

      # do the real work here...
      
      # write to output (if given) and return ast
      return self.output_and_return_ast()

class Composite(Processor):
   """A Composite processor."""

   processors = Parameter([], 'the list of processors this is composed of')

   def __init__(self, *processors, **kwds):
      """This __init__ is a convenience constructor that takes a var list
      to list the desired processors. If the named values contain 'processors',
      they override the var list."""
      if processors: self.processors = processors
      self.set_parameters(kwds)

   def process(self, ast, **kwds):
      """apply a list of processors. The 'input' value is passed to the first
      processor only, the 'output' to the last. 'verbose' and 'debug' are
      passed down if explicitely given as named values.
      All other keywords are ignored."""

      if not self.processors:
         return super(Composite, self).process(ast, **kwds)

      self.set_parameters(kwds)

      if len(self.processors) == 1:
         my_kwds = {}
         if self.input: my_kwds['input'] = self.input
         if self.output: my_kwds['output'] = self.output
         if self.verbose: my_kwds['verbose'] = self.verbose
         if self.debug: my_kwds['debug'] = self.debug
         if self.profile: my_kwds['profile'] = self.profile
         return self.processors[0].process(ast, **my_kwds)

      # more than one processor...
      # call the first, passing the 'input' parameter, if present
      my_kwds = {}
      if self.input: my_kwds['input'] = self.input
      if self.verbose: my_kwds['verbose'] = self.verbose
      if self.debug: my_kwds['debug'] = self.debug
      if self.profile: my_kwds['profile'] = self.profile
      ast = self.processors[0].process(ast, **my_kwds)

      # deal with all between the first and the last;
      # they only get 'verbose', 'debug', and 'profile' flags
      my_kwds = {}
      if self.verbose: my_kwds['verbose'] = self.verbose
      if self.debug: my_kwds['debug'] = self.debug
      if self.profile: my_kwds['profile'] = self.profile
      if len(self.processors) > 2:
         for p in self.processors[1:-1]:
            ast = p.process(ast, **my_kwds)

      # call the last, passing the 'output' parameter, if present
      if self.output: my_kwds['output'] = self.output
      ast = self.processors[-1].process(ast, **my_kwds)

      return ast
   
class Store(Processor):
   """Store is a convenience class useful to write out the intermediate
   state of the ast within a pipeline such as represented by the 'Composite'"""

   def process(self, ast, **kwds):
      """Simply store the current ast in the 'output' file."""

      self.set_parameters(kwds)
      self.ast = self.merge_input(ast)
      return self.output_and_return_ast()

# $Id: Processor.py,v 1.3 2003/11/11 18:17:17 stefan Exp $
#
# Copyright (C) 2003 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

import AST

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
      hierarchy = list(filter(lambda i:isinstance(i, Parametrized), cls.__mro__))
      hierarchy.reverse()
      parameters = {}
      for c in hierarchy:
         parameters.update(c._parameters)
         setattr(instance, '_parameters', parameters)

      for p in kwds:
         if not p in instance._parameters:
            raise KeyError, "'%s' processor doesn't have '%s' parameter"%(cls.__name__, p)
         else:
            setattr(cls, p, kwds[p])

      return instance

   def get_parameters(self):
      return self._parameters

   def set_parameters(self, kwds):
      """Sets the given parameters to override the default values."""
      for i in kwds:
         if i in self._parameters:
            setattr(self, i, kwds[i])
         else:
            raise TypeError, "No parameter '%s' in 'Parametrized'"%(i)


class Processor(Parametrized):
   """Processor documentation..."""

   verbose = Parameter(False, "operate verbosely")

   def set_parameters(self, kwds):
      """Sets the given parameters to override the default values.
      Override the Parametrized version so we can handle 'input' and 'output'
      which are not in the _parameters' catalog."""
      for i in kwds:
         if i in self._parameters:
            setattr(self, i, kwds[i])
         elif i == 'input' or i == 'output':
            setattr(self, i, kwds[i]) # these are not in self._parameters but are legal
         else:
            raise TypeError, "No parameter '%s' in processor '%s'"%(i, self.__class__.__name__)

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
         AST.save(output, self.ast)
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
      self.merge_input(ast)

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
      self.__dict__.update(kwds)

   def process(self, ast, **kwds):
      """apply a list of processors. The 'input' value is passed to the first
      processor only, the 'output' to the last."""

      if not self.processors:
         return ast

      elif len(self.processors) == 1:
         return self.processors[0].process(ast, **kwds)
      
      # first_kwds is a copy of kwds, but without 'output' defined
      first_kwds = kwds.copy()
      if first_kwds.has_key('output'):
         del first_kwds['output']
      # last_kwds is a copy of kwds, but without 'input' defined
      last_kwds = kwds.copy()
      if last_kwds.has_key('input'):
         del last_kwds['input']
      # remove 'input' and 'output' from kwds for all other processors
      # in the pipeline
      if kwds.has_key('input'):
         del kwds['input']
      if kwds.has_key('output'):
         del kwds['output']

      ast = self.processors[0].process(ast, **first_kwds)

      if len(self.processors) > 2:
         for p in self.processors[1:-1]:
            ast = p.process(ast, **kwds)

      return self.processors[-1].process(ast, **last_kwds)

class Store(Processor):
   """Store is a convenience class useful to write out the intermediate
   state of the ast within a pipeline such as represented by the 'Composite'"""

   output = Parameter('', 'the output filename')

   def process(self, ast, **kwds):
      """Simply store the current ast in the 'output' file."""

      self.__dict__.update(kwds)
      return self.output_and_return()

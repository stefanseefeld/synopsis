#! /usr/bin/env python

"""This tool inspects synopsis processors and generates a docbook section
with a listing of all parameters for inclusion into the tutorial."""

from Synopsis.import_processor import import_processor
from Synopsis.Processor import Parametrized
import sys, string, types

def name_of_instance(instance):
   name = string.split(instance.__module__, '.')
   if name[-1] != instance.__class__.__name__:
      name.append(instance.__class__.__name__)
   
   return '%s'%'.'.join(name)

def id_of_instance(instance):
   name = string.split(instance.__module__, '.')[-2:]
   if name[-1] != instance.__class__.__name__:
      name = [name[-1], instance.__class__.__name__]
   return '%s'%'-'.join(name)

if not len(sys.argv) == 3:
   print 'Usage : %s <processor> <output>'%sys.argv[0]
   sys.exit(-1)
   
try:
   processor = import_processor(sys.argv[1])
except ImportError, msg:
   sys.stderr.write('Error : %s\n'%msg)
   sys.exit(-1)

p = processor()
parameters = p.get_parameters()

output = open(sys.argv[2], 'w+')
output.write('<?xml version="1.0" encoding="UTF-8"?>\n')
output.write('<!DOCTYPE book PUBLIC "-//OASIS//DTD DocBook XML V4.2//EN"\n')
output.write('"http://www.oasis-open.org/docbook/xml/4.2/docbookx.dtd">\n')
output.write('<section id="%s-ref">\n'%id_of_instance(p))
output.write('  <title>%s</title>\n'%name_of_instance(p))

output.write('  <segmentedlist>\n')
output.write('    <segtitle>Name</segtitle>\n')
output.write('    <segtitle>Default value</segtitle>\n')
output.write('    <segtitle>Description</segtitle>\n')

for name in parameters:
   output.write('    <seglistitem>\n')
   output.write('      <seg>%s</seg>\n'%name)
   parameter = parameters[name]
   value = parameter.value
   if type(value) == types.InstanceType or isinstance(value, Parametrized):
      value = name_of_instance(value)
   elif type(value) == list:
      if (len(value) and (type(value[0]) == types.InstanceType
                          or isinstance(value[0], Parametrized))):
         value = '[%s]'%', '.join([name_of_instance(x) for x in value])
   elif type(value) == dict:
      if (len(value) and (type(value.values()[0]) == types.InstanceType
                          or isinstance(value.values()[0], Parametrized))):
         value = '{%s}'%', '.join(['%s: %s'%(n, name_of_instance(x))
                                   for (n,v) in value.items()])
   output.write('      <seg>%s</seg>\n'%value)
   output.write('      <seg>%s</seg>\n'%parameter.doc)
   output.write('    </seglistitem>\n')

output.write('  </segmentedlist>')
output.write('</section>\n')

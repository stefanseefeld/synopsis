#! /usr/bin/env python

"""This tool inspects synopsis processors and generates a docbook section
with a listing of all parameters for inclusion into the tutorial."""

from Synopsis.import_processor import import_processor
from Synopsis.Processor import Parametrized
import sys, string, types

def name_of_instance(instance):
   return '%s.%s'%(instance.__module__, instance.__class__.__name__)

if not len(sys.argv) == 3:
   print 'Usage : %s <processor> <output>'%sys.argv[0]
   sys.exit(-1)
   
try:
   processor = import_processor(sys.argv[1])
except ImportError, msg:
   sys.stderr.write('Error : %s'%msg)
   sys.exit(-1)

p = processor()
parameters = p.get_parameters()

output = open(sys.argv[2], 'w+')
output.write('<?xml version="1.0" encoding="UTF-8"?>\n')
output.write('<!DOCTYPE book PUBLIC "-//OASIS//DTD DocBook XML V4.2//EN"\n')
output.write('"http://www.oasis-open.org/docbook/xml/4.2/docbookx.dtd">\n')
output.write('<section>\n')
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
   elif type(value) == types.ListType:
      if (len(value)
          and (type(value[0]) == types.InstanceType
               or isinstance(value[0], Parametrized))):
         value = '[%s]'%string.join(map(lambda x:name_of_instance(x), value), ', ')
   output.write('      <seg>%s</seg>\n'%value)
   output.write('      <seg>%s</seg>\n'%parameter.doc)
   output.write('    </seglistitem>\n')

output.write('  </segmentedlist>')
output.write('</section>\n')

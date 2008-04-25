#
# Copyright (C) 2008 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.Processor import *
from Synopsis import ASG

class TemplateLinker(Processor, ASG.Visitor):
    """Link template specializations to their primary templates, and vice versa."""

    def process(self, ir, **kwds):

        self.set_parameters(kwds)
        self.ir = self.merge_input(ir)

        for d in ir.asg.declarations:
            d.accept(self)
        return self.output_and_return_ir()

    def link(self, d):

        if d.is_template_specialization:
            primary_name = d.name[:-1] + (d.name[-1].split('<')[0].strip(),)
            primary = self.ir.asg.types.get(primary_name)
            d.primary_template = primary_name
            if (type(primary) is ASG.DeclaredTypeId and
                d.name not in primary.declaration.specializations):
                primary.declaration.specializations.append(d.name)

    visit_forward = link
    visit_class = link
    visit_class_template = link

#
# Copyright (C) 2008 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.Formatters.HTML.Tags import *
from Synopsis.Formatters.HTML.Fragment import Fragment

class TemplateSpecializations(Fragment):
    """Cross-link primary templates with their specializations."""

    def format_forward(self, forward):

        if not forward.template:
            return ''
        if forward.specializations:
            spec = '<br/>'.join([self.reference(s.name)
                                 for s in forward.specializations])
            return div('specializations', 'Specializations: ' + spec)
        elif forward.primary_template:
            return div('primary-template',
                       'Primary template: ' + self.reference(forward.primary_template.name))
        return ''

    def format_class(self, class_):

        if class_.name[-1].endswith('>'):
            if not class_.primary_template:
                print class_.name, class_.file.name, class_.line
            return div('primary-template',
                       'Primary template: ' + self.reference(class_.primary_template.name))
        return ''

    def format_class_template(self, template_):

        if template_.specializations:
            spec = '<br/>'.join([self.reference(s.name)
                                 for s in template_.specializations])
            return div('specializations', 'Specializations: ' + spec)
        elif template_.primary_template:
            return div('primary-template',
                       'Primary template: ' + self.reference(template_.primary_template.name))
        return ''

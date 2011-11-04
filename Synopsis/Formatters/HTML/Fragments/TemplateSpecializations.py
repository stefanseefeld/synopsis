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

    # FIXME: Why are links between primary template and specializations
    #        passed by name (and then looked up in the Part.reference()
    #        method ? They have to be part of the same compilation unit
    #        anyhow...

    def format_forward(self, forward):

        if not forward.template:
            return ''
        if forward.specializations:
            spec = '\n'.join([div(self.reference(s.name))
                              for s in forward.specializations])
            return div('Specializations: ' + div(spec),
                       class_='specializations')
        elif forward.primary_template:
            return div('Primary template: ' + self.reference(forward.primary_template.name),
                       class_='primary-template')
        return ''

    def format_class(self, class_):

        if class_.primary_template:
            return div('Primary template: ' + self.reference(class_.primary_template.name),
                       class_='primary-template')
        return ''

    def format_class_template(self, template_):

        if template_.specializations:
            spec = ' '.join([div(self.reference(s.name))
                             for s in template_.specializations])
            return div('Specializations: ' + spec, class_='specializations')
        elif template_.primary_template:
            return div('Primary template: ' + self.reference(template_.primary_template.name),
                       class_='primary-template')
        return ''

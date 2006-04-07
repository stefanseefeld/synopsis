from Synopsis.process import process
from Synopsis.Processor import Processor, Parameter, Composite
from Synopsis.Parsers import Cxx
from Synopsis.Parsers import Python
from Synopsis.Processors import Linker
from Synopsis.Processors import Comments
from Synopsis.Formatters import HTML
from Synopsis.Formatters import Dot
from Synopsis.Formatters import Dump

class Joker(Processor):
    
    parameter = Parameter(':-)', 'a friendly parameter')

    def process(self, ast, **keywords):
        # override default parameter values
        self.set_parameters(keywords)
        # merge in ast from 'input' parameter if given
        self.ast = self.merge_input(ast)

        print 'this processor is harmless...', self.parameter
      
        # write to output (if given) and return ast
        return self.output_and_return_ast()

cxx = Cxx.Parser(base_path='../src')
cxx_ssd = Composite(cxx,
                    Comments.SSDFilter(),
                    Comments.Translator())
cxx_ss = Composite(cxx,
                   Comments.SSFilter(),
                   Comments.Translator())
cxx_ssd_prev = Composite(cxx,
                         Comments.SSDFilter(),
                         Comments.Grouper(),
                         Comments.Previous(),
                         Comments.Translator())
cxx_javadoc = Composite(cxx,
                        Comments.JavaFilter(),
                        Comments.Grouper(),
                        Comments.Translator(markup='javadoc'))


process(cxx_ssd = cxx_ssd,
        cxx_ss = cxx_ss,
        cxx_ssd_prev = cxx_ssd_prev,
        cxx_javadoc = cxx_javadoc,
        link = Linker(),
        html = HTML.Formatter(),
        dot = Dot.Formatter(),
        joker = Joker(parameter = '(-;'))

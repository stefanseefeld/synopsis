from Synopsis.process import process
from Synopsis.Processor import Processor, Parameter, Composite
from Synopsis.Parsers import Cxx
from Synopsis.Processors import Linker
from Synopsis.Processors.Comments import SSComments
from Synopsis.Processors.Comments import SSDComments
from Synopsis.Processors.Comments import JavaComments
from Synopsis.Processors.Comments import Previous
from Synopsis.Processors.Comments import JavaTags
from Synopsis.Processors.Comments import Grouper1
from Synopsis.Formatters import HTML
from Synopsis.Formatters.HTML import Comments
from Synopsis.Formatters import Dot

cxx = Cxx.Parser(base_path='../src')

cxx_ssd = Composite(cxx, SSDComments())

html = HTML.Formatter(comment_formatters = [Comments.QuoteHTML(),
                                            Comments.Section(),
                                            Comments.Javadoc()])

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
        

process(cxx_ssd = cxx_ssd,
        cxx_ss = Composite(cxx, SSComments()),
        cxx_ssd_prev = Composite(cxx, SSDComments(), Previous()),
        cxx_javadoc = Composite(cxx, JavaComments(), JavaTags()),
        link = Linker(Grouper1()),
        html = html,
        dot = Dot.Formatter(),
        joker = Joker(parameter = '(-;'))

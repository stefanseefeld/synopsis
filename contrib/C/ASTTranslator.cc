//
// Copyright (C) 2005 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#include "ASTTranslator.hh"
#include "TypeTranslator.hh"
#include <Synopsis/Trace.hh>
#include <Synopsis/PTree/Writer.hh> // for PTree::reify

using namespace Synopsis;

ASTTranslator::ASTTranslator(Synopsis::AST::AST ast, bool v, bool d)
  : my_ast(ast),
    my_lineno(0),
    my_types(my_ast.types(), v, d),
    my_verbose(v), my_debug(d) 
{
  Trace trace("ASTTranslator::ASTTranslator");  
}

void ASTTranslator::translate(PTree::Node *ptree, Buffer &buffer)
{
  Trace trace("ASTTranslator::translate");
  my_buffer = &buffer;
  ptree->accept(this);
}

void ASTTranslator::visit(PTree::List *node)
{
  if (node->car()) node->car()->accept(this);
  if (node->cdr()) node->cdr()->accept(this);
}

void ASTTranslator::visit(PTree::Declarator *decl)
{
  Trace trace("ASTTranslator::visit(PTree::Declarator *)");
  PTree::Encoding name = decl->encoded_name();
  PTree::Encoding type = decl->encoded_type();
  if (type.is_function())
  {
    // only declare once...
    trace << "declare function " << name;
 //    tt.translate(type);
    AST::Type return_type;
    AST::Function::Parameters parameters;
    AST::ScopedName n(std::string(name.begin() + 1, name.end()));
//     AST::Function func = my_kit.create_function(this->file, -1, "C", "function",
// 						AST::Modifiers(),
// 						return_type,
// 						name,
// 						this->symbol);
  }
  else
  {
    trace << "declare variable " << name;
  }
}

void ASTTranslator::visit(PTree::Typedef *typed)
{
  Trace trace("ASTTranslator::visit(PTree::Typedef *)");

  for (PTree::Node *d = PTree::third(typed); d; d = PTree::tail(d, 2))
  {
    if(PTree::type_of(d->car()) != Token::ntDeclarator)  // is this check necessary
      continue;
    
    PTree::Declarator *declarator = static_cast<PTree::Declarator *>(d->car());
    PTree::Encoding name = declarator->encoded_name();
    PTree::Encoding type = declarator->encoded_type();
    trace << "declare type " << name << " (" << type << ')';
    assert(name.is_simple_name());
    size_t length = (name.front() - 0x80);
    AST::ScopedName qname(std::string(name.begin() + 1, name.begin() + 1 + length));
    AST::Type alias = my_types.lookup(type);
    AST::Declaration declaration = my_ast_kit.create_typedef(my_file, 0, "C",
							     "typedef",
							     qname,
							     alias, false);
    my_types.declare(qname, declaration);
    
    add_comments(declaration, declarator->get_comments());
  }
}

void ASTTranslator::add_comments(AST::Declaration declarator, PTree::Node *c)
{
  Trace trace("ASTTranslator::add_comments");

  if (!declarator || !c) return;
  
  Python::List comments;

  // Loop over all comments in the list
  for (PTree::Node *next = PTree::rest(c); c && !c->is_atom(); next = PTree::rest(c))
  {
    PTree::Node *first = PTree::first(c);
    if (!first || !first->is_atom())
    {
      c = next;
      continue;
    }
    // update position information for this comment
    update_position(c);

    // Check if comment is continued, eg: consecutive C++ comments
    while (next && PTree::first(next) && PTree::first(next)->is_atom())
    {
      if (!strncmp(first->position() + first->length() - 2, "*/", 2))
        break;
      if (strncmp(PTree::first(next)->position(), "//", 2))
        break;
      char const *next_pos = PTree::first(next)->position();
      char const *start_pos = PTree::first(c)->position();
      char const *curr_pos = start_pos + PTree::first(c)->length();
      // Must only be whitespace between current comment and next
      // and only one newline
      int newlines = 0;
      while (curr_pos < next_pos && strchr(" \t\r\n", *curr_pos))
        if (*curr_pos == '\n' && newlines > 0)
          break;
        else if (*curr_pos++ == '\n')
          ++newlines;
      if (curr_pos < next_pos)
        break;
      // Current comment stretches to end of next
      int len = int(next_pos - start_pos + PTree::first(next)->length());
      c->set_car(first = new PTree::Atom(start_pos, len));
      // Skip the combined comment
      next = PTree::rest(next);
    }

    // all comments that are not immediately (i.e. separated
    // by a single new line) followed by a declaration are
    // marked as 'suspect'
    bool suspect = false;
    char const *pos = first->position() + first->length();
    while (*pos && strchr(" \t\r", *pos)) ++pos;
    if (*pos == '\n')
    {
      ++pos;
      // Found only allowed \n
      while (*pos && strchr(" \t\r", *pos)) ++pos;
      if (*pos == '\n' || !strncmp(pos, "/*", 2)) suspect = true;
    }

    AST::Comment comment = my_ast_kit.create_comment(my_file, my_lineno,
						     // FIXME: 'first' ought to be an atom,
						     //        so we could just take the position/length
						     PTree::reify(first),
						     suspect);
    comments.append(comment);

//     if (my_links) my_links->long_span(first, "file-comment");
    // Set first to 0 so we dont accidentally do them twice (eg:
    // when parsing expressions)
    c->set_car(0);
    c = next;
  }

  declarator.comments().extend(comments);
}

void ASTTranslator::update_position(PTree::Node *node)
{
  std::string filename;
  my_lineno = my_buffer->origin(node->begin(), filename);
  // TODO

//   if (filename != my_file.name())
//   {
//     my_file = my_filter->get_sourcefile(my_filename.c_str());
//     my_builder->set_file(my_file);
//   }  
}

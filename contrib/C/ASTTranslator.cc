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

void ASTTranslator::visit(PTree::Declarator *declarator)
{
  Trace trace("ASTTranslator::visit(PTree::Declarator *)");
  PTree::Encoding name = declarator->encoded_name();
  PTree::Encoding type = declarator->encoded_type();
  if (type.is_function())
  {
    AST::TypeList parameter_types;
    AST::Type return_type = my_types.lookup_function_types(type, parameter_types);
    AST::Function::Parameters parameters;
    PTree::Node *p = PTree::rest(declarator);
    while (p && p->car() && *p->car() != '(') p = PTree::rest(p);
    translate_parameters(PTree::second(p), parameter_types, parameters);

    size_t length = (name.front() - 0x80);
    AST::ScopedName qname(std::string(name.begin() + 1, name.begin() + 1 + length));
    AST::Modifiers modifiers;
    AST::Function function = my_ast_kit.create_function(my_file, my_lineno,
							"C",
							"function",
							modifiers,
							return_type,
							qname,
							qname.get(0));
    function.parameters().extend(parameters);
    add_comments(function, declarator->get_comments());
    my_ast.declarations().append(function);
  }
  else
  {
    AST::Type t = my_types.lookup(type);
    size_t length = (name.front() - 0x80);
    AST::ScopedName qname(std::string(name.begin() + 1, name.begin() + 1 + length));

    std::string vtype;// = my_builder->scope()->type();
    if (vtype == "class" || vtype == "struct" || vtype == "union")
      vtype = "data member";
    else
    {
      if (vtype == "function")
	vtype = "local";
      vtype += " variable";
    }
    AST::Variable variable = my_ast_kit.create_variable(my_file, my_lineno, "C",
							vtype, qname, t, false);
    add_comments(variable, declarator->get_comments());
    my_ast.declarations().append(variable);
  }
}

void ASTTranslator::visit(PTree::ClassSpec *class_spec)
{
  Trace trace("ASTTranslator::visit(PTree::ClassSpec *)");
  update_position(class_spec);
  size_t size = PTree::length(class_spec);
  if (size == 2) // forward declaration
  {
    // [ class|struct <name> ]

    std::string type = PTree::reify(PTree::first(class_spec));
    std::string name = PTree::reify(PTree::second(class_spec));
    AST::Forward forward = my_ast_kit.create_forward(my_file, my_lineno, "C",
						     type, AST::ScopedName(name));
    add_comments(forward, class_spec->get_comments());
    my_ast.declarations().append(forward);
    return;
  }  
  if (size == 4) // struct definition
  {
    // [ class|struct <name> <inheritance> [{ body }] ]

    std::string type = PTree::reify(PTree::first(class_spec));
    std::string name = PTree::reify(PTree::second(class_spec));
    PTree::nth(class_spec, 3)->accept(this);
  }
  else if (size == 3) // anonymous struct definition
  {
    // [ struct [nil nil] [{ ... }] ]

  }
}

void ASTTranslator::visit(PTree::EnumSpec *enum_spec)
{
  Trace trace("ASTTranslator::visit(PTree::EnumSpec *)");
  if (!PTree::second(enum_spec)) return; /* anonymous enum */
  std::string name = PTree::reify(PTree::second(enum_spec));
  update_position(enum_spec);

  AST::Enumerators enumerators;
  PTree::Node *enode = PTree::second(PTree::third(enum_spec));
  AST::Enumerator enumerator;
  while (enode)
  {
    update_position(enode);
    PTree::Node *penumor = PTree::first(enode);
    if (penumor->is_atom())
    {
      // Just a name
      AST::ScopedName qname(PTree::reify(penumor));
      enumerator = my_ast_kit.create_enumerator(my_file, my_lineno, "C",
						qname, "");
      add_comments(enumerator, static_cast<PTree::CommentedAtom *>(penumor)->get_comments());
    }
    else
    {
      // Name = Value
      AST::ScopedName qname(PTree::reify(PTree::first(penumor)));
      std::string value;
      if (PTree::length(penumor) == 3)
        value = PTree::reify(PTree::third(penumor));
      enumerator = my_ast_kit.create_enumerator(my_file, my_lineno, "C",
						qname, value);
      add_comments(enumerator, static_cast<PTree::CommentedAtom *>(penumor)->get_comments());
    }
    enumerators.append(enumerator);
    enode = PTree::rest(enode);
    // Skip comma
    if (enode && enode->car() && *enode->car() == ',') enode = PTree::rest(enode);
  }
  // Add a dummy enumerator at the end to absorb trailing comments.
  PTree::Node *close = PTree::third(PTree::third(enum_spec));
  enumerator = my_ast_kit.create_enumerator(my_file, my_lineno, "C",
					    AST::ScopedName(std::string("dummy")), "");
  add_comments(enumerator, static_cast<PTree::CommentedAtom *>(close));
  enumerators.append(enumerator);
  
  // Create AST.Enum object
  AST::Enum enum_ = my_ast_kit.create_enum(my_file, my_lineno, "C",
					   name, enumerators);
  add_comments(enum_, enum_spec);

  my_ast.declarations().append(enum_);
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
    my_ast.declarations().append(declaration);
  }
}

void ASTTranslator::translate_parameters(PTree::Node *node,
					 AST::TypeList types,
					 AST::Function::Parameters &parameters)
{
  Trace trace("ASTTranslator::translate_parameters");

  while (node)
  {
    // A parameter has a type, possibly a name and possibly a value.
    // Treat the value as a string, i.e. don't analyse the expression further.
    std::string name, value;
    AST::Modifiers premods, postmods;
    if (*node->car() == ',')
      node = node->cdr();
    PTree::Node *parameter = PTree::first(node);
    AST::Type type = types.get(0);
    types.del(0); // pop one value

    // Discover contents. Ptree may look like:
    //[register iostate [* a] = [0 + 2]]
    //[register iostate [nil] = 0]
    //[register iostate [nil]]
    //[iostate [nil] = 0]
    //[iostate [nil]]   etc
    if (PTree::length(parameter) > 1)
    {
      // There is a parameter
      int type_ix, value_ix = -1, len = PTree::length(parameter);
      if (len >= 4 && *PTree::nth(parameter, len-2) == '=')
      {
        // There is an =, which must be followed by the value
        value_ix = len-1;
        type_ix = len-4;
      }
      else
      {
        // No =, so last is name and second last is type
        type_ix = len-2;
      }
      // Skip keywords (eg: register) which are atoms
      for (int ix = 0; ix < type_ix && PTree::nth(parameter, ix)->is_atom(); ++ix)
      {
        PTree::Node *atom = PTree::nth(parameter, ix);
        premods.append(PTree::reify(atom));
      }
      // Find name
      if (PTree::Node *n = PTree::nth(parameter, type_ix+1))
      {
        if (PTree::last(n) && !PTree::last(n)->is_atom() && 
	    PTree::first(PTree::last(n)) &&
            *PTree::first(PTree::last(n)) == ')' && PTree::length(n) >= 4)
        {
          // Probably a function pointer type
          // pname is [* [( [* convert] )] ( [params] )]
          // set to [( [* convert] )] from example
          n = PTree::nth(n, PTree::length(n) - 4);
          if (n && !n->is_atom() && PTree::length(n) == 3)
          {
            // set to [* convert] from example
            n = PTree::second(n);
            if (n && PTree::second(n) && PTree::second(n)->is_atom())
              name = PTree::reify(PTree::second(n));
          }
        }
        else if (!n->is_atom() && PTree::last(n) && PTree::last(n)->car())
        {
          // * and & modifiers are stored with the name so we must skip them
          PTree::Node *last = PTree::last(n)->car();
          if (*last != '*' && *last != '&')
            // The last node is the name:
            name = PTree::reify(last);
        }
      }
      // Find value
      if (value_ix >= 0) value = PTree::reify(PTree::nth(parameter, value_ix));
    }
    AST::Parameter p = my_ast_kit.create_parameter(premods, type, postmods,
						   name, value);
    parameters.append(p);
    node = PTree::rest(node);
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

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
#include <Support/Path.hh>

using namespace Synopsis;

ASTTranslator::ASTTranslator(std::string const &filename,
			     std::string const &base_path, bool main_file_only,
			     Synopsis::AST::AST ast, bool v, bool d)
  : my_ast(ast),
    my_ast_kit("C"),
    my_lineno(0),
    my_raw_filename(filename),
    my_base_path(base_path),
    my_main_file_only(main_file_only),
    my_types(my_ast.types(), v, d),
    my_verbose(v), my_debug(d) 
{
  Trace trace("ASTTranslator::ASTTranslator");  

  // determine canonical filenames
  Path path = Path(my_raw_filename).abs();
  std::string long_filename = path.str();
  path.strip(my_base_path);
  std::string short_filename = path.str();

  AST::SourceFile file = my_ast.files().get(short_filename);
  if (file)
    my_file = file;
  else
  {
    my_file = my_ast_kit.create_source_file(short_filename, long_filename);
    my_ast.files().set(short_filename, my_file);
  }
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
  trace << declarator;
  if (!PTree::first(declarator)) return; // empty

  bool visible = update_position(declarator);
  PTree::Encoding name = declarator->encoded_name();
  PTree::Encoding type = declarator->encoded_type();
  if (type.is_function())
  {
    trace << "declare function " << name << " (" << type << ')' 
	  << my_raw_filename << ':' << my_lineno;

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
							"function",
							modifiers,
							return_type,
							qname,
							qname.get(0));
    function.parameters().extend(parameters);
    if (my_declaration) add_comments(function, my_declaration->get_comments());
    add_comments(function, declarator->get_comments());
    if (visible) declare(function);
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
	vtype = "local ";
      vtype += "variable";
    }
    AST::Variable variable = my_ast_kit.create_variable(my_file, my_lineno,
							vtype, qname, t, false);
    if (my_declaration) add_comments(variable, my_declaration->get_comments());
    add_comments(variable, declarator->get_comments());
    if (visible) declare(variable);
  }
}

void ASTTranslator::visit(PTree::Declaration *declaration)
{
  Trace trace("ASTTranslator::visit(PTree::Declaration *)");
  // Cache the declaration while traversing the individual declarators;
  // the comments are passed through.
  my_declaration = declaration;
  visit(static_cast<PTree::List *>(declaration));
  my_declaration = 0;
}

void ASTTranslator::visit(PTree::ClassSpec *class_spec)
{
  Trace trace("ASTTranslator::visit(PTree::ClassSpec *)");
  
  bool visible = update_position(class_spec);

  size_t size = PTree::length(class_spec);
  if (size == 2) // forward declaration
  {
    // [ class|struct <name> ]

    std::string type = PTree::reify(PTree::first(class_spec));
    std::string name = PTree::reify(PTree::second(class_spec));
    AST::ScopedName qname(name);
    AST::Forward forward = my_ast_kit.create_forward(my_file, my_lineno,
						     type, qname);
    add_comments(forward, class_spec->get_comments());
    if (visible) declare(forward);
    my_types.declare(qname, forward);
    return;
  }

  std::string type = PTree::reify(PTree::first(class_spec));
  std::string name;
  PTree::ClassBody *body = 0;

  if (size == 4) // struct definition
  {
    // [ class|struct <name> <inheritance> [{ body }] ]

    name = PTree::reify(PTree::second(class_spec));
    body = static_cast<PTree::ClassBody *>(PTree::nth(class_spec, 3));
  }
  else if (size == 3) // anonymous struct definition
  {
    // [ struct [nil nil] [{ ... }] ]

    PTree::Encoding ename = class_spec->encoded_name();
    size_t length = (ename.front() - 0x80);
    name = std::string(ename.begin() + 1, ename.begin() + 1 + length);
    body = static_cast<PTree::ClassBody *>(PTree::nth(class_spec, 2));
  }

  AST::ScopedName qname(name);
  AST::Class class_ = my_ast_kit.create_class(my_file, my_lineno, type, qname);
  add_comments(class_, class_spec->get_comments());
  if (visible) declare(class_);
  my_types.declare(qname, class_);
  my_scope.push(class_);
  body->accept(this);
  my_scope.pop();
}

void ASTTranslator::visit(PTree::EnumSpec *enum_spec)
{
  Trace trace("ASTTranslator::visit(PTree::EnumSpec *)");

  bool visible = update_position(enum_spec);
  std::string name;
  
  if (!PTree::second(enum_spec)) //anonymous
  {
    PTree::Encoding ename = enum_spec->encoded_name();
    size_t length = (ename.front() - 0x80);
    name = std::string(ename.begin() + 1, ename.begin() + 1 + length);
  }
  else
    name = PTree::reify(PTree::second(enum_spec));

  AST::Enumerators enumerators;
  PTree::Node *enode = PTree::second(PTree::third(enum_spec));
  AST::Enumerator enumerator;
  while (enode)
  {
    // quite a costly way to update the line number...
    update_position(enode);
    PTree::Node *penumor = PTree::first(enode);
    if (penumor->is_atom())
    {
      // Just a name
      AST::ScopedName qname(PTree::reify(penumor));
      enumerator = my_ast_kit.create_enumerator(my_file, my_lineno, qname, "");
      add_comments(enumerator, static_cast<PTree::CommentedAtom *>(penumor)->get_comments());
    }
    else
    {
      // Name = Value
      AST::ScopedName qname(PTree::reify(PTree::first(penumor)));
      std::string value;
      if (PTree::length(penumor) == 3)
        value = PTree::reify(PTree::third(penumor));
      enumerator = my_ast_kit.create_enumerator(my_file, my_lineno, qname, value);
      add_comments(enumerator, static_cast<PTree::CommentedAtom *>(penumor)->get_comments());
    }
    enumerators.append(enumerator);
    enode = PTree::rest(enode);
    // Skip comma
    if (enode && enode->car() && *enode->car() == ',') enode = PTree::rest(enode);
  }
  // Add a dummy enumerator at the end to absorb trailing comments.
  PTree::Node *close = PTree::third(PTree::third(enum_spec));
  enumerator = my_ast_kit.create_enumerator(my_file, my_lineno,
					    AST::ScopedName(std::string("dummy")), "");
  add_comments(enumerator, static_cast<PTree::CommentedAtom *>(close));
  enumerators.append(enumerator);
  
  // Create AST.Enum object
  AST::Enum enum_ = my_ast_kit.create_enum(my_file, my_lineno, name, enumerators);
  add_comments(enum_, enum_spec);

  if (visible) declare(enum_);
  my_types.declare(AST::ScopedName(name), enum_);
}

void ASTTranslator::visit(PTree::Typedef *typed)
{
  Trace trace("ASTTranslator::visit(PTree::Typedef *)");

  bool visible = update_position(typed);

  // the second child node may be an inlined class spec, i.e.
  // typedef struct {...} type;
  PTree::second(typed)->accept(this);

  for (PTree::Node *d = PTree::third(typed); d; d = PTree::tail(d, 2))
  {
    if(PTree::type_of(d->car()) != Token::ntDeclarator)  // is this check necessary
      continue;
    
    PTree::Declarator *declarator = static_cast<PTree::Declarator *>(d->car());
    PTree::Encoding name = declarator->encoded_name();
    PTree::Encoding type = declarator->encoded_type();
    trace << "declare type " << name << " (" << type << ')' 
	  << my_raw_filename << ':' << my_lineno;
    assert(name.is_simple_name());
    size_t length = (name.front() - 0x80);
    AST::ScopedName qname(std::string(name.begin() + 1, name.begin() + 1 + length));
    AST::Type alias = my_types.lookup(type);
    AST::Declaration declaration = my_ast_kit.create_typedef(my_file, my_lineno,
							     "typedef",
							     qname,
							     alias, false);
    add_comments(declaration, declarator->get_comments());
    if (visible) declare(declaration);
    my_types.declare(qname, declaration);
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

bool ASTTranslator::update_position(PTree::Node *node)
{
  Trace trace("ASTTranslator::update_position");

  std::string filename;
  my_lineno = my_buffer->origin(node->begin(), filename);

  if (filename != my_raw_filename)
  {
    if (my_main_file_only)
      // my_raw_filename remains the main file's name
      // and all declarations from elsewhere are ignored
      return false;
    my_raw_filename = filename;

    // determine canonical filenames
    Path path = Path(filename).abs();
    std::string long_filename = path.str();
    path.strip(my_base_path);
    std::string short_filename = path.str();

    AST::SourceFile file = my_ast.files().get(short_filename);
    if (file)
      my_file = file;
    else
    {
      my_file = my_ast_kit.create_source_file(short_filename, long_filename);
      my_ast.files().set(short_filename, my_file);
    }
  }
  return true;
}

// FIXME: AST should derive from Scope (a global scope IsA scope...)
//        This method exists because currently it is not.
void ASTTranslator::declare(AST::Declaration declaration)
{
  if (my_scope.size())
    my_scope.top().declarations().append(declaration);
  else
    my_ast.declarations().append(declaration);    
}

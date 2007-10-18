//
// Copyright (C) 2005 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#include "ASGTranslator.hh"
#include "TypeTranslator.hh"
#include <Synopsis/Trace.hh>
#include <Synopsis/PTree/Writer.hh> // for PTree::reify
#include <Support/Path.hh>

using namespace Synopsis;

ASGTranslator::ASGTranslator(std::string const &filename,
			     std::string const &base_path, bool primary_file_only,
			     Synopsis::IR ir, bool v, bool d)
  : my_ir(ir),
    my_ast_kit(),
    my_sf_kit("C"),
    my_raw_filename(filename),
    my_base_path(base_path),
    my_primary_file_only(primary_file_only),
    my_lineno(0),
    my_types(my_ir.types(), v, d),
    my_verbose(v),
    my_debug(d),
    my_buffer(0),
    my_declaration(0)
{
  Trace trace("ASGTranslator::ASGTranslator", Trace::TRANSLATION);
  // determine canonical filenames
  Path path = Path(my_raw_filename).abs();
  std::string long_filename = path.str();
  path.strip(my_base_path);
  std::string short_filename = path.str();

  SourceFile file = my_ir.files().get(short_filename);
  if (file)
    my_file = file;
  else
  {
    my_file = my_sf_kit.create_source_file(short_filename, long_filename);
    my_ir.files().set(short_filename, my_file);
  }
}

void ASGTranslator::translate(PTree::Node *ptree, Buffer &buffer)
{
  Trace trace("ASGTranslator::translate", Trace::TRANSLATION);
  my_buffer = &buffer;
  ptree->accept(this);
}

void ASGTranslator::visit(PTree::List *node)
{
  if (node->car()) node->car()->accept(this);
  if (node->cdr()) node->cdr()->accept(this);
}

void ASGTranslator::visit(PTree::Declarator *declarator)
{
  Trace trace("ASGTranslator::visit(PTree::Declarator *)", Trace::TRANSLATION);
  trace << declarator;
  if (!my_declaration || !PTree::first(declarator)) return; // empty

  bool visible = update_position(declarator);
  PTree::Encoding name = declarator->encoded_name();
  PTree::Encoding type = declarator->encoded_type();
  if (type.is_function())
  {
    trace << "declare function " << name << " (" << type << ')' 
	  << my_raw_filename << ':' << my_lineno;

    ASG::TypeList parameter_types;
    ASG::Type return_type = my_types.lookup_function_types(type, parameter_types);
    ASG::Function::Parameters parameters;
    PTree::Node *p = PTree::rest(declarator);
    while (p && p->car() && *p->car() != '(') p = PTree::rest(p);
    translate_parameters(PTree::second(p), parameter_types, parameters);

    size_t length = (name.front() - 0x80);
    ASG::ScopedName qname(std::string(name.begin() + 1, name.begin() + 1 + length));
    ASG::Modifiers pre;
    ASG::Modifiers post;
    ASG::Function function = my_ast_kit.create_function(my_file, my_lineno,
							"function",
							pre,
							return_type,
							post,
							qname,
							qname.get(0));
    function.parameters().extend(parameters);
    if (my_declaration) add_comments(function, my_declaration->get_comments());
    add_comments(function, declarator->get_comments());
    if (visible) declare(function);
  }
  else
  {
    ASG::Type t = my_types.lookup(type);
    size_t length = (name.front() - 0x80);
    ASG::ScopedName qname(std::string(name.begin() + 1, name.begin() + 1 + length));

    std::string vtype;// = my_builder->scope()->type();
    if (vtype == "class" || vtype == "struct" || vtype == "union")
      vtype = "data member";
    else
    {
      if (vtype == "function")
	vtype = "local ";
      vtype += "variable";
    }
    ASG::Variable variable = my_ast_kit.create_variable(my_file, my_lineno,
							vtype, qname, t, false);
    if (my_declaration) add_comments(variable, my_declaration->get_comments());
    add_comments(variable, declarator->get_comments());
    if (visible) declare(variable);
  }
}

void ASGTranslator::visit(PTree::Declaration *declaration)
{
  Trace trace("ASGTranslator::visit(PTree::Declaration *)", Trace::TRANSLATION);
  // Cache the declaration while traversing the individual declarators;
  // the comments are passed through.
  my_declaration = declaration;
  visit(static_cast<PTree::List *>(declaration));
  my_declaration = 0;
}

void ASGTranslator::visit(PTree::FunctionDefinition *def)
{
  Trace trace("ASGTranslator::visit(PTree::FunctionDefinition *)", Trace::TRANSLATION);
  // Cache the declaration while traversing the individual declarators;
  // the comments are passed through.
  my_declaration = def;
  // Only traverse declaration-specifier-seq and declarator, but not body
  if (PTree::first(def)) PTree::first(def)->accept(this);
  PTree::second(def)->accept(this);
  my_declaration = 0;
}

void ASGTranslator::visit(PTree::ClassSpec *class_spec)
{
  Trace trace("ASGTranslator::visit(PTree::ClassSpec *)", Trace::TRANSLATION);
  
  bool visible = update_position(class_spec);

  size_t size = PTree::length(class_spec);
  if (size == 2) // forward declaration
  {
    // [ class|struct <name> ]

    std::string type = PTree::reify(PTree::first(class_spec));
    std::string name = PTree::reify(PTree::second(class_spec));
    ASG::ScopedName qname(name);
    ASG::Forward forward = my_ast_kit.create_forward(my_file, my_lineno,
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

  ASG::ScopedName qname(name);
  ASG::Class class_ = my_ast_kit.create_class(my_file, my_lineno, type, qname);
  add_comments(class_, class_spec->get_comments());
  if (visible) declare(class_);
  my_types.declare(qname, class_);
  my_scope.push(class_);
  body->accept(this);
  my_scope.pop();
}

void ASGTranslator::visit(PTree::EnumSpec *enum_spec)
{
  Trace trace("ASGTranslator::visit(PTree::EnumSpec *)", Trace::TRANSLATION);

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

  ASG::Enumerators enumerators;
  PTree::Node *enode = PTree::second(PTree::third(enum_spec));
  ASG::Enumerator enumerator;
  while (enode)
  {
    // quite a costly way to update the line number...
    update_position(enode);
    PTree::Node *penumor = PTree::first(enode);
    if (penumor->is_atom())
    {
      // Just a name
      ASG::ScopedName qname(PTree::reify(penumor));
      enumerator = my_ast_kit.create_enumerator(my_file, my_lineno, qname, "");
      add_comments(enumerator, static_cast<PTree::CommentedAtom *>(penumor)->get_comments());
    }
    else
    {
      // Name = Value
      ASG::ScopedName qname(PTree::reify(PTree::first(penumor)));
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
					    ASG::ScopedName(std::string("dummy")), "");
  add_comments(enumerator, static_cast<PTree::CommentedAtom *>(close));
  enumerators.append(enumerator);
  
  // Create ASG.Enum object
  ASG::Enum enum_ = my_ast_kit.create_enum(my_file, my_lineno, name, enumerators);
  add_comments(enum_, enum_spec);

  if (visible) declare(enum_);
  my_types.declare(ASG::ScopedName(name), enum_);
}

void ASGTranslator::visit(PTree::Typedef *typed)
{
  Trace trace("ASGTranslator::visit(PTree::Typedef *)", Trace::TRANSLATION);

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
    ASG::ScopedName qname(std::string(name.begin() + 1, name.begin() + 1 + length));
    ASG::Type alias = my_types.lookup(type);
    // FIXME: need to honour constr parameter
    ASG::Declaration declaration = my_ast_kit.create_typedef(my_file, my_lineno,
							     "typedef",
							     qname,
							     alias, false);
    add_comments(declaration, declarator->get_comments());
    if (visible) declare(declaration);
    my_types.declare(qname, declaration);
  }
}

void ASGTranslator::translate_parameters(PTree::Node *node,
					 ASG::TypeList types,
					 ASG::Function::Parameters &parameters)
{
  Trace trace("ASGTranslator::translate_parameters", Trace::TRANSLATION);
  while (node)
  {
    // A parameter has a type, possibly a name and possibly a value.
    // Treat the value as a string, i.e. don't analyse the expression further.
    std::string name, value;
    ASG::Modifiers premods, postmods;
    if (*node->car() == ',')
      node = node->cdr();
    PTree::Node *parameter = PTree::first(node);
    ASG::Type type = types.get(0);
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
      for (int ix = 0;
	   ix < type_ix && 
	     PTree::nth(parameter, ix) &&
	     PTree::nth(parameter, ix)->is_atom();
	   ++ix)
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
    ASG::Parameter p = my_ast_kit.create_parameter(premods, type, postmods,
						   name, value);
    parameters.append(p);
    node = PTree::rest(node);
  }
}

void ASGTranslator::add_comments(ASG::Declaration declarator, PTree::Node *c)
{
  Trace trace("ASGTranslator::add_comments", Trace::TRANSLATION);
  if (!declarator || !c) return;
  
  Python::List comments;

  // Loop over all comments in the list
  // If the last comment is separated from the declaration by more than a single '\n',
  // we add a None to the list.
  bool suspect = false;
  for (PTree::Node *next = PTree::rest(c); c && !c->is_atom(); next = PTree::rest(c))
  {
    PTree::Node *first = PTree::first(c);
    if (!first || !first->is_atom())
    {
      c = next;
      continue;
    }
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
    suspect = false;
    char const *pos = first->position() + first->length();
    while (*pos && strchr(" \t\r", *pos)) ++pos;
    if (*pos == '\n')
    {
      ++pos;
      // Found only allowed \n
      while (*pos && strchr(" \t\r", *pos)) ++pos;
      if (*pos == '\n' || !strncmp(pos, "/*", 2)) suspect = true;
    }
    Python::Object comment = PTree::reify(first);
    comments.append(comment);

//     if (my_links) my_links->long_span(first, "file-comment");
    // Set first to 0 so we dont accidentally do them twice (eg:
    // when parsing expressions)
    c->set_car(0);
    c = next;
  }
  if (suspect) comments.append(Python::Object());
  declarator.annotations().set("comments", comments);
}

bool ASGTranslator::update_position(PTree::Node *node)
{
  Trace trace("ASGTranslator::update_position", Trace::TRANSLATION);

  std::string filename;
  my_lineno = my_buffer->origin(node->begin(), filename);

  if (filename != my_raw_filename)
  {
    if (my_primary_file_only)
      // my_raw_filename remains the primary file's name
      // and all declarations from elsewhere are ignored
      return false;
    my_raw_filename = filename;

    // determine canonical filenames
    Path path = Path(filename).abs();
    std::string long_filename = path.str();
    path.strip(my_base_path);
    std::string short_filename = path.str();

    SourceFile file = my_ir.files().get(short_filename);
    if (file)
      my_file = file;
    else
    {
      my_file = my_sf_kit.create_source_file(short_filename, long_filename);
      my_ir.files().set(short_filename, my_file);
    }
  }
  return true;
}

// FIXME: ASG should derive from Scope (a global scope IsA scope...)
//        This method exists because currently it is not.
void ASGTranslator::declare(ASG::Declaration declaration)
{
  if (my_scope.size())
    my_scope.top().declarations().append(declaration);
  else
    my_ir.declarations().append(declaration);    
}

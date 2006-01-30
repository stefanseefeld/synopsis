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

using Synopsis::Token;
using Synopsis::Trace;
namespace PT = Synopsis::PTree;

ASTTranslator::ASTTranslator(std::string const &filename,
                             std::string const &base_path, bool main_file_only,
                             Synopsis::AST::AST ast, bool v, bool d)
  : my_ast(ast),
    my_ast_kit("C"),
    my_raw_filename(filename),
    my_base_path(base_path),
    my_main_file_only(main_file_only),
    my_lineno(0),
    my_types(my_ast.types(), v, d),
    my_verbose(v), my_debug(d) 
{
  Trace trace("ASTTranslator::ASTTranslator", Trace::TRANSLATION);

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

void ASTTranslator::translate(PT::Node *ptree, Buffer &buffer)
{
  Trace trace("ASTTranslator::translate", Trace::TRANSLATION);
  my_buffer = &buffer;
  ptree->accept(this);
}

void ASTTranslator::visit(PT::List *node)
{
  if (node->car()) node->car()->accept(this);
  if (node->cdr()) node->cdr()->accept(this);
}

void ASTTranslator::visit(PT::Declarator *declarator)
{
  Trace trace("ASTTranslator::visit(PT::Declarator *)", Trace::TRANSLATION);
  trace << declarator;
  if (!PT::nth<0>(declarator)) return; // empty

  bool visible = update_position(declarator);
  PT::Encoding name = declarator->encoded_name();
  PT::Encoding type = declarator->encoded_type();
  if (type.is_function())
  {
    trace << "declare function " << name << " (" << type << ')' 
          << my_raw_filename << ':' << my_lineno;

    AST::TypeList parameter_types;
    AST::Type return_type = my_types.lookup_function_types(type, parameter_types);
    AST::Function::Parameters parameters;
    PT::List *p = declarator->cdr();
    while (p && p->car() && *p->car() != '(') p = p->cdr();

    for (PT::List *node = static_cast<PT::List *>(PT::nth<1>(p));
	 node->cdr();
	 node = PT::tail(node, 2))
    {
      node->car()->accept(this); // PT::ParameterDeclaration
      parameters.append(my_parameter);
    }
    size_t length = (name.front() - 0x80);
    AST::ScopedName qname(std::string(name.begin() + 1, name.begin() + 1 + length));
    AST::Function function = my_ast_kit.create_function(my_file, my_lineno,
                                                        "function",
                                                        AST::Modifiers(),
                                                        return_type,
							AST::Modifiers(),
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

void ASTTranslator::visit(PT::SimpleDeclaration *declaration)
{
  Trace trace("ASTTranslator::visit(SimpleDeclaration)", Trace::TRANSLATION);
  bool visible = update_position(declaration);
  // Check whether this is a typedef:
  PT::DeclSpec *spec = declaration->decl_specifier_seq();
  // the decl-specifier-seq may contain a class-specifier, i.e.
  // which we need to define first.
  if (spec) spec->accept(this);
  if (spec && spec->is_typedef())
  {
    for (PT::List *d = declaration->declarators(); d; d = PT::tail(d, 2))
    {
      PT::Declarator *declarator = static_cast<PT::Declarator *>(d->car());
      PT::Encoding name = declarator->encoded_name();
      PT::Encoding type = declarator->encoded_type();
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
  else
  {
    // Cache the declaration while traversing the individual declarators;
    // the comments are passed through.
    my_declaration = declaration;
    visit(static_cast<PT::List *>(declaration));
    my_declaration = 0;
  }
}

void ASTTranslator::visit(PT::ClassSpec *spec)
{
  Trace trace("ASTTranslator::visit(PT::ClassSpec *)", Trace::TRANSLATION);
  
  bool visible = update_position(spec);

  std::string key = PT::string(spec->key());
  AST::ScopedName qname;
  if (spec->name()) qname = PT::string(static_cast<PT::Atom *>(spec->name()));
  else // anonymous
  {
    PT::Encoding ename = spec->encoded_name();
    size_t length = (ename.front() - 0x80);
    qname = std::string(ename.begin() + 1, ename.begin() + 1 + length);
  }
  AST::Class class_ = my_ast_kit.create_class(my_file, my_lineno, key, qname);
  add_comments(class_, spec->get_comments());
  if (visible) declare(class_);
  my_types.declare(qname, class_);
  my_scope.push(class_);
  spec->body()->accept(this);
  my_scope.pop();
}

void ASTTranslator::visit(PT::EnumSpec *spec)
{
  Trace trace("ASTTranslator::visit(PT::EnumSpec *)", Trace::TRANSLATION);

  bool visible = update_position(spec);
  std::string name;
  
  if (!spec->name()) //anonymous
  {
    PT::Encoding ename = spec->encoded_name();
    size_t length = (ename.front() - 0x80);
    name = std::string(ename.begin() + 1, ename.begin() + 1 + length);
  }
  else name = PT::reify(spec->name());

  AST::Enumerators enumerators;
  PT::List *enode = spec->enumerators();
  AST::Enumerator enumerator;
  while (enode)
  {
    // quite a costly way to update the line number...
    update_position(enode);
    PT::Node *penumor = PT::nth<0>(enode);
    if (penumor->is_atom())
    {
      // identifier
      AST::ScopedName qname(PT::string(static_cast<PT::Atom *>(penumor)));
      enumerator = my_ast_kit.create_enumerator(my_file, my_lineno, qname, "");
      add_comments(enumerator, static_cast<PT::Identifier *>(penumor)->get_comments());
    }
    else
    {
      // identifier = constant-expression
      AST::ScopedName qname(PT::string(static_cast<PT::Atom *>(PT::nth<0>(static_cast<PT::List *>(penumor)))));
      // TBD: For now stringify the initializer expression.
      std::string value = PT::reify(PT::nth<2>(static_cast<PT::List *>(penumor)));
      enumerator = my_ast_kit.create_enumerator(my_file, my_lineno, qname, value);
      add_comments(enumerator, static_cast<PT::Identifier *>(penumor)->get_comments());
    }
    enumerators.append(enumerator);
    enode = PT::tail(enode, 0);
    // Skip comma
    if (enode && enode->car() && *enode->car() == ',') enode = PT::tail(enode, 0);
  }
  // Add a dummy enumerator at the end to absorb trailing comments.
//   PT::Node *close = PT::nth<2>(PT::nth<2>(spec));
//   enumerator = my_ast_kit.create_enumerator(my_file, my_lineno,
//                                             AST::ScopedName(std::string("dummy")), "");
//   add_comments(enumerator, static_cast<PT::CommentedAtom *>(close));
//   enumerators.append(enumerator);
  
  // Create AST.Enum object
  AST::Enum enum_ = my_ast_kit.create_enum(my_file, my_lineno, name, enumerators);
  add_comments(enum_, spec);

  if (visible) declare(enum_);
  my_types.declare(AST::ScopedName(name), enum_);
}

void ASTTranslator::visit(PTree::ParameterDeclaration *param)
{
  Trace trace("ASTTranslator::visit(ParameterDeclaration)", Trace::TRANSLATION);

  PT::DeclSpec *spec = param->decl_specifier_seq();
  // FIXME: extract modifiers and type from decl-specifier-seq
  AST::Modifiers pre;
  AST::Type type = my_types.lookup(spec->type());
  AST::Modifiers post;
  std::string name = param->declarator()->encoded_name().unmangled();
  std::string value;
  PT::Node *initializer = param->initializer();
  if (initializer) value = PT::reify(initializer);
  my_parameter = my_ast_kit.create_parameter(pre, type, post, name, value);
}

void ASTTranslator::add_comments(AST::Declaration declarator, PT::Node *c)
{
  Trace trace("ASTTranslator::add_comments", Trace::TRANSLATION);
  if (!declarator || !c) return;
  
  Python::List comments;

  // Loop over all comments in the list
//   for (PT::Node *next = c->cdr(); c && !c->is_atom(); next = c->cdr())
//   {
//     PT::Node *first = PT::nth<0>(c);
//     if (!first || !first->is_atom())
//     {
//       c = next;
//       continue;
//     }
//     // update position information for this comment
//     update_position(c);

//     // Check if comment is continued, eg: consecutive C++ comments
//     while (next && PT::nth<0>(next) && PT::nth<0>(next)->is_atom())
//     {
//       if (!strncmp(first->position() + first->length() - 2, "*/", 2))
//         break;
//       if (strncmp(PT::nth<0>(next)->position(), "//", 2))
//         break;
//       char const *next_pos = PT::nth<0>(next)->position();
//       char const *start_pos = PT::nth<0>(c)->position();
//       char const *curr_pos = start_pos + PT::nth<0>(c)->length();
//       // Must only be whitespace between current comment and next
//       // and only one newline
//       int newlines = 0;
//       while (curr_pos < next_pos && strchr(" \t\r\n", *curr_pos))
//         if (*curr_pos == '\n' && newlines > 0)
//           break;
//         else if (*curr_pos++ == '\n')
//           ++newlines;
//       if (curr_pos < next_pos)
//         break;
//       // Current comment stretches to end of next
//       int len = int(next_pos - start_pos + PT::nth<0>(next)->length());
//       c->set_car(first = new PT::Atom(start_pos, len));
//       // Skip the combined comment
//       next = next->cdr();
//     }

//     // all comments that are not immediately (i.e. separated
//     // by a single new line) followed by a declaration are
//     // marked as 'suspect'
//     bool suspect = false;
//     char const *pos = first->position() + first->length();
//     while (*pos && strchr(" \t\r", *pos)) ++pos;
//     if (*pos == '\n')
//     {
//       ++pos;
//       // Found only allowed \n
//       while (*pos && strchr(" \t\r", *pos)) ++pos;
//       if (*pos == '\n' || !strncmp(pos, "/*", 2)) suspect = true;
//     }
//     AST::Comment comment = my_ast_kit.create_comment(my_file, my_lineno,
//                                                      // FIXME: 'first' ought to be an atom,
//                                                      //        so we could just take the position/length
//                                                      PT::reify(first),
//                                                      suspect);
//     comments.append(comment);

// //     if (my_links) my_links->long_span(first, "file-comment");
//     // Set first to 0 so we dont accidentally do them twice (eg:
//     // when parsing expressions)
//     c->set_car(0);
//     c = next;
//   }

  declarator.comments().extend(comments);
}

bool ASTTranslator::update_position(PT::Node *node)
{
  Trace trace("ASTTranslator::update_position", Trace::TRANSLATION);

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

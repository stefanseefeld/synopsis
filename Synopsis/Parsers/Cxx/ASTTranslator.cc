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
#include <Synopsis/PTree/Display.hh> // for PTree::display (debugging...)
#include <Support/Path.hh>

using Synopsis::Token;
using Synopsis::Trace;
namespace PT = Synopsis::PTree;
namespace ST = Synopsis::SymbolTable;

ASTTranslator::TemplateParameterTranslator::TemplateParameterTranslator(ASTTranslator &parent)
  : my_parent(parent) {}

Python::List ASTTranslator::TemplateParameterTranslator::translate(PT::TemplateDecl *templ)
{
  for (PT::List *params = static_cast<PT::List *>(PT::nth<2>(templ));
       params;
       params = PT::tail(params, 2))
    params->car()->accept(this);      
  return my_parameters;
}

void ASTTranslator::TemplateParameterTranslator::visit(PT::TypeParameter *param)
{
  PT::Node *typename_ = PT::nth<0>(param);
  PT::Node *name = PT::nth<1>(param);
  AST::ScopedName qname(std::string(name->position(), name->length()));
  AST::Type type = my_parent.my_types.create_dependent(qname);
  AST::Modifiers pre(std::string(typename_->position(), typename_->length()));
  AST::Modifiers post;
  AST::Parameter parameter = my_parent.my_ast_kit.create_parameter(pre,
								   type,
								   post,
								   "", // name (unused)
								   ""); // default value
  my_parameters.append(parameter);
}

void ASTTranslator::TemplateParameterTranslator::visit(PT::ParameterDeclaration *param)
{
  PT::Encoding const &name = PT::nth<2>(param)->encoded_name();
  AST::Modifiers pre;
  // FIXME: AST.Parameter wants a type, even though this is a non-type.
  //        Is AST.Parameter really what we want here ?
  //        Wouldn't AST.Const be a better match ?
  AST::Type type;
  AST::Modifiers post;
  AST::Parameter parameter = my_parent.my_ast_kit.create_parameter(pre,
								   type,
								   post,
								   name.unmangled(), // name (unused)
								   ""); // default value
  my_parameters.append(parameter);
}

ASTTranslator::ASTTranslator(ST::Scope *scope,
			     std::string const &filename,
			     std::string const &base_path, bool main_file_only,
			     Synopsis::AST::AST ast, bool v, bool d)
  : Walker(scope),
    my_ast(ast),
    my_ast_kit("C++"),
    my_raw_filename(filename),
    my_base_path(base_path),
    my_main_file_only(main_file_only),
    my_lineno(0),
    my_types(my_ast.types(), v, d),
    my_in_class(false),
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

void ASTTranslator::visit(PT::NamespaceSpec *spec)
{
  Trace trace("ASTTranslator::visit(NamespaceSpec)", Trace::TRANSLATION);

  bool visible = update_position(spec);
  if (!visible) return;

  PTree::Node *identifier = PTree::nth<1>(spec);
  std::string name;
  if (identifier) name = std::string(identifier->position(), identifier->length());
  else // anonymous namespace
  {
    name = my_file.name();
    std::string::size_type p = name.rfind('/');
    if (p != std::string::npos) name.erase(0, p + 1);
    name = "{" + name + "}";
  }
  AST::ScopedName qname = name;
  AST::Module module = my_ast_kit.create_module(my_file, my_lineno,
						"namespace", qname);
  add_comments(module, spec->get_comments());
  declare(module);
  my_types.declare(qname, module);
  my_scope.push(module);
  traverse_body(spec);
  my_scope.pop();
}

void ASTTranslator::visit(PT::Declarator *declarator)
{
  Trace trace("ASTTranslator::visit(PT::Declarator *)", Trace::TRANSLATION);
  trace << declarator;
  if (!PT::nth<0>(declarator)) return; // empty

  bool visible = update_position(declarator);
  PT::Encoding name = declarator->encoded_name();
  // If the name is qualified, we have already seen a corresponding declaration.
  if (name.is_qualified()) return;
  PT::Encoding type = declarator->encoded_type();
  if (type.is_function())
  {
    trace << "declare function " << name << " (" << type << ')' 
	  << my_raw_filename << ':' << my_lineno;

    AST::TypeList parameter_types;
    AST::Type return_type = my_types.lookup_function_types(type, parameter_types);
    AST::Function::Parameters parameters;
    PT::List *p = PT::tail(declarator, 0);
    while (p && p->car() && *p->car() != '(') p = PT::tail(p, 0);
    translate_parameters(static_cast<PT::List *>(PT::nth<1>(p)),
			 parameter_types, parameters);
    size_t length = (name.front() - 0x80);
    AST::ScopedName qname(std::string(name.begin() + 1, name.begin() + 1 + length));
    AST::Modifiers modifiers;
    AST::Function function;
    if (my_in_class)
      function = my_ast_kit.create_operation(my_file, my_lineno,
					     "member function",
					     modifiers,
					     return_type,
					     qname,
					     name.unmangled());
    else
      function = my_ast_kit.create_function(my_file, my_lineno,
					    "function",
					    modifiers,
					    return_type,
					    qname,
					    name.unmangled());
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

    std::string vtype = my_scope.size() ? my_scope.top().type() : "global variable";
    AST::Variable variable = my_ast_kit.create_variable(my_file, my_lineno,
							vtype, qname, t, false);
    if (my_declaration) add_comments(variable, my_declaration->get_comments());
    add_comments(variable, declarator->get_comments());
    if (visible) declare(variable);
  }
}

void ASTTranslator::visit(PT::Declaration *declaration)
{
  Trace trace("ASTTranslator::visit(PT::Declaration *)", Trace::TRANSLATION);
  // Cache the declaration while traversing the individual declarators;
  // the comments are passed through.
  my_declaration = declaration;
  Walker::visit(declaration);
  my_declaration = 0;
}

void ASTTranslator::visit(PTree::FunctionDefinition *fdef)
{
  Trace trace("ASTTranslator::visit(PT::FunctionDefinition *)", Trace::TRANSLATION);
  my_declaration = fdef;
  PT::Node *decl = PT::nth<2>(fdef);
  visit(static_cast<PT::Declarator *>(decl)); // visit the declarator
  my_declaration = 0;
}

void ASTTranslator::visit(PT::ClassSpec *spec)
{
  Trace trace("ASTTranslator::visit(PT::ClassSpec *)", Trace::TRANSLATION);
  
  bool visible = update_position(spec);
  if (!visible) return;

  size_t size = PT::length(spec);
  if (size == 2) // forward declaration
  {
    std::string type = PT::reify(PT::nth<0>(spec));
    std::string name = PT::reify(PT::nth<1>(spec));
    AST::ScopedName qname(name);
    AST::Forward forward = my_ast_kit.create_forward(my_file, my_lineno,
						     type, qname);
    add_comments(forward, spec->get_comments());
    if (visible) declare(forward);
    my_types.declare(qname, forward);
    return;
  }

  std::string type = PT::reify(PT::nth<0>(spec));
  std::string name;

  if (size == 4) // struct definition
    name = PT::reify(PT::nth<1>(spec));
  else if (size == 3) // anonymous struct definition
  {
    PT::Encoding ename = spec->encoded_name();
    size_t length = (ename.front() - 0x80);
    name = std::string(ename.begin() + 1, ename.begin() + 1 + length);
  }

  AST::ScopedName qname(name);
  AST::Class class_ = my_ast_kit.create_class(my_file, my_lineno, type, qname);
  add_comments(class_, spec->get_comments());
  if (visible) declare(class_);
  if (my_template_parameters)
  {
    AST::Type type = my_types.declare(qname, class_, my_template_parameters);
    class_.set_template(type);
  }
  else
    my_types.declare(qname, class_);
  my_scope.push(class_);
  my_in_class = true;
  Walker::traverse_body(spec);
  my_in_class = false;
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
// 					    AST::ScopedName(std::string("dummy")), "");
//   add_comments(enumerator, static_cast<PT::CommentedAtom *>(close));
//   enumerators.append(enumerator);
  
  // Create AST.Enum object
  AST::Enum enum_ = my_ast_kit.create_enum(my_file, my_lineno, name, enumerators);
  add_comments(enum_, spec);

  if (visible) declare(enum_);
  my_types.declare(AST::ScopedName(name), enum_);
}

void ASTTranslator::visit(PT::Typedef *typed)
{
  Trace trace("ASTTranslator::visit(PT::Typedef *)", Trace::TRANSLATION);

  bool visible = update_position(typed);

  // the second child node may be an inlined class spec, i.e.
  // typedef struct {...} type;
  PT::nth<1>(typed)->accept(this);

  for (PT::List *d = static_cast<PT::List *>(PT::nth<1>(typed));
       d;
       d = PT::tail(d, 2))
  {
    if(PT::type_of(d->car()) != Token::ntDeclarator)  // is this check necessary
      continue;
    
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

void ASTTranslator::visit(PTree::TemplateDecl *templ)
{
  Trace trace("ASTTranslator::visit(PT::TemplateDecl *)", Trace::TRANSLATION);
  TemplateParameterTranslator translator(*this);
  my_template_parameters = translator.translate(templ);
  PT::nth(templ, 4)->accept(this);
  // Reset, to indicate we are not inside a template declaration.
  my_template_parameters = AST::Template::Parameters();
}

void ASTTranslator::translate_parameters(PT::List *node,
					 AST::TypeList types,
					 AST::Function::Parameters &parameters)
{
  Trace trace("ASTTranslator::translate_parameters", Trace::TRANSLATION);

  while (node)
  {
    // A parameter has a type, possibly a name and possibly a value.
    // Treat the value as a string, i.e. don't analyse the expression further.
    std::string name, value;
    AST::Modifiers premods, postmods;
    if (*node->car() == ',') node = node->cdr();
    PT::List *param = static_cast<PT::List *>(PT::nth<0>(node));
    AST::Type type = types.get(0);
    types.del(0); // pop one value

    // Discover contents. Ptree may look like:
    //[register iostate [* a] = [0 + 2]]
    //[register iostate [nil] = 0]
    //[register iostate [nil]]
    //[iostate [nil] = 0]
    //[iostate [nil]]   etc
    if (PT::length(param) > 1)
    {
      // There is a parameter
      int type_ix, value_ix = -1, len = PT::length(param);
      if (len >= 4 && *PT::nth(param, len-2) == '=')
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
	   ix < type_ix && PT::nth(param, ix) && PT::nth(param, ix)->is_atom();
	   ++ix)
      {
        PT::Node *atom = PT::nth(param, ix);
        premods.append(PT::reify(atom));
      }
      // Find name
//       if (PT::Node *n = PT::nth(param, type_ix+1))
//       {
//         if (PT::last(n) && !PT::last(n)->is_atom() && 
// 	    PT::nth<0>(PT::last(n)) &&
//             *PT::nth<0>(PT::last(n)) == ')' && PT::length(n) >= 4)
//         {
//           // Probably a function pointer type
//           // pname is [* [( [* convert] )] ( [params] )]
//           // set to [( [* convert] )] from example
//           n = PT::nth(n, PT::length(n) - 4);
//           if (n && !n->is_atom() && PT::length(n) == 3)
//           {
//             // set to [* convert] from example
//             n = PT::nth<1>(n);
//             if (n && PT::nth<1>(n) && PT::nth<1>(n)->is_atom())
//               name = PT::reify(PT::nth<1>(n));
//           }
//         }
//         else if (!n->is_atom() && PT::last(n) && PT::last(n)->car())
//         {
//           // * and & modifiers are stored with the name so we must skip them
//           PT::Node *last = PT::last(n)->car();
//           if (*last != '*' && *last != '&')
//             // The last node is the name:
//             name = PT::reify(last);
//         }
//       }
      // Find value
      if (value_ix >= 0) value = PT::reify(PT::nth(param, value_ix));
    }
    AST::Parameter p = my_ast_kit.create_parameter(premods, type, postmods,
						   name, value);
    parameters.append(p);
    node = PT::tail(node, 0);
  }
}

void ASTTranslator::add_comments(AST::Declaration declarator, PT::Node *c)
{
  Trace trace("ASTTranslator::add_comments", Trace::TRANSLATION);
  if (!declarator || !c) return;
  
  Python::List comments;

  // Loop over all comments in the list
//   for (PT::Node *next = PT::tail(c, 0); c && !c->is_atom(); next = PT::tail(c, 0))
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
//     while (next && PT::first(next) && PT::first(next)->is_atom())
//     {
//       if (!strncmp(first->position() + first->length() - 2, "*/", 2))
//         break;
//       if (strncmp(PT::first(next)->position(), "//", 2))
//         break;
//       char const *next_pos = PT::first(next)->position();
//       char const *start_pos = PT::first(c)->position();
//       char const *curr_pos = start_pos + PT::first(c)->length();
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
//       int len = int(next_pos - start_pos + PT::first(next)->length());
//       c->set_car(first = new PT::Atom(start_pos, len));
//       // Skip the combined comment
//       next = PT::tail(next, 0);
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
// 						     // FIXME: 'first' ought to be an atom,
// 						     //        so we could just take the position/length
// 						     PT::reify(first),
// 						     suspect);
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

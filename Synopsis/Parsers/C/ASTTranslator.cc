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
#include <Support/path.hh>

using Synopsis::Token;
using Synopsis::Trace;
namespace PT = Synopsis::PTree;

ASTTranslator::ASTTranslator(std::string const &filename,
			     std::string const &base_path, bool primary_file_only,
			     Synopsis::AST::AST ast, bool v, bool d)
  : ast_(ast),
    ast_kit_(),
    sf_kit_("C"),
    raw_filename_(filename),
    base_path_(base_path),
    primary_file_only_(primary_file_only),
    lineno_(0),
    types_(ast_.types(), v, d),
    verbose_(v),
    debug_(d) 
{
  Trace trace("ASTTranslator::ASTTranslator", Trace::TRANSLATION);
  // determine canonical filenames
  std::string long_filename = make_full_path(raw_filename_);
  std::string short_filename = make_short_path(raw_filename_, base_path_);

  AST::SourceFile file = ast_.files().get(short_filename);
  if (file)
    file_ = file;
  else
  {
    file_ = sf_kit_.create_source_file(short_filename, long_filename);
    ast_.files().set(short_filename, file_);
  }
}

void ASTTranslator::translate(PT::Node *ptree, Buffer &buffer)
{
  Trace trace("ASTTranslator::translate", Trace::TRANSLATION);
  buffer_ = &buffer;
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
	  << raw_filename_ << ':' << lineno_;

    AST::TypeList parameter_types;
    AST::Type return_type = types_.lookup_function_types(type, parameter_types);
    AST::Function::Parameters parameters;
    PT::List *p = declarator->cdr();
    while (p && p->car() && *p->car() != '(') p = p->cdr();

    for (PT::List *node = static_cast<PT::List *>(PT::nth<1>(p));
	 node->cdr();
	 node = PT::tail(node, 2))
    {
      node->car()->accept(this); // PT::ParameterDeclaration
      parameters.append(parameter_);
    }
    size_t length = (name.front() - 0x80);
    AST::ScopedName qname(std::string(name.begin() + 1, name.begin() + 1 + length));
    AST::Modifiers pre;
    AST::Modifiers post;
    AST::Function function = ast_kit_.create_function(file_, lineno_,
						      "function",
						      pre,
						      return_type,
						      post,
						      qname,
						      qname.get(0));
    function.parameters().extend(parameters);
    if (declaration_) add_comments(function, declaration_->get_comments());
    add_comments(function, declarator->get_comments());
    if (visible) declare(function);
  }
  else
  {
    AST::Type t = types_.lookup(type);
    size_t length = (name.front() - 0x80);
    AST::ScopedName qname(std::string(name.begin() + 1, name.begin() + 1 + length));

    std::string vtype;// = builder_->scope()->type();
    if (vtype == "class" || vtype == "struct" || vtype == "union")
      vtype = "data member";
    else
    {
      if (vtype == "function")
	vtype = "local ";
      vtype += "variable";
    }
    AST::Variable variable = ast_kit_.create_variable(file_, lineno_,
						      vtype, qname, t, false);
    if (declaration_) add_comments(variable, declaration_->get_comments());
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
	    << raw_filename_ << ':' << lineno_;
      assert(name.is_simple_name());
      size_t length = (name.front() - 0x80);
      AST::ScopedName qname(std::string(name.begin() + 1, name.begin() + 1 + length));
      AST::Type alias = types_.lookup(type);
      AST::Declaration declaration = ast_kit_.create_typedef(file_, lineno_,
							     "typedef",
							     qname,
							     alias, false);
      add_comments(declaration, declarator->get_comments());
      if (visible) declare(declaration);
      types_.declare(qname, declaration);
    }
  }
  else
  {
    // Cache the declaration while traversing the individual declarators;
    // the comments are passed through.
    declaration_ = declaration;
    visit(static_cast<PT::List *>(declaration));
    declaration_ = 0;
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
  AST::Class class_ = ast_kit_.create_class(file_, lineno_, key, qname);
  add_comments(class_, spec->get_comments());
  if (visible) declare(class_);
  types_.declare(qname, class_);
  scope_.push(class_);
  spec->body()->accept(this);
  scope_.pop();
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
      enumerator = ast_kit_.create_enumerator(file_, lineno_, qname, "");
      add_comments(enumerator, static_cast<PT::Identifier *>(penumor)->get_comments());
    }
    else
    {
      // identifier = constant-expression
      AST::ScopedName qname(PT::string(static_cast<PT::Atom *>(PT::nth<0>(static_cast<PT::List *>(penumor)))));
      // TBD: For now stringify the initializer expression.
      std::string value = PT::reify(PT::nth<2>(static_cast<PT::List *>(penumor)));
      enumerator = ast_kit_.create_enumerator(file_, lineno_, qname, value);
      add_comments(enumerator, static_cast<PT::Identifier *>(penumor)->get_comments());
    }
    enumerators.append(enumerator);
    enode = PT::tail(enode, 2);
    // Skip comma
    if (enode && enode->car() && *enode->car() == ',') enode = PT::tail(enode, 0);
  }
  // Create AST.Enum object
  AST::Enum enum_ = ast_kit_.create_enum(file_, lineno_, name, enumerators);
  add_comments(enum_, spec);

  if (visible) declare(enum_);
  types_.declare(AST::ScopedName(name), enum_);
}

void ASTTranslator::visit(PTree::ParameterDeclaration *param)
{
  Trace trace("ASTTranslator::visit(ParameterDeclaration)", Trace::TRANSLATION);

  PT::DeclSpec *spec = param->decl_specifier_seq();
  // FIXME: extract modifiers and type from decl-specifier-seq
  AST::Modifiers pre;
  AST::Type type = types_.lookup(spec->type());
  AST::Modifiers post;
  std::string name = param->declarator()->encoded_name().unmangled();
  std::string value;
  PT::Node *initializer = param->initializer();
  if (initializer) value = PT::reify(initializer);
  parameter_ = ast_kit_.create_parameter(pre, type, post, name, value);
}

void ASTTranslator::add_comments(AST::Declaration declarator, PT::Node *c)
{
  Trace trace("ASTTranslator::add_comments", Trace::TRANSLATION);
  if (!declarator || !c) return;
}

bool ASTTranslator::update_position(PT::Node *node)
{
  Trace trace("ASTTranslator::update_position", Trace::TRANSLATION);

  std::string filename;
  lineno_ = buffer_->origin(node->begin(), filename);

  if (filename != raw_filename_)
  {
    if (primary_file_only_)
      // raw_filename_ remains the primary file's name
      // and all declarations from elsewhere are ignored
      return false;
    raw_filename_ = filename;

    // determine canonical filenames
    std::string long_filename = make_full_path(filename);
    std::string short_filename = make_short_path(filename, base_path_);

    AST::SourceFile file = ast_.files().get(short_filename);
    if (file)
      file_ = file;
    else
    {
      file_ = sf_kit_.create_source_file(short_filename, long_filename);
      ast_.files().set(short_filename, file_);
    }
  }
  return true;
}

// FIXME: AST should derive from Scope (a global scope IsA scope...)
//        This method exists because currently it is not.
void ASTTranslator::declare(AST::Declaration declaration)
{
  if (scope_.size())
    scope_.top().declarations().append(declaration);
  else
    ast_.declarations().append(declaration);    
}

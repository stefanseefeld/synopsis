//
// Copyright (C) 2000 Stefan Seefeld
// Copyright (C) 2000 Stephen Davies
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#include <iostream>
#include <string>
#include <typeinfo>

#include <Synopsis/PTree.hh>
#include <Synopsis/Parser.hh>
#undef Scope

#include "linkstore.hh"
#include "swalker.hh"
#include "builder.hh"
#include "decoder.hh"
#include "lookup.hh"
#include "dumper.hh"
#include "strace.hh"
#include "type.hh"
#include "ast.hh"

using namespace Synopsis;

void SWalker::visit(PTree::ReturnStatement *node)
{
  STrace trace("SWalker::visit(PTree::ReturnStatement*)");
  if (!my_links) return;

  // Link 'return' keyword
  my_links->span(PTree::first(node), "file-keyword");

  // Translate the body of the return, if present
  if (PTree::length(node) == 3) translate(PTree::second(node));
}

void SWalker::visit(PTree::InfixExpr *node)
{
  STrace trace("SWalker::visit(PTree::Infix*)");
  translate(PTree::first(node));
  Types::Type* left_type = my_type;
  translate(PTree::third(node));
  Types::Type* right_type = my_type;
  std::string oper = parse_name(PTree::second(node));
  TypeFormatter tf;
  LOG("BINARY-OPER: " << tf.format(left_type) << " " << oper << " " << tf.format(right_type));
  nodeLOG(node);
  if (!left_type || !right_type)
  {
    my_type = 0;
    return;
  }
  // Lookup an appropriate operator
  AST::Function* func = my_lookup->lookupOperator(oper, left_type, right_type);
  if (func)
  {
    my_type = func->return_type();
    if (my_links) my_links->link(PTree::second(node), func->declared());
  }
  return;
}

void SWalker::translate_variable(PTree::Node *spec)
{
  // REVISIT: Figure out why this is so long!!!
  STrace trace("SWalker::TranslateVariable");
  if (my_links) find_comments(spec);
  try
  {
    PTree::Node *name_spec = spec;
    Types::Named* type;
    ScopedName scoped_name;
    if (!spec->is_atom())
    {
      // Must be a scoped name.. iterate through the scopes
      // stop when spec is at the last name
      //nodeLOG(spec);
      // If first node is '::' then reset my_scope to the global scope
      if (*PTree::first(spec) == "::")
      {
        scoped_name.push_back("");
        spec = PTree::rest(spec);
      }
      while (PTree::length(spec) > 2)
      {
        scoped_name.push_back(parse_name(PTree::first(spec)));
        /*
          if (!type) { throw nodeERROR(spec, "scope '" << parse_name(spec->First()) << "' not found!"); }
          try { my_scope = Types::declared_cast<AST::Scope>(type); }
          catch (const Types::wrong_type_cast&) { throw nodeERROR(spec, "scope '"<<parse_name(spec->First())<<"' found but not a scope!"); }
          // Link the scope name
          if (my_links) my_links->link(spec->First(), my_scope->declared());
        */
        spec = PTree::rest(PTree::rest(spec));
      }
      spec = PTree::first(spec);
      // Check for 'operator >>' type syntax:
      if (!spec->is_atom() && PTree::length(spec) == 2 && *PTree::first(spec) == "operator")
      {
        // Name lookup is done based on only the operator type, so
        // skip the 'operator' node
        spec = PTree::second(spec);
      }
      scoped_name.push_back(parse_name(spec));
    }
    std::string name = parse_name(spec);
    if (my_postfix_flag == Postfix_Var)
    {
      // Variable lookup. my_type will be the vtype
      /*cout << "my_scope is " << (my_scope ? my_type_formatter->format(my_scope->declared()) : "global") << endl;*/
      if (!scoped_name.empty())
        type = my_lookup->lookupType(scoped_name, true, my_scope);
      else if (my_scope)
        type = my_lookup->lookupType(name, my_scope);
      else
        type = my_lookup->lookupType(name);
      if (!type)
      {
        throw nodeERROR(spec, "variable '" << name << "' not found!");
      }
      // Now find vtype (throw wrong_type_cast if not a variable)
      try
      {
        Types::Declared& declared = dynamic_cast<Types::Declared&>(*type);
        // The variable could be a Variable or Enumerator
        AST::Variable* var;
        AST::Enumerator* enumor;
        if ((var = dynamic_cast<AST::Variable*>(declared.declaration())) != 0)
        {
          // It is a variable
          my_type = var->vtype();
          // Store a link to the variable itself (not its type)
          if (my_links) my_links->link(name_spec, type);
          /*cout << "var type name is " << my_type_formatter->format(my_type) << endl;*/
        }
        else if ((enumor = dynamic_cast<AST::Enumerator*>(declared.declaration())) != 0)
        {
          // It is an enumerator
          my_type = 0; // we have no use for enums in type code
          // But still a link is needed
          if (my_links) my_links->link(name_spec, type);
          /*cout << "enum type name is " << my_type_formatter->format(type) << endl;*/
        }
        else
        {
          throw nodeERROR(name_spec, "var was not a Variable nor Enumerator!");
        }
      }
      catch (const std::bad_cast &)
      {
        if (dynamic_cast<Types::Unknown*>(type))
          throw nodeERROR(spec, "variable '" << name << "' was an Unknown type!");
        if (dynamic_cast<Types::Base*>(type))
          throw nodeERROR(spec, "variable '" << name << "' was a Base type!");
        throw nodeERROR(spec, "variable '" << name << "' wasn't a declared type!");
      }
    }
    else
    {
      // Function lookup. my_type will be returnType. params are in my_params
      AST::Scope* scope = my_scope ;
      if (!scope) scope = my_builder->scope();
      // if (!scoped_name.empty()) func = my_lookup->lookupFunc(scoped_name, scope, my_params);
      AST::Function* func = my_lookup->lookupFunc(name, scope, my_params);
      if (!func)
      {
        throw nodeERROR(name_spec, "Warning: function '" << name << "' not found!");
      }
      // Store a link to the function name
      if (my_links)
        my_links->link(name_spec, func->declared(), LinkStore::FunctionCall);
      // Now find returnType
      my_type = func->return_type();
    }
  }
  catch(const TranslateError& e)
  {
    my_scope = 0;
    my_type = 0;
    e.set_node(spec);
    throw;
  }
  catch(const Types::wrong_type_cast &)
  {
    throw nodeERROR(spec, "wrong type error in TranslateVariable!");
  }
  catch(...)
  {
    throw nodeERROR(spec, "unknown error in TranslateVariable!");
  }
  my_scope = 0;
}

void SWalker::translate_function_args(PTree::Node *args)
{
  // args: [ arg (, arg)* ]
  while (PTree::length(args))
  {
    PTree::Node *arg = PTree::first(args);
    // Translate this arg, TODO: my_params would be better as a vector<Type*>
    my_type = 0;
    translate(arg);
    my_params.push_back(my_type);
    // Skip over arg and comma
    args = PTree::rest(PTree::rest(args));
  }
}

void SWalker::visit(PTree::FuncallExpr *node) 	// and fstyle cast
{
  STrace trace("SWalker::visit(PTree::FuncallExpr*)");
  // TODO: figure out how to deal with fstyle casts.. does it only apply to
  // base types? eg: int(4.0) ?
  // This is similar to TranslateVariable, except we have to check params..
  // doh! That means more my_type nastiness
  //
  LOG(node);
  // In translating the postfix the last var should be looked up as a
  // function. This means we have to find the args first, and store them in
  // my_params as a hint
  Types::Type::vector save_params = my_params;
  my_params.clear();
  try
  {
    translate_function_args(PTree::third(node));
  }
  catch (...)
  {
    // Restore params before rethrowing exception
    my_params = save_params;
    throw;
  }

  Postfix_Flag save_flag = my_postfix_flag;
  try
  {
    my_postfix_flag = Postfix_Func;
    translate(PTree::first(node));
  }
  catch (...)
  {
    // Restore params and flag before rethrowing exception
    my_params = save_params;
    my_postfix_flag = save_flag;
    throw;
  }

  // Restore my_params since we're done with it now
  my_params = save_params;
  my_postfix_flag = save_flag;
}

void SWalker::visit(PTree::ExprStatement *node)
{
  STrace trace("SWalker::visit(ExprStatement*)");
  translate(PTree::first(node));
}

void SWalker::visit(PTree::UnaryExpr *node)
{
  STrace trace("SWalker::visit(UnaryExpr*)");
  if (my_links) find_comments(node);
  // TODO: lookup unary operator
  translate(PTree::second(node));
}

void SWalker::visit(PTree::AssignExpr *node)
{
  STrace trace("SWalker::visit(AssignExpr*)");
  // TODO: lookup = operator
  my_type = 0;
  translate(PTree::first(node));
  Types::Type* ret_type = my_type;
  translate(PTree::third(node));
  my_type = ret_type;
}

//. Resolves the final type of any given Type. For example, it traverses
//. typedefs and parameterized types, and resolves unknowns by looking them up.
class TypeResolver : public Types::Visitor
{
  // TODO: Move to separate file???
public:
  //. Constructor - needs a Builder to resolve unknowns with
  TypeResolver(Builder* b) { my_builder = b;}
  
  //. Resolves the given type object
  Types::Type* resolve(Types::Type* t)
  {
    my_type = t;
    t->accept(this);
    return my_type;
  }

  //. Tries to resolve the given type object to a Scope
  AST::Scope* scope(Types::Type* t) throw (Types::wrong_type_cast, TranslateError)
  {
    return Types::declared_cast<AST::Scope>(resolve(t));
  }

  //. Looks up the unknown type for a fresh definition
  void visit_unknown(Types::Unknown* t)
  {
    my_type = my_builder->lookup()->resolveType(t);
    if (!dynamic_cast<Types::Unknown*>(my_type)) my_type->accept(this);
  }
  
  //. Recursively processes the aliased type
  void visit_modifier(Types::Modifier* t) { t->alias()->accept(this);}

  //. Checks for typedefs and recursively processes them
  void visit_declared(Types::Declared* t)
  {
    AST::Typedef* tdef = dynamic_cast<AST::Typedef*>(t->declaration());
    if (tdef) tdef->alias()->accept(this);
    else my_type = t;
  }

  //. Processes the template type
  void visit_parameterized(Types::Parameterized* t)
  {
    if (t->template_type()) t->template_type()->accept(this);
  }

protected:
  Builder* my_builder; //.< A reference to the builder object
  Types::Type* my_type; //.< The type to return
};

void SWalker::visit(PTree::ArrowMemberExpr *node)
{
  STrace trace("SWalker::visit(ArrowMember*)");
  my_type = 0;
  my_scope = 0;
  Postfix_Flag save_flag = my_postfix_flag;
  my_postfix_flag = Postfix_Var;
  translate(PTree::first(node));
  my_postfix_flag = save_flag;
  // my_type should be a modifier to a declared to a class. Throw bad_cast if not
  if (!my_type)
  {
    throw nodeERROR(node, "Unable to resolve type of LHS of ->");
  }
  try
  {
    my_scope = TypeResolver(my_builder).scope(my_type);
  }
  catch (const Types::wrong_type_cast&)
  {
    throw nodeERROR(node, "LHS of -> was not a scope!");
  }
  // Find member, my_type becomes the var type or func returnType
  translate(PTree::third(node));
  my_scope = 0;
}

void SWalker::visit(PTree::DotMemberExpr *node)
{
  STrace trace("SWalker::visit(DotMember*)");
  my_type = 0;
  my_scope = 0;
  Postfix_Flag save_flag = my_postfix_flag;
  my_postfix_flag = Postfix_Var;
  translate(PTree::first(node));
  my_postfix_flag = save_flag;
  LOG(parse_name(PTree::first(node)) << " resolved to " << my_type_formatter->format(my_type));
  // my_type should be a declared to a class
  if (!my_type)
  {
    throw nodeERROR(node, "Unable to resolve type of LHS of .");
  }
  LOG("resolving type to scope");
  // Check for reference type
  try
  {
    my_scope = TypeResolver(my_builder).scope(my_type);
  }
  catch (const Types::wrong_type_cast &)
  {
    throw nodeERROR(node, "Warning: LHS of . was not a scope: " << my_type_formatter->format(my_type));
  }
  // Find member, my_type becomes the var type or func returnType
  LOG("translating third");
  translate(PTree::third(node));
  my_scope = 0;
}

void SWalker::visit(PTree::IfStatement *node)
{
  STrace trace("SWalker::visit(IfStatement*)");
  if (my_links) find_comments(node);
  if (my_links) my_links->span(PTree::first(node), "file-keyword");
  // Start a temporary namespace, in case expr is a declaration
  my_builder->start_namespace("if", NamespaceUnique);
  // Parse expression
  translate(PTree::third(node));
  // Store a copy of any declarations for use in the else block
  std::vector<AST::Declaration*> decls = my_builder->scope()->declarations();
  // Translate then-statement. If a block then we avoid starting a new ns
  PTree::Node *stmt = PTree::nth(node, 4);
  if (stmt && PTree::first(stmt) && *PTree::first(stmt) == '{') visit((PTree::Brace *)stmt);
  else translate(stmt);
  // End the block and check for else
  my_builder->end_namespace();
  if (PTree::length(node) == 7)
  {
    if (my_links) my_links->span(PTree::nth(node, 5), "file-keyword");
    AST::Namespace* ns = my_builder->start_namespace("else", NamespaceUnique);
    ns->declarations().insert(ns->declarations().begin(), decls.begin(), decls.end());
    // Translate else statement, same deal as above
    stmt = PTree::nth(node, 6);
    if (stmt && PTree::first(stmt) && *PTree::first(stmt) == '{') visit((PTree::Brace*)stmt);
    else translate(stmt);
    my_builder->end_namespace();
  }
}

void SWalker::visit(PTree::SwitchStatement *node)
{
  STrace trace("SWalker::visit(SwitchStatement*)");
  if (my_links) find_comments(node);
  if (my_links) my_links->span(PTree::first(node), "file-keyword");
  my_builder->start_namespace("switch", NamespaceUnique);
  // Parse expression
  translate(PTree::third(node));
  // Translate statement. If a block then we avoid starting a new ns
  PTree::Node *stmt = PTree::nth(node, 4);
  if (stmt && PTree::first(stmt) && *PTree::first(stmt) == '{') visit((PTree::Brace *)stmt);
  else translate(stmt);
  // End the block and check for else
  my_builder->end_namespace();
}

void SWalker::visit(PTree::CaseStatement *node)
{
  STrace trace("SWalker::visit(Case*)");
  if (my_links) find_comments(node);
  if (my_links) my_links->span(PTree::first(node), "file-keyword");
  translate(PTree::second(node));
  translate(PTree::nth(node, 3));
}

void SWalker::visit(PTree::DefaultStatement *node)
{
  STrace trace("SWalker::visit(DefaultStatement*)");
  if (my_links) find_comments(node);
  if (my_links) my_links->span(PTree::first(node), "file-keyword");
  translate(PTree::third(node));
}

void SWalker::visit(PTree::BreakStatement *node)
{
  STrace trace("SWalker::visit(Break*)");
  if (my_links) find_comments(node);
  if (my_links) my_links->span(PTree::first(node), "file-keyword");
}

void SWalker::visit(PTree::ForStatement *node)
{
  STrace trace("SWalker::visit(For*)");
  if (my_links) find_comments(node);
  if (my_links) my_links->span(PTree::first(node), "file-keyword");
  my_builder->start_namespace("for", NamespaceUnique);
  // Parse expressions
  translate(PTree::third(node));
  translate(PTree::nth(node, 3));
  translate(PTree::nth(node, 5));
  // Translate statement. If a block then we avoid starting a new ns
  PTree::Node *stmt = PTree::nth(node, 7);
  if (stmt && PTree::first(stmt) && *PTree::first(stmt) == '{')
    visit((PTree::Brace *)stmt);
  else translate(stmt);
  // End the block
  my_builder->end_namespace();
}

void SWalker::visit(PTree::WhileStatement *node)
{
  STrace trace("SWalker::visit(While*)");
  if (my_links) find_comments(node);
  if (my_links) my_links->span(PTree::first(node), "file-keyword");
  my_builder->start_namespace("while", NamespaceUnique);
  // Parse expression
  translate(PTree::third(node));
  // Translate statement. If a block then we avoid starting a new ns
  PTree::Node *stmt = PTree::nth(node, 4);
  if (stmt && PTree::first(stmt) && *PTree::first(stmt) == '{')
    visit((PTree::Brace *)stmt);
  else translate(stmt);
  // End the block and check for else
  my_builder->end_namespace();
}

void SWalker::visit(PTree::PostfixExpr *node)
{
  STrace trace("SWalker::visit(Postfix*)");
  translate(PTree::first(node));
}

void SWalker::visit(PTree::ParenExpr *node)
{
  STrace trace("SWalker::visit(Paren*)");
  if (my_links) find_comments(node);
  translate(PTree::second(node));
}

void SWalker::visit(PTree::CastExpr *node)
{
  STrace trace("SWalker::visit(Cast*)");
  if (my_links) find_comments(node);
  PTree::Node *type_expr = PTree::second(node);
  //Translate(type_expr->First());
  PTree::Encoding enc = PTree::second(type_expr)->encoded_type();
  if (!enc.empty())
  {
    my_decoder->init(enc);
    my_type = my_decoder->decodeType();
    my_type = TypeResolver(my_builder).resolve(my_type);
    if (my_type && my_links)
      my_links->link(PTree::first(type_expr), my_type);
  }
  else my_type = 0;
  translate(PTree::nth(node, 3));
}

void SWalker::visit(PTree::TryStatement *node)
{
  STrace trace("SWalker::visit(Try*)");
  if (my_links) find_comments(node);
  if (my_links) my_links->span(PTree::first(node), "file-keyword");
  my_builder->start_namespace("try", NamespaceUnique);
  translate(PTree::second(node));
  my_builder->end_namespace();
  for (int n = 2; n < PTree::length(node); n++)
  {
    // [ catch ( arg ) [{}] ]
    PTree::Node *catch_node = PTree::nth(node, n);
    if (my_links) my_links->span(PTree::first(catch_node), "file-keyword");
    my_builder->start_namespace("catch", NamespaceUnique);
    PTree::Node *arg = PTree::third(catch_node);
    if (PTree::length(arg) == 2)
    {
      // Get the arg type
      my_decoder->init(PTree::second(arg)->encoded_type());
      Types::Type* arg_type = my_decoder->decodeType();
      // Link the type
      Types::Type* arg_link = TypeResolver(my_builder).resolve(arg_type);
      if (my_links) my_links->link(PTree::first(arg), arg_link);
      // Create a declaration for the argument
      if (PTree::second(arg))
      {
	PTree::Encoding enc = PTree::second(arg)->encoded_name();
	if (!enc.empty())
	{
	  std::string name = my_decoder->decodeName(enc);
	  my_builder->add_variable(my_lineno, name, arg_type, false, "exception");
	}
      }
    }
    // Translate contents of 'catch' block
    translate(PTree::nth(catch_node, 4));
    my_builder->end_namespace();
  }
}

void SWalker::visit(PTree::ArrayExpr *node)
{
  STrace trace("SWalker::visit(ArrayExpr*)");
  translate(PTree::first(node));
  Types::Type* object = my_type;
  
  translate(PTree::third(node));
  Types::Type* arg = my_type;
  
  if (!object || !arg)
  {
    my_type = 0;
    return;
  }
  // Resolve final type
  try
  {
    TypeFormatter tf;
    LOG("ARRAY-OPER: " << tf.format(object) << " [] " << tf.format(arg));
    AST::Function* func;
    my_type = my_lookup->arrayOperator(object, arg, func);
    if (func && my_links)
    {
      // Link the [ and ] to the function operator used
      my_links->link(PTree::nth(node, 1), func->declared());
      my_links->link(PTree::nth(node, 3), func->declared());
    }
  }
  catch (const TranslateError& e)
  {
    e.set_node(node);
    throw;
  }
}

void SWalker::visit(PTree::CondExpr *node)
{
  STrace trace("SWalker::visit(Cond*)");
  translate(PTree::nth(node, 0));
  translate(PTree::nth(node, 2));
  translate(PTree::nth(node, 4));
}

void SWalker::visit(PTree::Kwd::This *node)
{
  STrace trace("SWalker::visit(This*)");
  if (my_links) find_comments(node);
  if (my_links) my_links->span(node, "file-keyword");
  // Set my_type to type of 'this', stored in the name lookup for this func
  my_type = my_lookup->lookupType("this");
}

void SWalker::visit(PTree::TemplateInstantiation *)
{
  STrace trace("SWalker::visit(TemplateInstantiation*) NYI");
}

void SWalker::visit(PTree::ExternTemplate *)
{
  STrace trace("SWalker::visit(ExternTemplate*) NYI");
}

void SWalker::visit(PTree::MetaclassDecl *)
{
  STrace trace("SWalker::visit(MetaclassDecl*) NYI");
}

PTree::Node *SWalker::translate_storage_specifiers(PTree::Node *)
{
  STrace trace("SWalker::translate_storage_specifiers NYI");
  return 0;
}

PTree::Node *SWalker::translate_function_body(PTree::Node *)
{
  STrace trace("SWalker::translate_function_body NYI");
  return 0;
}

//PTree::Node *SWalker::TranslateEnumSpec(PTree::Node *) { STrace trace("SWalker::TranslateEnumSpec NYI"); return 0; }

void SWalker::visit(PTree::AccessDecl *node)
{
  STrace trace("SWalker::visit(AccessDecl*) NYI");
  if (my_links) find_comments(node);
#ifdef DEBUG
  node->print(std::cout);
#endif
}

void SWalker::visit(PTree::UserAccessSpec *node)
{
  STrace trace("SWalker::visist(UserAccessSpec*) NYI");
  if (my_links) find_comments(node);
#ifdef DEBUG
  node->print(std::cout);
#endif
}

void SWalker::visit(PTree::DoStatement *node)
{
  STrace trace("SWalker::visit(Do*) NYI");
  if (my_links)
  {
    find_comments(node);
    my_links->span(PTree::first(node), "file-keyword");
    my_links->span(PTree::third(node), "file-keyword");
  }
  // Translate block
  my_builder->start_namespace("do", NamespaceUnique);
  // Translate statement. If a block then we avoid starting a new ns
  PTree::Node *stmt = PTree::second(node);
  if (stmt && PTree::first(stmt) && *PTree::first(stmt) == '{')
    visit((PTree::Brace*)stmt);
  else translate(stmt);
  // End the block and check for else
  my_builder->end_namespace();
  // Translate the while condition
  translate(PTree::nth(node, 4));
}

void SWalker::visit(PTree::ContinueStatement *node)
{
  STrace trace("SWalker::visit(Continue*) NYI");
  if (my_links) find_comments(node);
  if (my_links) my_links->span(PTree::first(node), "file-keyword");
}

void SWalker::visit(PTree::GotoStatement *node)
{
  STrace trace("SWalker::visist(Goto*) NYI");
  if (my_links) find_comments(node);
#ifdef DEBUG  
  node->print(std::cout);
#endif
}

void SWalker::visit(PTree::LabelStatement *node)
{
  STrace trace("SWalker::visit(Label*) NYI");
  if (my_links) find_comments(node);
#ifdef DEBUG
  node->print(std::cout);
#endif
}

void SWalker::visit(PTree::CommaExpr *node)
{
  STrace trace("SWalker::visit(Comma*)");
  PTree::Node *node2 = node;
  while (node2)
  {
    translate(PTree::first(node2));
    node2 = PTree::rest(node2);
    if (node2) node2 = PTree::rest(node2);
  }
}

void SWalker::visit(PTree::PmExpr *node)
{
  STrace trace("SWalker::visit(Pm*) NYI");
#ifdef DEBUG
  node->print(std::cout);
#endif
}

void SWalker::visit(PTree::ThrowExpr *node)
{
  STrace trace("SWalker::visit(Throw*)");
  if (my_links) find_comments(node);
  if (my_links) my_links->span(PTree::first(node), "file-keyword");
  translate(PTree::second(node));
}

void SWalker::visit(PTree::SizeofExpr *node)
{
  STrace trace("SWalker::visit(Sizeof*)");
  if (my_links) find_comments(node);
  if (my_links) my_links->span(PTree::first(node), "file-keyword");
  // TODO: find the type for highlighting, and figure out what the ??? is
  my_type = my_lookup->lookupType("int");
}

void SWalker::visit(PTree::NewExpr *node)
{
  STrace trace("SWalker::visit(New*) NYI");
  if (my_links) find_comments(node);
#ifdef DEBUG
  node->print(std::cout);
#endif
}

PTree::Node *SWalker::translate_new3(PTree::Node *node)
{
  STrace trace("SWalker::translate_new3 NYI");
  if (my_links) find_comments(node);
#ifdef DEBUG
  node->print(std::cout);
#endif
  return 0;
}

void SWalker::visit(PTree::DeleteExpr *node)
{
  STrace trace("SWalker::visit(DeleteExpr*)");
  if (my_links) find_comments(node);
  if (my_links) my_links->span(PTree::first(node), "file-keyword");
  translate(PTree::second(node));
}

void SWalker::visit(PTree::FstyleCastExpr *node)
{
  STrace trace("SWalker::visit(FstyleCast*) NYI");
  if (my_links) find_comments(node);
  my_type = 0;
  //Translate(node->Third()); <-- unknown ptree???? FIXME
  my_decoder->init(node->encoded_type());
  my_type = my_decoder->decodeType();
  // TODO: Figure out if should have called a function for this
}

void SWalker::visit(PTree::UserStatementExpr *node)
{
  STrace trace("SWalker::visit(UserStatement*) NYI");
#ifdef DEBUG
  node->print(std::cout);
#endif
}

void SWalker::visit(PTree::StaticUserStatementExpr *node)
{
  STrace trace("SWalker::visit(StaticUserStatement*) NYI");
#ifdef DEBUG
  node->print(std::cout);
#endif
}

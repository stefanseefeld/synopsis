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

#include <occ/PTree.hh>
#include <occ/Parser.hh>
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

PTree::Node *
SWalker::TranslateReturn(PTree::Node *spec)
{
  STrace trace("SWalker::TranslateReturn");
  if (!my_links) return 0;

  // Link 'return' keyword
  my_links->span(PTree::first(spec), "file-keyword");

  // Translate the body of the return, if present
  if (PTree::length(spec) == 3) Translate(PTree::second(spec));
  return 0;
}

PTree::Node *
SWalker::TranslateInfix(PTree::Node *node)
{
  STrace trace("SWalker::TranslateInfix");
  // [left op right]
  Translate(PTree::first(node));
  Types::Type* left_type = my_type;
  Translate(PTree::third(node));
  Types::Type* right_type = my_type;
  std::string oper = parse_name(PTree::second(node));
  TypeFormatter tf;
  LOG("BINARY-OPER: " << tf.format(left_type) << " " << oper << " " << tf.format(right_type));
  nodeLOG(node);
  if (!left_type || !right_type)
  {
    my_type = 0;
    return 0;
  }
  // Lookup an appropriate operator
  AST::Function* func = my_lookup->lookupOperator(oper, left_type, right_type);
  if (func)
  {
    my_type = func->return_type();
    if (my_links) my_links->link(PTree::second(node), func->declared());
  }
  return 0;
}

PTree::Node *
SWalker::TranslateVariable(PTree::Node *spec)
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
  return 0;
}

void
SWalker::TranslateFunctionArgs(PTree::Node *args)
{
  // args: [ arg (, arg)* ]
  while (PTree::length(args))
  {
    PTree::Node *arg = PTree::first(args);
    // Translate this arg, TODO: my_params would be better as a vector<Type*>
    my_type = 0;
    Translate(arg);
    my_params.push_back(my_type);
    // Skip over arg and comma
    args = PTree::rest(PTree::rest(args));
  }
}

PTree::Node *
SWalker::TranslateFuncall(PTree::Node *node) 	// and fstyle cast
{
  STrace trace("SWalker::TranslateFuncall");
  // TODO: figure out how to deal with fstyle casts.. does it only apply to
  // base types? eg: int(4.0) ?
  // This is similar to TranslateVariable, except we have to check params..
  // doh! That means more my_type nastiness
  //
  // [ postfix ( args ) ]
  LOG(node);
  // In translating the postfix the last var should be looked up as a
  // function. This means we have to find the args first, and store them in
  // my_params as a hint
  Types::Type::vector save_params = my_params;
  my_params.clear();
  try
  {
    TranslateFunctionArgs(PTree::third(node));
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
    Translate(PTree::first(node));
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
  return 0;
}

PTree::Node *
SWalker::TranslateExprStatement(PTree::Node *node)
{
  STrace trace("SWalker::TranslateExprStatement");
  Translate(PTree::first(node));
  return 0;
}

PTree::Node *
SWalker::TranslateUnary(PTree::Node *node)
{
  STrace trace("SWalker::TranslateUnary");
  // [op expr]
  if (my_links) find_comments(node);
  // TODO: lookup unary operator
  Translate(PTree::second(node));
  return 0;
}

PTree::Node *
SWalker::TranslateAssign(PTree::Node *node)
{
  STrace trace("SWalker::TranslateAssign");
  // [left = right]
  // TODO: lookup = operator
  my_type = 0;
  Translate(PTree::first(node));
  Types::Type* ret_type = my_type;
  Translate(PTree::third(node));
  my_type = ret_type;
  return 0;
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

PTree::Node *
SWalker::TranslateArrowMember(PTree::Node *node)
{
  STrace trace("SWalker::TranslateArrowMember");
  // [ postfix -> name ]
  my_type = 0;
  my_scope = 0;
  Postfix_Flag save_flag = my_postfix_flag;
  my_postfix_flag = Postfix_Var;
  Translate(PTree::first(node));
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
  Translate(PTree::third(node));
  my_scope = 0;
  return 0;
}

PTree::Node *
SWalker::TranslateDotMember(PTree::Node *node)
{
  STrace trace("SWalker::TranslateDotMember");
  // [ postfix . name ]
  my_type = 0;
  my_scope = 0;
  Postfix_Flag save_flag = my_postfix_flag;
  my_postfix_flag = Postfix_Var;
  Translate(PTree::first(node));
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
  Translate(PTree::third(node));
  my_scope = 0;
  return 0;
}

PTree::Node *
SWalker::TranslateIf(PTree::Node *node)
{
  STrace trace("SWalker::TranslateIf");
  // [ if ( expr ) statement (else statement)? ]
  if (my_links) find_comments(node);
  if (my_links) my_links->span(PTree::first(node), "file-keyword");
  // Start a temporary namespace, in case expr is a declaration
  my_builder->start_namespace("if", NamespaceUnique);
  // Parse expression
  Translate(PTree::third(node));
  // Store a copy of any declarations for use in the else block
  std::vector<AST::Declaration*> decls = my_builder->scope()->declarations();
  // Translate then-statement. If a block then we avoid starting a new ns
  PTree::Node *stmt = PTree::nth(node, 4);
  if (stmt && PTree::first(stmt) && *PTree::first(stmt) == '{') TranslateBrace(stmt);
  else Translate(stmt);
  // End the block and check for else
  my_builder->end_namespace();
  if (PTree::length(node) == 7)
  {
    if (my_links) my_links->span(PTree::nth(node, 5), "file-keyword");
    AST::Namespace* ns = my_builder->start_namespace("else", NamespaceUnique);
    ns->declarations().insert(ns->declarations().begin(), decls.begin(), decls.end());
    // Translate else statement, same deal as above
    stmt = PTree::nth(node, 6);
    if (stmt && PTree::first(stmt) && *PTree::first(stmt) == '{') TranslateBrace(stmt);
    else Translate(stmt);
    my_builder->end_namespace();
  }
  return 0;
}

PTree::Node *
SWalker::TranslateSwitch(PTree::Node *node)
{
  STrace trace("SWalker::TranslateSwitch");
  // [ switch ( expr ) statement ]
  if (my_links) find_comments(node);
  if (my_links) my_links->span(PTree::first(node), "file-keyword");
  my_builder->start_namespace("switch", NamespaceUnique);
  // Parse expression
  Translate(PTree::third(node));
  // Translate statement. If a block then we avoid starting a new ns
  PTree::Node *stmt = PTree::nth(node, 4);
  if (stmt && PTree::first(stmt) && *PTree::first(stmt) == '{') TranslateBrace(stmt);
  else Translate(stmt);
  // End the block and check for else
  my_builder->end_namespace();
  return 0;
}

PTree::Node *
SWalker::TranslateCase(PTree::Node *node)
{
  STrace trace("SWalker::TranslateCase");
  // [ case expr : [expr] ]
  if (my_links) find_comments(node);
  if (my_links) my_links->span(PTree::first(node), "file-keyword");
  Translate(PTree::second(node));
  Translate(PTree::nth(node, 3));
  return 0;
}

PTree::Node *
SWalker::TranslateDefault(PTree::Node *node)
{
  STrace trace("SWalker::TranslateDefault");
  // [ default : [expr] ]
  if (my_links) find_comments(node);
  if (my_links) my_links->span(PTree::first(node), "file-keyword");
  Translate(PTree::third(node));
  return 0;
}

PTree::Node *
SWalker::TranslateBreak(PTree::Node *node)
{
  STrace trace("SWalker::TranslateBreak");
  // [ break ; ]
  if (my_links) find_comments(node);
  if (my_links) my_links->span(PTree::first(node), "file-keyword");
  return 0;
}

PTree::Node *
SWalker::TranslateFor(PTree::Node *node)
{
  STrace trace("SWalker::TranslateFor");
  // [ for ( stmt expr ; expr ) statement ]
  if (my_links) find_comments(node);
  if (my_links) my_links->span(PTree::first(node), "file-keyword");
  my_builder->start_namespace("for", NamespaceUnique);
  // Parse expressions
  Translate(PTree::third(node));
  Translate(PTree::nth(node, 3));
  Translate(PTree::nth(node, 5));
  // Translate statement. If a block then we avoid starting a new ns
  PTree::Node *stmt = PTree::nth(node, 7);
  if (stmt && PTree::first(stmt) && *PTree::first(stmt) == '{')
    TranslateBrace(stmt);
  else Translate(stmt);
  // End the block
  my_builder->end_namespace();
  return 0;
}

PTree::Node *
SWalker::TranslateWhile(PTree::Node *node)
{
  STrace trace("SWalker::TranslateWhile");
  // [ while ( expr ) statement ]
  if (my_links) find_comments(node);
  if (my_links) my_links->span(PTree::first(node), "file-keyword");
  my_builder->start_namespace("while", NamespaceUnique);
  // Parse expression
  Translate(PTree::third(node));
  // Translate statement. If a block then we avoid starting a new ns
  PTree::Node *stmt = PTree::nth(node, 4);
  if (stmt && PTree::first(stmt) && *PTree::first(stmt) == '{') TranslateBrace(stmt);
  else Translate(stmt);
  // End the block and check for else
  my_builder->end_namespace();
  return 0;
}

PTree::Node *
SWalker::TranslatePostfix(PTree::Node *node)
{
  STrace trace("SWalker::TranslatePostfix");
  // [ expr ++ ]
  Translate(PTree::first(node));
  return 0;
}

PTree::Node *
SWalker::TranslateParen(PTree::Node *node)
{
  STrace trace("SWalker::TranslateParen");
  // [ ( expr ) ]
  if (my_links) find_comments(node);
  Translate(PTree::second(node));
  return 0;
}

PTree::Node *
SWalker::TranslateCast(PTree::Node *node)
{
  STrace trace("SWalker::TranslateCast");
  // ( type-expr ) expr   ..type-expr is type encoded
  if (my_links) find_comments(node);
  PTree::Node *type_expr = PTree::second(node);
  //Translate(type_expr->First());
  if (PTree::second(type_expr)->encoded_type())
  {
    my_decoder->init(PTree::second(type_expr)->encoded_type());
    my_type = my_decoder->decodeType();
    my_type = TypeResolver(my_builder).resolve(my_type);
    if (my_type && my_links)
      my_links->link(PTree::first(type_expr), my_type);
  }
  else my_type = 0;
  Translate(PTree::nth(node, 3));
  return 0;
}

PTree::Node *
SWalker::TranslateTry(PTree::Node *node)
{
  STrace trace("SWalker::TranslateTry");
  // [ try [{}] [catch ( arg ) [{}] ]* ]
  if (my_links) find_comments(node);
  if (my_links) my_links->span(PTree::first(node), "file-keyword");
  my_builder->start_namespace("try", NamespaceUnique);
  Translate(PTree::second(node));
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
      if (PTree::second(arg) && PTree::second(arg)->encoded_name())
      {
        std::string name = my_decoder->decodeName(PTree::second(arg)->encoded_name());
        my_builder->add_variable(my_lineno, name, arg_type, false, "exception");
      }
    }
    // Translate contents of 'catch' block
    Translate(PTree::nth(catch_node, 4));
    my_builder->end_namespace();
  }
  return 0;
}

PTree::Node *
SWalker::TranslateArray(PTree::Node *node)
{
  STrace trace("SWalker::TranslateArray");
  // <postfix> \[ <expr> \]
  Translate(PTree::first(node));
  Types::Type* object = my_type;
  
  Translate(PTree::third(node));
  Types::Type* arg = my_type;
  
  if (!object || !arg)
  {
    my_type = 0;
    return 0;
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
  return 0;
}

PTree::Node *
SWalker::TranslateCond(PTree::Node *node)
{
  STrace trace("SWalker::TranslateCond");
  Translate(PTree::nth(node, 0));
  Translate(PTree::nth(node, 2));
  Translate(PTree::nth(node, 4));
  return 0;
}

PTree::Node *
SWalker::TranslateThis(PTree::Node *node)
{
  STrace trace("SWalker::TranslateThis");
  if (my_links) find_comments(node);
  if (my_links) my_links->span(node, "file-keyword");
  // Set my_type to type of 'this', stored in the name lookup for this func
  my_type = my_lookup->lookupType("this");
  return 0;
}


PTree::Node *
SWalker::TranslateTemplateInstantiation(PTree::Node *)
{
  STrace trace("SWalker::TranslateTemplateInstantiation NYI");
  return 0;
}
PTree::Node *
SWalker::TranslateExternTemplate(PTree::Node *)
{
  STrace trace("SWalker::TranslateExternTemplate NYI");
  return 0;
}
PTree::Node *
SWalker::TranslateMetaclassDecl(PTree::Node *)
{
  STrace trace("SWalker::TranslateMetaclassDecl NYI");
  return 0;
}

PTree::Node *
SWalker::TranslateStorageSpecifiers(PTree::Node *)
{
  STrace trace("SWalker::TranslateStorageSpecifiers NYI");
  return 0;
}
PTree::Node *
SWalker::TranslateFunctionBody(PTree::Node *)
{
  STrace trace("SWalker::TranslateFunctionBody NYI");
  return 0;
}

//PTree::Node *SWalker::TranslateEnumSpec(PTree::Node *) { STrace trace("SWalker::TranslateEnumSpec NYI"); return 0; }

PTree::Node *
SWalker::TranslateAccessDecl(PTree::Node *node)
{
  STrace trace("SWalker::TranslateAccessDecl NYI");
  if (my_links) find_comments(node);
#ifdef DEBUG

  node->print(std::cout);
#endif

  return 0;
}

PTree::Node *
SWalker::TranslateUserAccessSpec(PTree::Node *node)
{
  STrace trace("SWalker::TranslateUserAccessSpec NYI");
  if (my_links) find_comments(node);
#ifdef DEBUG

  node->print(std::cout);
#endif

  return 0;
}


PTree::Node *
SWalker::TranslateDo(PTree::Node *node)
{
  STrace trace("SWalker::TranslateDo NYI");
  // [ do [{ ... }] while ( [...] ) ; ]
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
  if (stmt && PTree::first(stmt) && *PTree::first(stmt) == '{') TranslateBrace(stmt);
  else Translate(stmt);
  // End the block and check for else
  my_builder->end_namespace();
  // Translate the while condition
  Translate(PTree::nth(node, 4));
  return 0;
}

PTree::Node *
SWalker::TranslateContinue(PTree::Node *node)
{
  STrace trace("SWalker::TranslateContinue NYI");
  if (my_links) find_comments(node);
  if (my_links) my_links->span(PTree::first(node), "file-keyword");
  return 0;
}

PTree::Node *
SWalker::TranslateGoto(PTree::Node *node)
{
  STrace trace("SWalker::TranslateGoto NYI");
  if (my_links) find_comments(node);
#ifdef DEBUG
  
  node->print(std::cout);
#endif

  return 0;
}

PTree::Node *
SWalker::TranslateLabel(PTree::Node *node)
{
  STrace trace("SWalker::TranslateLabel NYI");
  if (my_links) find_comments(node);
#ifdef DEBUG

  node->print(std::cout);
#endif

  return 0;
}


PTree::Node *
SWalker::TranslateComma(PTree::Node *node)
{
  STrace trace("SWalker::TranslateComma");
  // [ expr , expr (, expr)* ]
  while (node)
  {
    Translate(PTree::first(node));
    node = PTree::rest(node);
    if (node) node = PTree::rest(node);
  }
  return 0;
}

PTree::Node *
SWalker::TranslatePm(PTree::Node *node)
{
  STrace trace("SWalker::TranslatePm NYI");
#ifdef DEBUG

  node->print(std::cout);
#endif

  return 0;
}

PTree::Node *
SWalker::TranslateThrow(PTree::Node *node)
{
  STrace trace("SWalker::TranslateThrow");
  // [ throw [expr] ]
  if (my_links) find_comments(node);
  if (my_links) my_links->span(PTree::first(node), "file-keyword");
  Translate(PTree::second(node));
  return 0;
}

PTree::Node *
SWalker::TranslateSizeof(PTree::Node *node)
{
  STrace trace("SWalker::TranslateSizeof");
  // [ sizeof ( [type [???] ] ) ]
  if (my_links) find_comments(node);
  if (my_links) my_links->span(PTree::first(node), "file-keyword");
  // TODO: find the type for highlighting, and figure out what the ??? is
  my_type = my_lookup->lookupType("int");
  return 0;
}

PTree::Node *
SWalker::TranslateNew(PTree::Node *node)
{
  STrace trace("SWalker::TranslateNew NYI");
  if (my_links) find_comments(node);
#ifdef DEBUG

  node->print(std::cout);
#endif

  return 0;
}

PTree::Node *
SWalker::TranslateNew3(PTree::Node *node)
{
  STrace trace("SWalker::TranslateNew3 NYI");
  if (my_links) find_comments(node);
#ifdef DEBUG

  node->print(std::cout);
#endif

  return 0;
}

PTree::Node *
SWalker::TranslateDelete(PTree::Node *node)
{
  STrace trace("SWalker::TranslateDelete");
  // [ delete [expr] ]
  if (my_links) find_comments(node);
  if (my_links) my_links->span(PTree::first(node), "file-keyword");
  Translate(PTree::second(node));
  return 0;
}

PTree::Node *
SWalker::TranslateFstyleCast(PTree::Node *node)
{
  STrace trace("SWalker::TranslateFstyleCast NYI");
  if (my_links) find_comments(node);
  // [ [type] ( [expr] ) ]
  my_type = 0;
  //Translate(node->Third()); <-- unknown ptree???? FIXME
  my_decoder->init(node->encoded_type());
  my_type = my_decoder->decodeType();
  // TODO: Figure out if should have called a function for this
  return 0;
}

PTree::Node *
SWalker::TranslateUserStatement(PTree::Node *node)
{
  STrace trace("SWalker::TranslateUserStatement NYI");
#ifdef DEBUG

  node->print(std::cout);
#endif

  return 0;
}

PTree::Node *
SWalker::TranslateStaticUserStatement(PTree::Node *node)
{
  STrace trace("SWalker::TranslateStaticUserStatement NYI");
#ifdef DEBUG

  node->print(std::cout);
#endif

  return 0;
}

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

#include <occ/AST.hh>
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

Ptree*
SWalker::TranslateReturn(Ptree* spec)
{
  STrace trace("SWalker::TranslateReturn");
  if (!my_links) return 0;

  // Link 'return' keyword
  my_links->span(spec->First(), "file-keyword");

  // Translate the body of the return, if present
  if (spec->Length() == 3) Translate(spec->Second());
  return 0;
}

Ptree*
SWalker::TranslateInfix(Ptree* node)
{
  STrace trace("SWalker::TranslateInfix");
  // [left op right]
  Translate(node->First());
  Types::Type* left_type = my_type;
  Translate(node->Third());
  Types::Type* right_type = my_type;
  std::string oper = parse_name(node->Second());
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
    if (my_links) my_links->link(node->Second(), func->declared());
  }
  return 0;
}

Ptree*
SWalker::TranslateVariable(Ptree* spec)
{
  // REVISIT: Figure out why this is so long!!!
  STrace trace("SWalker::TranslateVariable");
  if (my_links) find_comments(spec);
  try
  {
    Ptree* name_spec = spec;
    Types::Named* type;
    ScopedName scoped_name;
    if (!spec->IsLeaf())
    {
      // Must be a scoped name.. iterate through the scopes
      // stop when spec is at the last name
      //nodeLOG(spec);
      // If first node is '::' then reset my_scope to the global scope
      if (spec->First()->Eq("::"))
      {
        scoped_name.push_back("");
        spec = spec->Rest();
      }
      while (spec->Length() > 2)
      {
        scoped_name.push_back(parse_name(spec->First()));
        /*
          if (!type) { throw nodeERROR(spec, "scope '" << parse_name(spec->First()) << "' not found!"); }
          try { my_scope = Types::declared_cast<AST::Scope>(type); }
          catch (const Types::wrong_type_cast&) { throw nodeERROR(spec, "scope '"<<parse_name(spec->First())<<"' found but not a scope!"); }
          // Link the scope name
          if (my_links) my_links->link(spec->First(), my_scope->declared());
        */
        spec = spec->Rest()->Rest();
      }
      spec = spec->First();
      // Check for 'operator >>' type syntax:
      if (!spec->IsLeaf() && spec->Length() == 2 && spec->First()->Eq("operator"))
      {
        // Name lookup is done based on only the operator type, so
        // skip the 'operator' node
        spec = spec->Second();
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
SWalker::TranslateFunctionArgs(Ptree* args)
{
  // args: [ arg (, arg)* ]
  while (args->Length())
  {
    Ptree* arg = args->First();
    // Translate this arg, TODO: my_params would be better as a vector<Type*>
    my_type = 0;
    Translate(arg);
    my_params.push_back(my_type);
    // Skip over arg and comma
    args = Ptree::Rest(Ptree::Rest(args));
  }
}

Ptree*
SWalker::TranslateFuncall(Ptree* node) 	// and fstyle cast
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
    TranslateFunctionArgs(node->Third());
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
    Translate(node->First());
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

Ptree*
SWalker::TranslateExprStatement(Ptree* node)
{
  STrace trace("SWalker::TranslateExprStatement");
  Translate(node->First());
  return 0;
}

Ptree*
SWalker::TranslateUnary(Ptree* node)
{
  STrace trace("SWalker::TranslateUnary");
  // [op expr]
  if (my_links) find_comments(node);
  // TODO: lookup unary operator
  Translate(node->Second());
  return 0;
}

Ptree*
SWalker::TranslateAssign(Ptree* node)
{
  STrace trace("SWalker::TranslateAssign");
  // [left = right]
  // TODO: lookup = operator
  my_type = 0;
  Translate(node->First());
  Types::Type* ret_type = my_type;
  Translate(node->Third());
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

Ptree*
SWalker::TranslateArrowMember(Ptree* node)
{
  STrace trace("SWalker::TranslateArrowMember");
  // [ postfix -> name ]
  my_type = 0;
  my_scope = 0;
  Postfix_Flag save_flag = my_postfix_flag;
  my_postfix_flag = Postfix_Var;
  Translate(node->First());
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
  Translate(node->Third());
  my_scope = 0;
  return 0;
}

Ptree*
SWalker::TranslateDotMember(Ptree* node)
{
  STrace trace("SWalker::TranslateDotMember");
  // [ postfix . name ]
  my_type = 0;
  my_scope = 0;
  Postfix_Flag save_flag = my_postfix_flag;
  my_postfix_flag = Postfix_Var;
  Translate(node->First());
  my_postfix_flag = save_flag;
  LOG(parse_name(node->First()) << " resolved to " << my_type_formatter->format(my_type));
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
  Translate(node->Third());
  my_scope = 0;
  return 0;
}

Ptree*
SWalker::TranslateIf(Ptree* node)
{
  STrace trace("SWalker::TranslateIf");
  // [ if ( expr ) statement (else statement)? ]
  if (my_links) find_comments(node);
  if (my_links) my_links->span(node->First(), "file-keyword");
  // Start a temporary namespace, in case expr is a declaration
  my_builder->start_namespace("if", NamespaceUnique);
  // Parse expression
  Translate(node->Third());
  // Store a copy of any declarations for use in the else block
  std::vector<AST::Declaration*> decls = my_builder->scope()->declarations();
  // Translate then-statement. If a block then we avoid starting a new ns
  Ptree* stmt = node->Nth(4);
  if (stmt && stmt->First() && stmt->First()->Eq('{')) TranslateBrace(stmt);
  else Translate(stmt);
  // End the block and check for else
  my_builder->end_namespace();
  if (node->Length() == 7)
  {
    if (my_links) my_links->span(node->Nth(5), "file-keyword");
    AST::Namespace* ns = my_builder->start_namespace("else", NamespaceUnique);
    ns->declarations().insert(ns->declarations().begin(), decls.begin(), decls.end());
    // Translate else statement, same deal as above
    stmt = node->Nth(6);
    if (stmt && stmt->First() && stmt->First()->Eq('{')) TranslateBrace(stmt);
    else Translate(stmt);
    my_builder->end_namespace();
  }
  return 0;
}

Ptree*
SWalker::TranslateSwitch(Ptree* node)
{
  STrace trace("SWalker::TranslateSwitch");
  // [ switch ( expr ) statement ]
  if (my_links) find_comments(node);
  if (my_links) my_links->span(node->First(), "file-keyword");
  my_builder->start_namespace("switch", NamespaceUnique);
  // Parse expression
  Translate(node->Third());
  // Translate statement. If a block then we avoid starting a new ns
  Ptree* stmt = node->Nth(4);
  if (stmt && stmt->First() && stmt->First()->Eq('{')) TranslateBrace(stmt);
  else Translate(stmt);
  // End the block and check for else
  my_builder->end_namespace();
  return 0;
}

Ptree*
SWalker::TranslateCase(Ptree* node)
{
  STrace trace("SWalker::TranslateCase");
  // [ case expr : [expr] ]
  if (my_links) find_comments(node);
  if (my_links) my_links->span(node->First(), "file-keyword");
  Translate(node->Second());
  Translate(node->Nth(3));
  return 0;
}

Ptree*
SWalker::TranslateDefault(Ptree* node)
{
  STrace trace("SWalker::TranslateDefault");
  // [ default : [expr] ]
  if (my_links) find_comments(node);
  if (my_links) my_links->span(node->First(), "file-keyword");
  Translate(node->Third());
  return 0;
}

Ptree*
SWalker::TranslateBreak(Ptree* node)
{
  STrace trace("SWalker::TranslateBreak");
  // [ break ; ]
  if (my_links) find_comments(node);
  if (my_links) my_links->span(node->First(), "file-keyword");
  return 0;
}

Ptree*
SWalker::TranslateFor(Ptree* node)
{
  STrace trace("SWalker::TranslateFor");
  // [ for ( stmt expr ; expr ) statement ]
  if (my_links) find_comments(node);
  if (my_links) my_links->span(node->First(), "file-keyword");
  my_builder->start_namespace("for", NamespaceUnique);
  // Parse expressions
  Translate(node->Third());
  Translate(node->Nth(3));
  Translate(node->Nth(5));
  // Translate statement. If a block then we avoid starting a new ns
  Ptree* stmt = node->Nth(7);
  if (stmt && stmt->First() && stmt->First()->Eq('{'))
    TranslateBrace(stmt);
  else Translate(stmt);
  // End the block
  my_builder->end_namespace();
  return 0;
}

Ptree*
SWalker::TranslateWhile(Ptree* node)
{
  STrace trace("SWalker::TranslateWhile");
  // [ while ( expr ) statement ]
  if (my_links) find_comments(node);
  if (my_links) my_links->span(node->First(), "file-keyword");
  my_builder->start_namespace("while", NamespaceUnique);
  // Parse expression
  Translate(node->Third());
  // Translate statement. If a block then we avoid starting a new ns
  Ptree* stmt = node->Nth(4);
  if (stmt && stmt->First() && stmt->First()->Eq('{')) TranslateBrace(stmt);
  else Translate(stmt);
  // End the block and check for else
  my_builder->end_namespace();
  return 0;
}

Ptree*
SWalker::TranslatePostfix(Ptree* node)
{
  STrace trace("SWalker::TranslatePostfix");
  // [ expr ++ ]
  Translate(node->First());
  return 0;
}

Ptree*
SWalker::TranslateParen(Ptree* node)
{
  STrace trace("SWalker::TranslateParen");
  // [ ( expr ) ]
  if (my_links) find_comments(node);
  Translate(node->Second());
  return 0;
}

Ptree*
SWalker::TranslateCast(Ptree* node)
{
  STrace trace("SWalker::TranslateCast");
  // ( type-expr ) expr   ..type-expr is type encoded
  if (my_links) find_comments(node);
  Ptree* type_expr = node->Second();
  //Translate(type_expr->First());
  if (type_expr->Second()->GetEncodedType())
  {
    my_decoder->init(type_expr->Second()->GetEncodedType());
    my_type = my_decoder->decodeType();
    my_type = TypeResolver(my_builder).resolve(my_type);
    if (my_type && my_links)
      my_links->link(type_expr->First(), my_type);
  }
  else my_type = 0;
  Translate(node->Nth(3));
  return 0;
}

Ptree*
SWalker::TranslateTry(Ptree* node)
{
  STrace trace("SWalker::TranslateTry");
  // [ try [{}] [catch ( arg ) [{}] ]* ]
  if (my_links) find_comments(node);
  if (my_links) my_links->span(node->First(), "file-keyword");
  my_builder->start_namespace("try", NamespaceUnique);
  Translate(node->Second());
  my_builder->end_namespace();
  for (int n = 2; n < node->Length(); n++)
  {
    // [ catch ( arg ) [{}] ]
    Ptree* catch_node = node->Nth(n);
    if (my_links) my_links->span(catch_node->First(), "file-keyword");
    my_builder->start_namespace("catch", NamespaceUnique);
    Ptree* arg = catch_node->Third();
    if (arg->Length() == 2)
    {
      // Get the arg type
      my_decoder->init(arg->Second()->GetEncodedType());
      Types::Type* arg_type = my_decoder->decodeType();
      // Link the type
      Types::Type* arg_link = TypeResolver(my_builder).resolve(arg_type);
      if (my_links) my_links->link(arg->First(), arg_link);
      // Create a declaration for the argument
      if (arg->Second() && arg->Second()->GetEncodedName())
      {
        std::string name = my_decoder->decodeName(arg->Second()->GetEncodedName());
        my_builder->add_variable(my_lineno, name, arg_type, false, "exception");
      }
    }
    // Translate contents of 'catch' block
    Translate(catch_node->Nth(4));
    my_builder->end_namespace();
  }
  return 0;
}

Ptree*
SWalker::TranslateArray(Ptree* node)
{
  STrace trace("SWalker::TranslateArray");
  // <postfix> \[ <expr> \]
  Translate(node->First());
  Types::Type* object = my_type;
  
  Translate(node->Third());
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
      my_links->link(node->Nth(1), func->declared());
      my_links->link(node->Nth(3), func->declared());
    }
  }
  catch (const TranslateError& e)
  {
    e.set_node(node);
    throw;
  }
  return 0;
}

Ptree*
SWalker::TranslateCond(Ptree* node)
{
  STrace trace("SWalker::TranslateCond");
  Translate(node->Nth(0));
  Translate(node->Nth(2));
  Translate(node->Nth(4));
  return 0;
}

Ptree*
SWalker::TranslateThis(Ptree* node)
{
  STrace trace("SWalker::TranslateThis");
  if (my_links) find_comments(node);
  if (my_links) my_links->span(node, "file-keyword");
  // Set my_type to type of 'this', stored in the name lookup for this func
  my_type = my_lookup->lookupType("this");
  return 0;
}


Ptree*
SWalker::TranslateTemplateInstantiation(Ptree*)
{
  STrace trace("SWalker::TranslateTemplateInstantiation NYI");
  return 0;
}
Ptree*
SWalker::TranslateExternTemplate(Ptree*)
{
  STrace trace("SWalker::TranslateExternTemplate NYI");
  return 0;
}
Ptree*
SWalker::TranslateMetaclassDecl(Ptree*)
{
  STrace trace("SWalker::TranslateMetaclassDecl NYI");
  return 0;
}

Ptree*
SWalker::TranslateStorageSpecifiers(Ptree*)
{
  STrace trace("SWalker::TranslateStorageSpecifiers NYI");
  return 0;
}
Ptree*
SWalker::TranslateFunctionBody(Ptree*)
{
  STrace trace("SWalker::TranslateFunctionBody NYI");
  return 0;
}

//Ptree* SWalker::TranslateEnumSpec(Ptree*) { STrace trace("SWalker::TranslateEnumSpec NYI"); return 0; }

Ptree*
SWalker::TranslateAccessDecl(Ptree* node)
{
  STrace trace("SWalker::TranslateAccessDecl NYI");
  if (my_links) find_comments(node);
#ifdef DEBUG

  node->print(std::cout);
#endif

  return 0;
}

Ptree*
SWalker::TranslateUserAccessSpec(Ptree* node)
{
  STrace trace("SWalker::TranslateUserAccessSpec NYI");
  if (my_links) find_comments(node);
#ifdef DEBUG

  node->print(std::cout);
#endif

  return 0;
}


Ptree*
SWalker::TranslateDo(Ptree* node)
{
  STrace trace("SWalker::TranslateDo NYI");
  // [ do [{ ... }] while ( [...] ) ; ]
  if (my_links)
  {
    find_comments(node);
    my_links->span(node->First(), "file-keyword");
    my_links->span(node->Third(), "file-keyword");
  }
  // Translate block
  my_builder->start_namespace("do", NamespaceUnique);
  // Translate statement. If a block then we avoid starting a new ns
  Ptree* stmt = node->Second();
  if (stmt && stmt->First() && stmt->First()->Eq('{')) TranslateBrace(stmt);
  else Translate(stmt);
  // End the block and check for else
  my_builder->end_namespace();
  // Translate the while condition
  Translate(node->Nth(4));
  return 0;
}

Ptree*
SWalker::TranslateContinue(Ptree* node)
{
  STrace trace("SWalker::TranslateContinue NYI");
  if (my_links) find_comments(node);
  if (my_links) my_links->span(node->First(), "file-keyword");
  return 0;
}

Ptree*
SWalker::TranslateGoto(Ptree* node)
{
  STrace trace("SWalker::TranslateGoto NYI");
  if (my_links) find_comments(node);
#ifdef DEBUG
  
  node->print(std::cout);
#endif

  return 0;
}

Ptree*
SWalker::TranslateLabel(Ptree* node)
{
  STrace trace("SWalker::TranslateLabel NYI");
  if (my_links) find_comments(node);
#ifdef DEBUG

  node->print(std::cout);
#endif

  return 0;
}


Ptree*
SWalker::TranslateComma(Ptree* node)
{
  STrace trace("SWalker::TranslateComma");
  // [ expr , expr (, expr)* ]
  while (node)
  {
    Translate(node->First());
    node = node->Rest();
    if (node) node = node->Rest();
  }
  return 0;
}

Ptree*
SWalker::TranslatePm(Ptree* node)
{
  STrace trace("SWalker::TranslatePm NYI");
#ifdef DEBUG

  node->print(std::cout);
#endif

  return 0;
}

Ptree*
SWalker::TranslateThrow(Ptree* node)
{
  STrace trace("SWalker::TranslateThrow");
  // [ throw [expr] ]
  if (my_links) find_comments(node);
  if (my_links) my_links->span(node->First(), "file-keyword");
  Translate(node->Second());
  return 0;
}

Ptree*
SWalker::TranslateSizeof(Ptree* node)
{
  STrace trace("SWalker::TranslateSizeof");
  // [ sizeof ( [type [???] ] ) ]
  if (my_links) find_comments(node);
  if (my_links) my_links->span(node->First(), "file-keyword");
  // TODO: find the type for highlighting, and figure out what the ??? is
  my_type = my_lookup->lookupType("int");
  return 0;
}

Ptree*
SWalker::TranslateNew(Ptree* node)
{
  STrace trace("SWalker::TranslateNew NYI");
  if (my_links) find_comments(node);
#ifdef DEBUG

  node->print(std::cout);
#endif

  return 0;
}

Ptree*
SWalker::TranslateNew3(Ptree* node)
{
  STrace trace("SWalker::TranslateNew3 NYI");
  if (my_links) find_comments(node);
#ifdef DEBUG

  node->print(std::cout);
#endif

  return 0;
}

Ptree*
SWalker::TranslateDelete(Ptree* node)
{
  STrace trace("SWalker::TranslateDelete");
  // [ delete [expr] ]
  if (my_links) find_comments(node);
  if (my_links) my_links->span(node->First(), "file-keyword");
  Translate(node->Second());
  return 0;
}

Ptree*
SWalker::TranslateFstyleCast(Ptree* node)
{
  STrace trace("SWalker::TranslateFstyleCast NYI");
  if (my_links) find_comments(node);
  // [ [type] ( [expr] ) ]
  my_type = 0;
  //Translate(node->Third()); <-- unknown ptree???? FIXME
  my_decoder->init(node->GetEncodedType());
  my_type = my_decoder->decodeType();
  // TODO: Figure out if should have called a function for this
  return 0;
}

Ptree*
SWalker::TranslateUserStatement(Ptree* node)
{
  STrace trace("SWalker::TranslateUserStatement NYI");
#ifdef DEBUG

  node->print(std::cout);
#endif

  return 0;
}

Ptree*
SWalker::TranslateStaticUserStatement(Ptree* node)
{
  STrace trace("SWalker::TranslateStaticUserStatement NYI");
#ifdef DEBUG

  node->print(std::cout);
#endif

  return 0;
}

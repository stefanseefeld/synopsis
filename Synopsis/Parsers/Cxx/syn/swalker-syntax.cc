// $Id: swalker-syntax.cc,v 1.11 2001/07/23 15:29:35 chalky Exp $
//
// This file is a part of Synopsis.
// Copyright (C) 2000, 2001 Stephen Davies
// Copyright (C) 2000, 2001 Stefan Seefeld
//
// Synopsis is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
// 02111-1307, USA.
//
// $Log: swalker-syntax.cc,v $
// Revision 1.11  2001/07/23 15:29:35  chalky
// Fixed some regressions and other mis-features
//
// Revision 1.10  2001/07/23 11:51:22  chalky
// Better support for name lookup wrt namespaces.
//
// Revision 1.9  2001/07/19 13:50:52  chalky
// Support 'using' somewhat, L"const" literals, and better Qual.Templ. names
//
// Revision 1.8  2001/06/11 10:37:30  chalky
// Operators! Arrays! (and probably more that I forget)
//
// Revision 1.7  2001/06/10 07:17:37  chalky
// Comment fix, better functions, linking etc. Better link titles
//
// Revision 1.6  2001/06/10 00:31:39  chalky
// Refactored link storage, better comments, better parsing
//
// Revision 1.5  2001/06/06 07:51:45  chalky
// Fixes and moving towards SXR
//
// Revision 1.4  2001/06/05 03:49:33  chalky
// Made my own wrong_type_cast exception. Added template support to qualified
// names (its bad but it doesnt crash). Added vector<string> output op to builder
//
// Revision 1.3  2001/05/06 20:15:03  stefan
// fixes to get std compliant; replaced some pass-by-value by pass-by-const-ref; bug fixes;
//
// Revision 1.2  2001/04/03 23:01:37  chalky
// Small fixes and extra comments
//
// Revision 1.1  2001/03/16 04:42:00  chalky
// SXR parses expressions, handles differences from macro expansions. Some work
// on function call resolution.
//
//

// This file contains the methods of SWalker that are mostly concerned with
// syntax highlighting (SXR). Having them in a separate file improves
// compilation time significantly for minor updates.
//
//
// From here on, the Translate* visitor methods are mostly only used when
// storing links, ie: they translate code rather than declarations.
//

#include <iostream.h>
#include <string>
#include <typeinfo>

#include "ptree.h"
#include "parse.h"

#include "linkstore.hh"
#include "swalker.hh"
#include "builder.hh"
#include "decoder.hh"
#include "dumper.hh"
#include "strace.hh"
#include "type.hh"
#include "ast.hh"

Ptree* SWalker::TranslateReturn(Ptree* spec) {
    STrace trace("SWalker::TranslateReturn");
    if (!m_links) return 0;

    // Link 'return' keyword
    m_links->span(spec->First(), "file-keyword");

    // Translate the body of the return, if present
    if (spec->Length() == 3) Translate(spec->Second());
    return 0;
}

Ptree* SWalker::TranslateInfix(Ptree* node) {
    STrace trace("SWalker::TranslateInfix"); 
    // [left op right]
    Translate(node->First());
    Type::Type* left_type = m_type;
    Translate(node->Third());
    Type::Type* right_type = m_type;
    std::string oper = getName(node->Second());
    TypeFormatter tf;
    LOG("BINARY-OPER: " << tf.format(left_type) << " " << oper << " " << tf.format(right_type));
    nodeLOG(node);
    if (!left_type || !right_type) { m_type = NULL; return 0; }
    // Lookup an appropriate operator
    AST::Function* func = m_builder->lookupOperator(oper, left_type, right_type);
    if (func) {
	m_type = func->returnType();
	if (m_links) m_links->link(node->Second(), func->declared());
    }
    return 0;
}

Ptree* SWalker::TranslateVariable(Ptree* spec) {
    STrace trace("SWalker::TranslateVariable");
    if (m_links) findComments(spec);
    try {
	Ptree* name_spec = spec;
	Type::Named* type;
	AST::Name scoped_name;
	if (!spec->IsLeaf()) {
	    // Must be a scoped name.. iterate through the scopes
	    // stop when spec is at the last name
	    nodeLOG(spec);
	    // If first node is '::' then reset m_scope to the global scope
	    if (spec->First()->Eq("::")) {
		scoped_name.push_back("");
		spec = spec->Rest();
	    }
	    while (spec->Length() > 2) {
		scoped_name.push_back(getName(spec->First()));
		/*
		if (!type) { throw nodeERROR(spec, "scope '" << getName(spec->First()) << "' not found!"); }
		try { m_scope = Type::declared_cast<AST::Scope>(type); }
		catch (const Type::wrong_type_cast&) { throw nodeERROR(spec, "scope '"<<getName(spec->First())<<"' found but not a scope!"); }
		// Link the scope name
		if (m_links) m_links->link(spec->First(), m_scope->declared());
		*/
		spec = spec->Rest()->Rest();
	    }
	    spec = spec->First();
	    // Check for 'operator >>' type syntax:
	    if (!spec->IsLeaf() && spec->Length() == 2 && spec->First()->Eq("operator")) {
		// Name lookup is done based on only the operator type, so
		// skip the 'operator' node
		spec = spec->Second();
	    }
	    scoped_name.push_back(getName(spec));
	}
	std::string name = getName(spec);
	if (m_postfix_flag == Postfix_Var) {
	    // Variable lookup. m_type will be the vtype
	    /*cout << "m_scope is " << (m_scope ? m_type_formatter->format(m_scope->declared()) : "global") << endl;*/
	    if (!scoped_name.empty()) type = m_builder->lookupType(scoped_name, true, m_scope);
	    else if (m_scope) type = m_builder->lookupType(name, m_scope);
	    else type = m_builder->lookupType(name);
	    if (!type) { throw nodeERROR(spec, "variable '" << name << "' not found!"); }
	    // Now find vtype (throw wrong_type_cast if not a variable)
	    try {
		Type::Declared& declared = dynamic_cast<Type::Declared&>(*type);
		// The variable could be a Variable or Enumerator
		AST::Variable* var; AST::Enumerator* enumor;
		if ((var = dynamic_cast<AST::Variable*>(declared.declaration())) != 0) {
		    // It is a variable
		    m_type = var->vtype();
		    // Store a link to the variable itself (not its type)
		    if (m_links) m_links->link(name_spec, type);
		    /*cout << "var type name is " << m_type_formatter->format(m_type) << endl;*/
		} else if ((enumor = dynamic_cast<AST::Enumerator*>(declared.declaration())) != 0) {
		    // It is an enumerator
		    m_type = 0; // we have no use for enums in type code
		    // But still a link is needed
		    if (m_links) m_links->link(name_spec, type);
		    /*cout << "enum type name is " << m_type_formatter->format(type) << endl;*/
		} else {
		    throw nodeERROR(name_spec, "var was not a Variable nor Enumerator!");
		}
	    } catch (const std::bad_cast &) {
		if (dynamic_cast<Type::Unknown*>(type))
		    throw nodeERROR(spec, "variable '" << name << "' was an Unknown type!");
		if (dynamic_cast<Type::Base*>(type))
		    throw nodeERROR(spec, "variable '" << name << "' was a Base type!");
		throw nodeERROR(spec, "variable '" << name << "' wasn't a declared type!");
	    }
	} else {
	    // Function lookup. m_type will be returnType. params are in m_params
	    AST::Scope* scope = m_scope ;
	    if (!scope) scope = m_builder->scope();
	    // if (!scoped_name.empty()) func = m_builder->lookupFunc(scoped_name, scope, m_params);
	    AST::Function* func = m_builder->lookupFunc(name, scope, m_params);
	    if (!func) { throw nodeERROR(name_spec, "Warning: function '" << name << "' not found!"); }
	    // Store a link to the function name
	    if (m_links) m_links->link(name_spec, func->declared());
	    // Now find returnType
	    m_type = func->returnType();
	}
    } 
    catch(TranslateError& e) {
	m_scope = 0;
	m_type = 0;
	e.set_node(spec);
	throw;
    }
    catch(const Type::wrong_type_cast &) {
	throw nodeERROR(spec, "wrong type error in TranslateVariable!");
    }
    catch(...) {
	throw nodeERROR(spec, "unknown error in TranslateVariable!");
    }
    m_scope = 0;
    return 0; 
}

void SWalker::TranslateFunctionArgs(Ptree* args)
{
    // args: [ arg (, arg)* ]
    while (args->Length()) {
	Ptree* arg = args->First();
	// Translate this arg, TODO: m_params would be better as a vector<Type*>
	m_type = 0;
	Translate(arg);
	m_params.push_back(m_type);
	// Skip over arg and comma
	args = Ptree::Rest(Ptree::Rest(args));
    }
}

Ptree* SWalker::TranslateFuncall(Ptree* node) {	// and fstyle cast
    STrace trace("SWalker::TranslateFuncall");
    // TODO: figure out what the "fstyle cast" comment means... sounds bad :)
    // This is similar to TranslateVariable, except we have to check params..
    // doh! That means more m_type nastiness
    //
    // [ postfix ( args ) ]
    LOG(node);
    // In translating the postfix the last var should be looked up as a
    // function. This means we have to find the args first, and store them in
    // m_params as a hint
    std::vector<Type::Type*> save_params = m_params;
    m_params.clear();
    try {
	TranslateFunctionArgs(node->Third());
    } catch (...) {
	// Restore params before rethrowing exception
	m_params = save_params;
	throw;
    }

    Postfix_Flag save_flag = m_postfix_flag;
    try {
	m_postfix_flag = Postfix_Func;
	Translate(node->First());
    } catch (...) {
	// Restore params and flag before rethrowing exception
	m_params = save_params;
	m_postfix_flag = save_flag;
	throw;
    }

    // Restore m_params since we're done with it now
    m_params = save_params;
    m_postfix_flag = save_flag;
    return 0;
}

Ptree* SWalker::TranslateExprStatement(Ptree* node) {
    STrace trace("SWalker::TranslateExprStatement");
    Translate(node->First());
    return 0;
}

Ptree* SWalker::TranslateUnary(Ptree* node) {
    STrace trace("SWalker::TranslateUnary");
    // [op expr]
    if (m_links) findComments(node);
    // TODO: lookup unary operator
    Translate(node->Second());
    return 0;
}

Ptree* SWalker::TranslateAssign(Ptree* node) {
    STrace trace("SWalker::TranslateAssign");
    // [left = right]
    // TODO: lookup = operator
    m_type = 0;
    Translate(node->First());
    Type::Type* ret_type = m_type;
    Translate(node->Third());
    m_type = ret_type; return 0;
}

//. Resolves the final type of any given Type. For example, it traverses
//. typedefs and parametrized types, and resolves unknowns by looking them up.
class TypeResolver : public Type::Visitor {
public:
    //. Constructor - needs a Builder to resolve unknowns with
    TypeResolver(Builder* b) { m_builder = b; }
    //. Resolves the given type object
    Type::Type* resolve(Type::Type* t) { m_type = t; t->accept(this); return m_type; }
    //. Tries to resolve the given type object to a Scope
    AST::Scope* scope(Type::Type* t) throw (Type::wrong_type_cast, TranslateError) {
	return Type::declared_cast<AST::Scope>(resolve(t));
    }
    //. Looks up the unknown type for a fresh definition
    void visitUnknown(Type::Unknown* t) {
	m_type = m_builder->resolveType(t);
	if (!dynamic_cast<Type::Unknown*>(m_type))
	    m_type->accept(this);
    }
    //. Recursively processes the aliased type
    void visitModifier(Type::Modifier* t) { t->alias()->accept(this); }
    //. Checks for typedefs and recursively processes them
    void visitDeclared(Type::Declared* t) {
	AST::Typedef* tdef = dynamic_cast<AST::Typedef*>(t->declaration());
	if (tdef) { tdef->alias()->accept(this); }
	else m_type = t;
    }
    //. Processes the template type
    void visitParameterized(Type::Parameterized* t) {
	if (t->templateType())
	    t->templateType()->accept(this);
    }
protected:
    Builder* m_builder; //.< A reference to the builder object
    Type::Type* m_type; //.< The type to return
};

Ptree* SWalker::TranslateArrowMember(Ptree* node) {
    STrace trace("SWalker::TranslateArrowMember");
    // [ postfix -> name ]
    m_type = 0; m_scope = 0;
    Postfix_Flag save_flag = m_postfix_flag;
    m_postfix_flag = Postfix_Var;
    Translate(node->First());
    m_postfix_flag = save_flag;
    // m_type should be a modifier to a declared to a class. Throw bad_cast if not
    if (!m_type) { throw nodeERROR(node, "Unable to resolve type of LHS of ->"); }
    try {
	m_scope = TypeResolver(m_builder).scope(m_type);
    } catch (const Type::wrong_type_cast&) { throw nodeERROR(node, "LHS of -> was not a scope!"); }
    // Find member, m_type becomes the var type or func returnType
    Translate(node->Third());
    m_scope = 0;
    return 0;
}

Ptree* SWalker::TranslateDotMember(Ptree* node) {
    STrace trace("SWalker::TranslateDotMember");
    // [ postfix . name ]
    m_type = 0; m_scope = 0;
    Postfix_Flag save_flag = m_postfix_flag;
    m_postfix_flag = Postfix_Var;
    Translate(node->First());
    m_postfix_flag = save_flag;
    LOG(getName(node->First()) << " resolved to " << m_type_formatter->format(m_type));
    // m_type should be a declared to a class
    if (!m_type) { throw nodeERROR(node, "Unable to resolve type of LHS of ."); }
    LOG("resolving type to scope");
    // Check for reference type
    try {
	m_scope = TypeResolver(m_builder).scope(m_type);
    } catch (const Type::wrong_type_cast &) { throw nodeERROR(node, "Warning: LHS of . was not a scope: " << m_type_formatter->format(m_type)); }
    // Find member, m_type becomes the var type or func returnType
    LOG("translating third");
    Translate(node->Third());
    m_scope = 0;
    return 0;
}

Ptree* SWalker::TranslateIf(Ptree* node) {
    STrace trace("SWalker::TranslateIf");
    // [ if ( expr ) statement (else statement)? ]
    if (m_links) findComments(node);
    if (m_links) m_links->span(node->First(), "file-keyword");
    // Start a temporary namespace, in case expr is a declaration
    m_builder->startNamespace("if", Builder::NamespaceUnique);
    // Parse expression
    Translate(node->Third());
    // Store a copy of any declarations for use in the else block
    std::vector<AST::Declaration*> decls = m_builder->scope()->declarations();
    // Translate then-statement. If a block then we avoid starting a new ns
    Ptree* stmt = node->Nth(4);
    if (stmt && stmt->First() && stmt->First()->Eq('{')) TranslateBrace(stmt);
    else Translate(stmt);
    // End the block and check for else
    m_builder->endNamespace();
    if (node->Length() == 7) {
	if (m_links) m_links->span(node->Nth(5), "file-keyword");
	AST::Namespace* ns = m_builder->startNamespace("else", Builder::NamespaceUnique);
	ns->declarations().insert(
		ns->declarations().begin(),
		decls.begin(), decls.end()
	);
	// Translate else statement, same deal as above
	stmt = node->Nth(6);
	if (stmt && stmt->First() && stmt->First()->Eq('{')) TranslateBrace(stmt);
	else Translate(stmt);
	m_builder->endNamespace();
    }
    return 0;
}

Ptree* SWalker::TranslateSwitch(Ptree* node) {
    STrace trace("SWalker::TranslateSwitch");
    // [ switch ( expr ) statement ]
    if (m_links) findComments(node);
    if (m_links) m_links->span(node->First(), "file-keyword");
    m_builder->startNamespace("switch", Builder::NamespaceUnique);
    // Parse expression
    Translate(node->Third());
    // Translate statement. If a block then we avoid starting a new ns
    Ptree* stmt = node->Nth(4);
    if (stmt && stmt->First() && stmt->First()->Eq('{')) TranslateBrace(stmt);
    else Translate(stmt);
    // End the block and check for else
    m_builder->endNamespace();
    return 0;
}

Ptree* SWalker::TranslateCase(Ptree* node) {
    STrace trace("SWalker::TranslateCase");
    // [ case expr : [expr] ]
    if (m_links) findComments(node);
    if (m_links) m_links->span(node->First(), "file-keyword");
    Translate(node->Second());
    Translate(node->Nth(3));
    return 0;
}

Ptree* SWalker::TranslateDefault(Ptree* node) {
    STrace trace("SWalker::TranslateDefault");
    // [ default : [expr] ]
    if (m_links) findComments(node);
    if (m_links) m_links->span(node->First(), "file-keyword");
    Translate(node->Third());
    return 0;
}

Ptree* SWalker::TranslateBreak(Ptree* node) {
    STrace trace("SWalker::TranslateBreak");
    // [ break ; ]
    if (m_links) findComments(node);
    if (m_links) m_links->span(node->First(), "file-keyword");
    return 0;
}

Ptree* SWalker::TranslateFor(Ptree* node) {
    STrace trace("SWalker::TranslateFor");
    // [ for ( stmt expr ; expr ) statement ]
    if (m_links) findComments(node);
    if (m_links) m_links->span(node->First(), "file-keyword");
    m_builder->startNamespace("for", Builder::NamespaceUnique);
    // Parse expressions
    Translate(node->Third());
    Translate(node->Nth(3));
    Translate(node->Nth(5));
    // Translate statement. If a block then we avoid starting a new ns
    Ptree* stmt = node->Nth(7);
    if (stmt && stmt->First() && stmt->First()->Eq('{')) TranslateBrace(stmt);
    else Translate(stmt);
    // End the block
    m_builder->endNamespace();
    return 0;
}

Ptree* SWalker::TranslateWhile(Ptree* node) {
    STrace trace("SWalker::TranslateWhile");
    // [ while ( expr ) statement ]
    if (m_links) findComments(node);
    if (m_links) m_links->span(node->First(), "file-keyword");
    m_builder->startNamespace("while", Builder::NamespaceUnique);
    // Parse expression
    Translate(node->Third());
    // Translate statement. If a block then we avoid starting a new ns
    Ptree* stmt = node->Nth(4);
    if (stmt && stmt->First() && stmt->First()->Eq('{')) TranslateBrace(stmt);
    else Translate(stmt);
    // End the block and check for else
    m_builder->endNamespace();
    return 0;
}

Ptree* SWalker::TranslatePostfix(Ptree* node) {
    STrace trace("SWalker::TranslatePostfix");
    // [ expr ++ ]
    Translate(node->First());
    return 0;
}

Ptree* SWalker::TranslateParen(Ptree* node) {
    STrace trace("SWalker::TranslateParen");
    // [ ( expr ) ]
    if (m_links) findComments(node);
    Translate(node->Second());
    return 0;
}

Ptree* SWalker::TranslateCast(Ptree* node) {
    STrace trace("SWalker::TranslateCast");
    // ( type-expr ) expr   ..type-expr is type encoded
    if (m_links) findComments(node);
    Ptree* type_expr = node->Second();
    //Translate(type_expr->First());
    if (type_expr->Second()->GetEncodedType()) {
	m_decoder->init(type_expr->Second()->GetEncodedType());
	m_type = m_decoder->decodeType();
	m_type = TypeResolver(m_builder).resolve(m_type);
	if (m_type && m_links) m_links->link(type_expr->First(), m_type);
    } else m_type = 0;
    Translate(node->Nth(3));
    return 0;
}

Ptree* SWalker::TranslateTry(Ptree* node) {
    STrace trace("SWalker::TranslateTry");
    // [ try [{}] [catch ( arg ) [{}] ]* ]
    if (m_links) findComments(node);
    if (m_links) m_links->span(node->First(), "file-keyword");
    m_builder->startNamespace("try", Builder::NamespaceUnique);
    Translate(node->Second());
    m_builder->endNamespace();
    for (int n = 2; n < node->Length(); n++) {
	// [ catch ( arg ) [{}] ]
	Ptree* catch_node = node->Nth(n);
	if (m_links) m_links->span(catch_node->First(), "file-keyword");
	m_builder->startNamespace("catch", Builder::NamespaceUnique);
	Ptree* arg = catch_node->Third();
	if (arg->Length() == 2) {
	    // Get the arg type
	    m_decoder->init(arg->Second()->GetEncodedType());
	    Type::Type* arg_type = m_decoder->decodeType();
	    // Link the type
	    Type::Type* arg_link = TypeResolver(m_builder).resolve(arg_type);
	    if (m_links) m_links->link(arg->First(), arg_link);
	    // Create a declaration for the argument
	    if (arg->Second() && arg->Second()->GetEncodedName()) {
		std::string name = m_decoder->decodeName(arg->Second()->GetEncodedName());
		m_builder->addVariable(m_lineno, name, arg_type, false, "exception");
	    }
	}
	// Translate contents of 'catch' block
	Translate(catch_node->Nth(4));
	m_builder->endNamespace();
    }
    return 0;
}

Ptree* SWalker::TranslateArray(Ptree* node) {
    STrace trace("SWalker::TranslateArray");
    // <postfix> \[ <expr> \]
    Translate(node->First());
    Type::Type* object = m_type;

    Translate(node->Third());
    Type::Type* arg = m_type;

    if (!object || !arg) { m_type = NULL; return 0; }
    // Resolve final type
    try {
	TypeFormatter tf;
	LOG("ARRAY-OPER: " << tf.format(object) << " [] " << tf.format(arg));
	AST::Function* func;
	m_type = m_builder->arrayOperator(object, arg, func);
	if (func && m_links) {
	    // Link the [ and ] to the function operator used
	    m_links->link(node->Nth(1), func->declared());
	    m_links->link(node->Nth(3), func->declared());
	}
    } catch(TranslateError& e) {
	e.set_node(node);
	throw;
    }
    return 0;
}

Ptree* SWalker::TranslateCond(Ptree* node) {
    STrace trace("SWalker::TranslateCond");
    Translate(node->Nth(0));
    Translate(node->Nth(2));
    Translate(node->Nth(4));
    return 0;
}

Ptree* SWalker::TranslateThis(Ptree* node) {
    STrace trace("SWalker::TranslateThis");
    if (m_links) findComments(node);
    if (m_links) m_links->span(node, "file-keyword");
    // Set m_type to type of 'this', stored in the name lookup for this func
    m_type = m_builder->lookupType("this");
    return 0;
}


Ptree* SWalker::TranslateTemplateInstantiation(Ptree*) { STrace trace("SWalker::TranslateTemplateInstantiation NYI"); return 0; }
Ptree* SWalker::TranslateExternTemplate(Ptree*) { STrace trace("SWalker::TranslateExternTemplate NYI"); return 0; }
Ptree* SWalker::TranslateTemplateFunction(Ptree*, Ptree*) { STrace trace("SWalker::TranslateTemplateFunction NYI"); return 0; }
Ptree* SWalker::TranslateMetaclassDecl(Ptree*) { STrace trace("SWalker::TranslateMetaclassDecl NYI"); return 0; }

Ptree* SWalker::TranslateStorageSpecifiers(Ptree*) { STrace trace("SWalker::TranslateStorageSpecifiers NYI"); return 0; }
Ptree* SWalker::TranslateFunctionBody(Ptree*) { STrace trace("SWalker::TranslateFunctionBody NYI"); return 0; }

//Ptree* SWalker::TranslateEnumSpec(Ptree*) { STrace trace("SWalker::TranslateEnumSpec NYI"); return 0; }

Ptree* SWalker::TranslateAccessDecl(Ptree* node) {
    STrace trace("SWalker::TranslateAccessDecl NYI");
    if (m_links) findComments(node);
#ifdef DEBUG
    node->Display2(cout);
#endif
    return 0;
}

Ptree* SWalker::TranslateUserAccessSpec(Ptree* node) {
    STrace trace("SWalker::TranslateUserAccessSpec NYI");
    if (m_links) findComments(node);
#ifdef DEBUG
    node->Display2(cout);
#endif
    return 0;
}


Ptree* SWalker::TranslateDo(Ptree* node) {
    STrace trace("SWalker::TranslateDo NYI");
    // [ do [{ ... }] while ( [...] ) ; ]
    if (m_links) {
	findComments(node);
	m_links->span(node->First(), "file-keyword");
	m_links->span(node->Third(), "file-keyword");
    }
    // Translate block
    m_builder->startNamespace("do", Builder::NamespaceUnique);
    // Translate statement. If a block then we avoid starting a new ns
    Ptree* stmt = node->Second();
    if (stmt && stmt->First() && stmt->First()->Eq('{')) TranslateBrace(stmt);
    else Translate(stmt);
    // End the block and check for else
    m_builder->endNamespace();
    // Translate the while condition
    Translate(node->Nth(4));
    return 0;
}

Ptree* SWalker::TranslateContinue(Ptree* node) {
    STrace trace("SWalker::TranslateContinue NYI");
    if (m_links) findComments(node);
    if (m_links) m_links->span(node->First(), "file-keyword");
    return 0;
}

Ptree* SWalker::TranslateGoto(Ptree* node) {
    STrace trace("SWalker::TranslateGoto NYI");
    if (m_links) findComments(node);
#ifdef DEBUG
    node->Display2(cout);
#endif
    return 0;
}

Ptree* SWalker::TranslateLabel(Ptree* node) {
    STrace trace("SWalker::TranslateLabel NYI");
    if (m_links) findComments(node);
#ifdef DEBUG
    node->Display2(cout);
#endif
    return 0;
}


Ptree* SWalker::TranslateComma(Ptree* node) {
    STrace trace("SWalker::TranslateComma");
    // [ expr , expr (, expr)* ]
    while (node) {
	Translate(node->First());
	node = node->Rest();
	if (node) node = node->Rest();
    }
    return 0;
}

Ptree* SWalker::TranslatePm(Ptree* node) {
    STrace trace("SWalker::TranslatePm NYI");
#ifdef DEBUG
    node->Display2(cout);
#endif
    return 0;
}

Ptree* SWalker::TranslateThrow(Ptree* node) {
    STrace trace("SWalker::TranslateThrow");
    // [ throw [expr] ]
    if (m_links) findComments(node);
    if (m_links) m_links->span(node->First(), "file-keyword");
    Translate(node->Second());
    return 0;
}

Ptree* SWalker::TranslateSizeof(Ptree* node) {
    STrace trace("SWalker::TranslateSizeof");
    // [ sizeof ( [type [???] ] ) ]
    if (m_links) findComments(node);
    if (m_links) m_links->span(node->First(), "file-keyword");
    // TODO: find the type for highlighting, and figure out what the ??? is
    m_type = m_builder->lookupType("int");
    return 0;
}

Ptree* SWalker::TranslateNew(Ptree* node) {
    STrace trace("SWalker::TranslateNew NYI");
    if (m_links) findComments(node);
#ifdef DEBUG
    node->Display2(cout);
#endif
    return 0;
}

Ptree* SWalker::TranslateNew3(Ptree* node) {
    STrace trace("SWalker::TranslateNew3 NYI");
    if (m_links) findComments(node);
#ifdef DEBUG
    node->Display2(cout);
#endif
    return 0;
}

Ptree* SWalker::TranslateDelete(Ptree* node) {
    STrace trace("SWalker::TranslateDelete");
    // [ delete [expr] ]
    if (m_links) findComments(node);
    if (m_links) m_links->span(node->First(), "file-keyword");
    Translate(node->Second());
    return 0;
}

Ptree* SWalker::TranslateFstyleCast(Ptree* node) {
    STrace trace("SWalker::TranslateFstyleCast NYI");
    if (m_links) findComments(node);
    // [ [type] ( [expr] ) ]
    m_type = 0;
    //Translate(node->Third()); <-- unknown ptree???? FIXME
    m_decoder->init(node->GetEncodedType());
    m_type = m_decoder->decodeType();
    // TODO: Figure out if should have called a function for this
    return 0;
}

Ptree* SWalker::TranslateUserStatement(Ptree* node) {
    STrace trace("SWalker::TranslateUserStatement NYI");
#ifdef DEBUG
    node->Display2(cout);
#endif
    return 0;
}

Ptree* SWalker::TranslateStaticUserStatement(Ptree* node) {
    STrace trace("SWalker::TranslateStaticUserStatement NYI");
#ifdef DEBUG
    node->Display2(cout);
#endif
    return 0;
}




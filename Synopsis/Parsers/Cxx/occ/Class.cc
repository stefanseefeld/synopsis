/*
  Copyright (C) 1997-2000 Shigeru Chiba, University of Tsukuba.

  Permission to use, copy, distribute and modify this software and   
  its documentation for any purpose is hereby granted without fee,        
  provided that the above copyright notice appear in all copies and that 
  both that copyright notice and this permission notice appear in 
  supporting documentation.

  Shigeru Chiba makes no representations about the suitability of this 
  software for any purpose.  It is provided "as is" without express or
  implied warranty.
*/

#include <iostream>
#include <cstring>
#include <stdexcept>
#include "Lexer.hh"
#include "Class.hh"
#include "Environment.hh"
#include "AST.hh"
#include "Walker.hh"
#include "ClassWalker.hh"
#include "TypeInfo.hh"
#include "Encoding.hh"

ClassArray* Class::class_list = 0;
int Class::num_of_cmd_options = 0;
char* Class::cmd_options[];

char* Class::metaclass_for_c_functions = 0;
Class* Class::for_c_functions = 0;

Ptree* Class::class_t = 0;
Ptree* Class::empty_block_t = 0;
Ptree* Class::public_t = 0;
Ptree* Class::protected_t = 0;
Ptree* Class::private_t = 0;
Ptree* Class::virtual_t = 0;
Ptree* Class::colon_t = 0;
Ptree* Class::comma_t = 0;
Ptree* Class::semicolon_t = 0;

static opcxx_ListOfMetaclass* classCreator = 0;
static opcxx_ListOfMetaclass* templateCreator = 0;
static Class* CreateClass(Ptree* def, Ptree* marg);
static Class* CreateTemplateClass(Ptree* def, Ptree* marg);

void Class::do_init_static()
{
    // Only do this once
    static bool done_init = false;
    if (done_init) return;
    done_init = true;

    class_t = new LeafReserved("class", 5);
    empty_block_t = new PtreeClassBody(new Leaf("{", 1),
          			     0,
          			     new Leaf("}", 1));
    public_t = new LeafPUBLIC("public", 6);
    protected_t = new LeafPROTECTED("protected", 9);
    private_t = new LeafPRIVATE("private", 7);
    virtual_t = new LeafVIRTUAL("virtual", 7);
    colon_t = new Leaf(":", 1);
    comma_t = new Leaf(",", 1);
    semicolon_t = new Leaf(";", 1);

    classCreator = new opcxx_ListOfMetaclass(
	    "Class", CreateClass, Class::Initialize, 0);

    templateCreator = new opcxx_ListOfMetaclass(
	    "TemplateClass", CreateTemplateClass,
	    TemplateClass::Initialize, 0);
}

// class Class

void Class::Construct(Environment* e, Ptree* name)
{
    Ptree* def;
    Encoding encode;

    encode.SimpleName(name);
    def = Ptree::List(name, 0, empty_block_t);
    def = new PtreeClassSpec(class_t, def, 0, encode.Get());

    full_definition = def;
    definition = def;
    class_environment = 0;
    member_list = 0;
    done_decl_translation = false;
    removed = false;
    changed_member_list = 0;
    appended_member_list = 0;
    appended_code = 0;
    new_base_classes = def->Third();
    new_class_specifier = 0;

    SetEnvironment(new Environment(e));
}

void Class::InitializeInstance(Ptree* def, Ptree*)
{
    full_definition = def;
    if(def->Car()->IsLeaf())
	definition = def;
    else
	definition = def->Cdr();	// if coming with a user keyword

    class_environment = 0;
    member_list = 0;

    if(class_list == 0)
	class_list = new ClassArray;

    class_list->Append(this);

    done_decl_translation = false;
    removed = false;
    changed_member_list = 0;
    appended_member_list = 0;
    appended_code = 0;
    new_base_classes = definition->Third();
    new_class_specifier = 0;
    new_class_name = 0;
}

Class::~Class() {}

// introspection

char* Class::MetaclassName()
{
    return "Class";
}

Ptree* Class::Comments()
{
  if (definition->IsA(Token::ntClassSpec))
	return ((PtreeClassSpec*)definition)->GetComments();
    return 0;
}

Ptree* Class::Name()
{
    return definition->Second();
}

Ptree* Class::BaseClasses()
{
    return definition->Third();
}

Ptree* Class::Members()
{
    return definition->Nth(3)->Second();
}

Class* Class::NthBaseClass(int n)
{
    Ptree* bases = definition->Third();
    while(bases != 0){
	bases = bases->Cdr();		// skip : or ,
	if(n-- == 0){
	    Ptree* base_class = bases->Car()->Last()->Car();
	    return class_environment->LookupClassMetaobject(base_class);
	}

	bases = bases->Cdr();
    }

    return 0;
}

bool Class::IsSubclassOf(Ptree* name)
{
    Ptree* bases = definition->Third();
    while(bases != 0){
	bases = bases->Cdr();		// skip : or ,
	Ptree* base_class = bases->Car()->Last()->Car();
	if(base_class->Eq(name))
	    return true;
	else{
	    Class* metaobject
		= class_environment->LookupClassMetaobject(base_class);
	    if(metaobject != 0 && metaobject->IsSubclassOf(name))
		return true;
	}

	bases = bases->Cdr();
    }

    return false;
}

bool Class::IsImmediateSubclassOf(Ptree* name)
{
    Ptree* bases = definition->Third();
    while(bases != 0){
	bases = bases->Cdr();		// skip : or ,
	Ptree* base_class = bases->Car()->Last()->Car();
	if(base_class->Eq(name))
	    return true;

	bases = bases->Cdr();
    }

    return false;
}

Ptree* Class::NthBaseClassName(int n)
{
    Ptree* bases = definition->Third();
    while(bases != 0){
	bases = bases->Cdr();		// skip : or ,
	if(n-- == 0)
	    return bases->Car()->Last()->Car();

	bases = bases->Cdr();
    }

    return 0;
}

bool Class::NthMember(int nth, Member& mem)
{
    MemberList* mlist = GetMemberList();
    if(mlist == 0 || nth >= mlist->Number())
	return false;

    mem.Set(this, mlist->Ref(nth)->declarator, nth);
    return true;
}

bool Class::LookupMember(Ptree* name)
{
    Member m;
    return LookupMember(name, m);
}

bool Class::LookupMember(Ptree* name, Member& mem, int index)
{
    MemberList* mlist = GetMemberList();
    if(mlist == 0)
	return false;

    int nth = mlist->Lookup(class_environment, name, index);
    if(nth < 0)
	return false;

    mem.Set(this, mlist->Ref(nth)->declarator, nth);
    return true;
}

bool Class::LookupMember(char* name)
{
    Member m;
    return LookupMember(name, m);
}

bool Class::LookupMember(char* name, Member& mem, int index)
{
    MemberList* mlist = GetMemberList();
    if(mlist == 0)
	return false;

    int nth = mlist->Lookup(class_environment, name, index);
    if(nth < 0)
	return false;

    mem.Set(this, mlist->Ref(nth)->declarator, nth);
    return true;
}

MemberList* Class::GetMemberList()
{
    if(member_list == 0){
	member_list = new MemberList;
	member_list->Make(this);
    }

    return member_list;
}

ClassArray& Class::AllClasses()
{
    return *class_list;
}

int Class::Subclasses(ClassArray& subclasses)
{
    return Subclasses(Name(), subclasses);
}

int Class::Subclasses(Ptree* name, ClassArray& subclasses)
{
    subclasses.Clear();
    if(class_list == 0)
	return 0;

    uint n = class_list->Number();
    for(uint i = 0; i < n; ++i){
	Class* c = class_list->Ref(i);
	if(c->IsSubclassOf(name))
	    subclasses.Append(c);
    }

    return subclasses.Number();
}

int Class::ImmediateSubclasses(ClassArray& subclasses)
{
    return ImmediateSubclasses(Name(), subclasses);
}

int Class::ImmediateSubclasses(Ptree* name, ClassArray& subclasses)
{
    subclasses.Clear();
    if(class_list == 0)
	return 0;

    uint n = class_list->Number();
    for(uint i = 0; i < n; ++i){
	Class* c = class_list->Ref(i);
	if(c->IsImmediateSubclassOf(name))
	    subclasses.Append(c);
    }

    return subclasses.Number();
}

int Class::InstancesOf(char* name, ClassArray& classes)
{
    classes.Clear();
    if(class_list == 0)
	return 0;

    uint n = class_list->Number();
    for(uint i = 0; i < n; ++i){
	Class* c = class_list->Ref(i);
	if(strcmp(name, c->MetaclassName()) == 0)
	    classes.Append(c);
    }

    return classes.Number();
}

Ptree* Class::NthMemberName(int nth)
{
    Member m;

    if(NthMember(nth, m))
	return m.Name();
    else
	return 0;
}

int Class::IsMember(Ptree* name)
{
    Member mem;
    if(LookupMember(name, mem, 0))
	return mem.Nth();
    else
	return -1;
}

bool Class::LookupMemberType(Ptree* name, TypeInfo& mem_type)
{
    return class_environment->Lookup(name, mem_type);
}


// translation of classes

void Class::TranslateClass(Environment* e)
{
}

/*
  The class specifier is inserted between the class keyword and
  the class name.  This is meaningless unless the compiler is MSVC++.
  e.g.
      class __declspec(dllexport) X { ... };
*/
void Class::AddClassSpecifier(Ptree* spec)
{
    new_class_specifier = spec;
}

void Class::ChangeName(Ptree* name)
{
    new_class_name = name;
}

void Class::ChangeBaseClasses(Ptree* list)
{
    CheckValidity("ChangeBaseClasses()");
    if(list->IsLeaf())
	list = Ptree::List(list);

    new_base_classes = list;	// list should include ':'
}

void Class::RemoveBaseClasses()
{
    CheckValidity("RemoveBaseClasses()");
    new_base_classes = 0;
}

void Class::AppendBaseClass(Class* c, int specifier, bool is_virtual)
{
    AppendBaseClass(c->Name(), specifier, is_virtual);
}

void Class::AppendBaseClass(char* name, int specifier, bool is_virtual)
{
    AppendBaseClass(new Leaf(name, strlen(name)), specifier, is_virtual);
}

void Class::AppendBaseClass(Ptree* name, int specifier, bool is_virtual)
{
    CheckValidity("AppendBaseClass()");

    Ptree* lf;
    switch(specifier){
    case Public :
	lf = public_t;
	break;
    case Protected :
	lf = protected_t;
	break;
    case Private :
	lf = private_t;
	break;
    default :
	MopErrorMessage("Class::AppendBaseClass()", "bad specifier");
	lf = 0;
	break;
    }

    Ptree* super = Ptree::List(lf, name);

    if(is_virtual)
	super = Ptree::Cons(virtual_t, super);

    if(new_base_classes == 0)
	new_base_classes = Ptree::List(colon_t, super);
    else
	new_base_classes = Ptree::Append(new_base_classes,
					 Ptree::List(comma_t, super));
}

void Class::ChangeMember(Member& m)
{
    CheckValidity("ChangeMember()");

    if(changed_member_list == 0)
	changed_member_list = new ChangedMemberList;

    changed_member_list->Append(&m, Undefined);
}

void Class::AppendMember(Member& m, int access)
{
    CheckValidity("AppendMember()");
    if(appended_member_list == 0)
	appended_member_list = new ChangedMemberList;

    appended_member_list->Append(&m, access);
}

void Class::AppendMember(Ptree* p)
{
    CheckValidity("AppendMember()");
    appended_code = Ptree::Snoc(appended_code, p);
}

void Class::RemoveMember(Member& m)
{
    CheckValidity("RemoveMember()");
    m.Remove();
    ChangeMember(m);
}

void Class::CheckValidity(char* name)
{
    if(done_decl_translation)
	MopWarningMessage2(name, " is available only in TranslateClass().");
}

// TranslateMemberFunction() is invoked only if the function is
// implemented out of the class declaration (not inlined.)

void Class::TranslateMemberFunction(Environment*, Member& m)
{
}

ChangedMemberList::Cmem* Class::GetChangedMember(Ptree* decl)
{
    if(changed_member_list == 0)
	return 0;
    else
	return changed_member_list->Lookup(decl);
}

// translation of expressions

/*
  init is either "= <expression>" or "( <expression> )".
*/
Ptree* Class::TranslateInitializer(Environment* env, Ptree*,
				   Ptree* init)
{
    if(init->Car()->Eq('('))
	return TranslateArguments(env, init);
    else{
	Ptree* exp = init->Second();
	Ptree* exp2 = TranslateExpression(env, exp);
	if(exp == exp2)
	    return init;
	else
	    return Ptree::List(init->Car(), exp2);
    }
}

Ptree* Class::TranslateNew(Environment* env, Ptree* header,
			   Ptree* op, Ptree* placement, Ptree* tname,
			   Ptree* arglist)
{
    Ptree* exp2;

    if(header != 0 && !header->Eq("::"))
	ErrorMessage(env, "unsupported user keyword: ", header, op);

    Ptree* tname2 = TranslateNewType(env, tname);
    if(arglist == 0)
	exp2 = Ptree::List(TranslateArguments(env, placement), tname2);
    else
	exp2 = Ptree::List(TranslateArguments(env, placement), tname2,
			   TranslateArguments(env, arglist));

    if(header == 0)
	return new PtreeNewExpr(op, exp2);
    else
	return new PtreeNewExpr(header, Ptree::Cons(op, exp2));
}

Ptree* Class::TranslateDelete(Environment* env, Ptree* op, Ptree* obj)
{
    Ptree* obj2 = TranslateExpression(env, obj);
    return new PtreeDeleteExpr(op, Ptree::List(obj2));
}

Ptree* Class::TranslateAssign(Environment* env, Ptree* left, Ptree* op,
			      Ptree* right)
{
    Ptree* left2 = TranslateExpression(env, left);
    Ptree* right2 = TranslateExpression(env, right);
    return new PtreeAssignExpr(left2, Ptree::List(op, right2));
}

Ptree* Class::TranslateBinary(Environment* env, Ptree* lexpr, Ptree* op,
			      Ptree* rexpr)
{
    return new PtreeInfixExpr(TranslateExpression(env, lexpr),
			Ptree::List(op, TranslateExpression(env, rexpr)));
}

Ptree* Class::TranslateUnary(Environment* env, Ptree* op, Ptree* object)
{
    return new PtreeUnaryExpr(op, Ptree::List(TranslateExpression(env,
								  object)));
}

Ptree* Class::TranslateSubscript(Environment* env, Ptree* object,
				 Ptree* index)
{
    Ptree* object2 = TranslateExpression(env, object);
    Ptree* exp = index->Second();
    Ptree* exp2 = TranslateExpression(env, exp);
    if(exp == exp2)
	return new PtreeArrayExpr(object2, index);
    else
	return new PtreeArrayExpr(object2,
				  Ptree::ShallowSubst(exp2, exp, index));
}

Ptree* Class::TranslatePostfix(Environment* env, Ptree* object,
			       Ptree* op)
{
    return new PtreePostfixExpr(TranslateExpression(env, object),
				Ptree::List(op));
}

/*
   TranslateFunctionCall() is for the overloaded function call operator ().
*/
Ptree* Class::TranslateFunctionCall(Environment* env, Ptree* object,
				    Ptree* arglist)
{
    return new PtreeFuncallExpr(TranslateExpression(env, object),
				TranslateArguments(env, arglist));
}

Ptree* Class::TranslateMemberCall(Environment* env, Ptree* object,
				  Ptree* op, Ptree* member, Ptree* arglist)
{
    Ptree* func;

    object = TranslateExpression(env, object);
    func = Ptree::List(op, member);
    if(op->Eq('.'))
	func = new PtreeDotMemberExpr(object, func);
    else
	func = new PtreeArrowMemberExpr(object, func);

    arglist = TranslateArguments(env, arglist);
    return new PtreeFuncallExpr(func, arglist);
}

Ptree* Class::TranslateMemberCall(Environment* env,
				  Ptree* member, Ptree* arglist)
{
    return new PtreeFuncallExpr(member, TranslateArguments(env, arglist));
}

Ptree* Class::TranslateMemberRead(Environment* env, Ptree* object,
				  Ptree* op, Ptree* member)
{
    object = TranslateExpression(env, object);
    Ptree* rest = Ptree::List(op, member);
    if(op->Eq('.'))
	return new PtreeDotMemberExpr(object, rest);
    else
	return new PtreeArrowMemberExpr(object, rest);
}

Ptree* Class::TranslateMemberRead(Environment*, Ptree* member)
{
    return member;
}

Ptree* Class::TranslateMemberWrite(Environment* env, Ptree* object,
				   Ptree* op, Ptree* member, Ptree* assign_op,
				   Ptree* expr)
{
    // Note: If this function is invoked, TranslateAssign() on the
    // member does not work.  Suppose that the expression is p->m = 3.
    // Although TranslateMemberWrite() is invoked on p's class,
    // TranslateAssign() is not invoked on m's class.  This is a sort
    // of bug, but I don't know how to fix.

    Ptree* left;
    object = TranslateExpression(env, object),
    left = Ptree::List(op, member);
    if(op->Eq('.'))
	left = new PtreeDotMemberExpr(object, left);
    else
	left = new PtreeArrowMemberExpr(object, left);

    expr = TranslateExpression(env, expr);
    return new PtreeAssignExpr(left, Ptree::List(assign_op, expr));
}

Ptree* Class::TranslateMemberWrite(Environment* env, Ptree* member,
				   Ptree* assign_op, Ptree* expr)
{
    return new PtreeAssignExpr(member,
			       Ptree::List(assign_op,
					   TranslateExpression(env, expr)));
}

Ptree* Class::TranslateUnaryOnMember(Environment* env, Ptree* unary_op,
				     Ptree* object, Ptree* access_op,
				     Ptree* member_name)
{
    Ptree* right;
    object = TranslateExpression(env, object),
    right = Ptree::List(access_op, member_name);
    if(access_op->Eq('.'))
	right = new PtreeDotMemberExpr(object, right);
    else
	right = new PtreeArrowMemberExpr(object, right);

    return new PtreeUnaryExpr(unary_op, Ptree::List(right));
}

Ptree* Class::TranslateUnaryOnMember(Environment*, Ptree* unary_op,
				     Ptree* member_name)
{
    return new PtreeUnaryExpr(unary_op, Ptree::List(member_name));
}

Ptree* Class::TranslatePostfixOnMember(Environment* env,
				       Ptree* object, Ptree* access_op,
				       Ptree* member_name, Ptree* postfix_op)
{
    Ptree* left;
    object = TranslateExpression(env, object),
    left = Ptree::List(access_op, member_name);
    if(access_op->Eq('.'))
	left = new PtreeDotMemberExpr(object, left);
    else
	left = new PtreeArrowMemberExpr(object, left);

    return new PtreePostfixExpr(left, Ptree::List(postfix_op));
}

Ptree* Class::TranslatePostfixOnMember(Environment*,
				       Ptree* member_name, Ptree* postfix_op)
{
    return new PtreePostfixExpr(member_name, Ptree::List(postfix_op));
}

Ptree* Class::TranslatePointer(Environment*, Ptree* var_name)
{
    return var_name;
}

Ptree* Class::TranslateUserStatement(Environment* env, Ptree*,
				     Ptree*,
				     Ptree* keyword, Ptree*)
{
    ErrorMessage(env, "unsupported user statement: ", keyword, keyword);
    return 0;
}

Ptree* Class::TranslateStaticUserStatement(Environment* env,
					   Ptree* keyword, Ptree*)
{
    ErrorMessage(env, "unsupported user statement: ", keyword, keyword);
    return 0;
}

Ptree* Class::StripClassQualifier(Ptree* qualified_name)
{
    if(qualified_name->IsLeaf())
	return qualified_name;
    else
	return Ptree::First(Ptree::Last(qualified_name));
}


// utilities for translation

Ptree* Class::TranslateExpression(Environment* env, Ptree* exp)
{
    if(exp == 0)
	return exp;
    else
	return env->GetWalker()->Translate(exp);
}

Ptree* Class::TranslateExpression(Environment* env, Ptree* exp,
				  TypeInfo& type)
{
    if(exp == 0){
	type.Unknown();
	return exp;
    }
    else{
	env->GetWalker()->Typeof(exp, type);
	return env->GetWalker()->Translate(exp);
    }
}

Ptree* Class::TranslateStatement(Environment* env, Ptree* exp)
{
    WarnObsoleteness("Class::TranslateStatement()",
		    "Class::TranslateExpression()");
    return TranslateExpression(env, exp);
}

Ptree* Class::TranslateNewType(Environment* env, Ptree* type)
{
    return env->GetWalker()->TranslateNew3(type);
}

Ptree* Class::TranslateArguments(Environment* env, Ptree* arglist)
{
    return env->GetWalker()->TranslateArguments(arglist);
}

Ptree* Class::TranslateFunctionBody(Environment* env, Member& m, Ptree* body)
{
    Walker* w = env->GetWalker();
    return w->RecordArgsAndTranslateFbody(this, m.ArgumentList(), body);
}

// others

void Class::SetEnvironment(Environment* e)
{
    class_environment = e;
    e->SetMetaobject(this);
}

// This metaobject cannot handle templates

bool Class::AcceptTemplate()
{
    return false;	// Only the subclasses of TemplateClass can
			// return true.
}

/*
  At the beginning, Initialize() is once called on each metaclass.
  The subclasses of Class may define their own Initialize().
  Initialize() returns false if the initialization fails.
*/
bool Class::Initialize()
{
    return true;
}

void Class::FinalizeAll(std::ostream& out)
{
    if(class_list == 0)
	return;

    int n = class_list->Number();
    for(int i = 0; i < n; ++i){
	Class* c = class_list->Ref(i);
	if(c != 0){
	    Ptree* p = c->FinalizeInstance();
	    if(p != 0){
		p->Write(out);
		out << '\n';
	    }
	}
    }
}

Ptree* Class::FinalizeInstance()
{
    return Finalize();
}

/*
  *Obsolete*

  Finalize() is called on each metaobject at the end.  The returned
  code is appended to the resulting file.  Note that Initialize()
  is called on each metaclass although Finalize() is on each metaobject.
*/
Ptree* Class::Finalize()
{
    return 0;
}

Ptree* Class::FinalizeClass()
{
    return 0;
}

void Class::RegisterNewModifier(char* str)
{
  throw std::runtime_error("sorry, registering new modifiers is not implemented");
//     if(!Lexer::RecordKeyword(str, UserKeyword))
// 	MopErrorMessage("Class::RegisterNewModifier()",
// 			"the keyword is already used.");
}

void Class::RegisterNewAccessSpecifier(char* str)
{
  throw std::runtime_error("sorry, registering new modifiers is not implemented");
//     if(!Lexer::RecordKeyword(str, UserKeyword4))
// 	MopErrorMessage("Class::RegisterNewAccessSpecifier()",
// 			"the keyword is already used.");
}

void Class::RegisterNewMemberModifier(char* str)
{
  throw std::runtime_error("sorry, registering new modifiers is not implemented");
//     if(!Lexer::RecordKeyword(str, UserKeyword5))
// 	MopErrorMessage("Class::RegisterNewMemberModifier()",
// 			"the keyword is already used.");
}

void Class::RegisterNewWhileStatement(char* str)
{
  throw std::runtime_error("sorry, registering new modifiers is not implemented");
//     if(!Lexer::RecordKeyword(str, UserKeyword))
// 	MopErrorMessage("Class::RegisterNewWhileStatement()",
// 			"the keyword is already used.");
}

void Class::RegisterNewForStatement(char* str)
{
  throw std::runtime_error("sorry, registering new modifiers is not implemented");
//     if(!Lexer::RecordKeyword(str, UserKeyword3))
// 	MopErrorMessage("Class::RegisterNewForStatement()",
// 			"the keyword is already used.");
}

void Class::RegisterNewClosureStatement(char* str)
{
  throw std::runtime_error("sorry, registering new modifiers is not implemented");
//     if(!Lexer::RecordKeyword(str, UserKeyword2))
// 	MopErrorMessage("Class::RegisterNewClosureStatement()",
// 			"the keyword is already used.");
}

void Class::RegisterMetaclass(char* keyword, char* class_name)
{
  throw std::runtime_error("sorry, registering new modifiers is not implemented");
//     if(Lexer::RecordKeyword(keyword, UserKeyword))
// 	if(Environment::RecordClasskeyword(keyword, class_name))
// 	    return;

//     MopErrorMessage("Class::RegisterMetaclass()",
// 		    "the keyword is already used.");
}

void Class::ChangeDefaultMetaclass(char* name)
{
    Walker::ChangeDefaultMetaclass(name);
}

void Class::SetMetaclassForFunctions(char* name)
{
    metaclass_for_c_functions = name;
}

void Class::InsertBeforeStatement(Environment* env, Ptree* p)
{
    Walker* w = env->GetWalker();
    if(w->IsClassWalker())
	((ClassWalker*)w)->InsertBeforeStatement(p);
    else
	MopWarningMessage("Class::InsertBeforeStatement()",
			  "cannot insert");
}

void Class::AppendAfterStatement(Environment* env, Ptree* p)
{
    Walker* w = env->GetWalker();
    if(w->IsClassWalker())
	((ClassWalker*)w)->AppendAfterStatement(p);
    else
	MopWarningMessage("Class::AppendAfterStatement()",
			  "cannot append");
}

void Class::InsertBeforeToplevel(Environment* env, Class* c)
{
    Walker* w = env->GetWalker();
    if(w->IsClassWalker())
	InsertBeforeToplevel(env, ((ClassWalker*)w)->ConstructClass(c));
    else
	MopWarningMessage("Class::InsertBeforeToplevel()",
			  "cannot insert");
}

void Class::InsertBeforeToplevel(Environment* env, Member& mem)
{
    Walker* w = env->GetWalker();
    if(w->IsClassWalker()){
	ChangedMemberList::Cmem cmem;
	Member::Copy(&mem, &cmem);
	InsertBeforeToplevel(env, ((ClassWalker*)w)->ConstructMember(&cmem));
    }
    else
	MopWarningMessage("Class::InsertBeforeToplevel()",
			  "cannot insert");
}

void Class::InsertBeforeToplevel(Environment* env, Ptree* p)
{
    Walker* w = env->GetWalker();
    if(w->IsClassWalker())
	((ClassWalker*)w)->InsertBeforeToplevel(p);
    else
	MopWarningMessage("Class::InsertBeforeToplevel()",
			  "cannot insert");
}

void Class::AppendAfterToplevel(Environment* env, Class* c)
{
    Walker* w = env->GetWalker();
    if(w->IsClassWalker())
	AppendAfterToplevel(env, ((ClassWalker*)w)->ConstructClass(c));
    else
	MopWarningMessage("Class::AppendAfterToplevel()",
			  "cannot insert");
}

void Class::AppendAfterToplevel(Environment* env, Member& mem)
{
    Walker* w = env->GetWalker();
    if(w->IsClassWalker()){
	ChangedMemberList::Cmem cmem;
	Member::Copy(&mem, &cmem);
	AppendAfterToplevel(env, ((ClassWalker*)w)->ConstructMember(&cmem));
    }
    else
	MopWarningMessage("Class::AppendAfterToplevel()",
			  "cannot insert");
}

void Class::AppendAfterToplevel(Environment* env, Ptree* p)
{
    Walker* w = env->GetWalker();
    if(w->IsClassWalker())
	((ClassWalker*)w)->AppendAfterToplevel(p);
    else
	MopWarningMessage("Class::AppendAfterToplevel()",
			  "cannot append");
}

bool Class::InsertDeclaration(Environment* env, Ptree* decl)
{
    return InsertDeclaration(env, decl, 0, 0);
}

bool Class::InsertDeclaration(Environment* env, Ptree* decl,
			      Ptree* key, void* client_data)
{
    Walker* w = env->GetWalker();
    if(w->IsClassWalker())
	return ((ClassWalker*)w)->InsertDeclaration(decl, this, key,
						    client_data);
    else{
	MopWarningMessage("Class::InsertDeclaration()",
			  "cannot insert");
	return false;
    }
}

void* Class::LookupClientData(Environment* env, Ptree* key)
{
    Walker* w = env->GetWalker();
    if(w->IsClassWalker())
	return ((ClassWalker*)w)->LookupClientData(this, key);
    else{
	MopWarningMessage("Class::LookupClientData()",
			  "cannot lookup");
	return 0;
    }
}

void Class::ErrorMessage(Environment* env, char* msg,
			 Ptree* name, Ptree* where)
{
    env->GetWalker()->ErrorMessage(msg, name, where);
}

void Class::WarningMessage(Environment* env, char* msg,
			   Ptree* name, Ptree* where)
{
    env->GetWalker()->WarningMessage(msg, name, where);
}

void Class::ErrorMessage(char* msg, Ptree* name, Ptree* where)
{
    Walker::InaccurateErrorMessage(msg, name, where);
}

void Class::WarningMessage(char* msg, Ptree* name, Ptree* where)
{
    Walker::InaccurateWarningMessage(msg, name, where);
}

bool Class::RecordCmdLineOption(char* key, char* value)
{
    if(num_of_cmd_options < MaxOptions * 2){
	cmd_options[num_of_cmd_options++] = key;
	cmd_options[num_of_cmd_options++] = value;
	return true;
    }
    else
	return false;
}

bool Class::LookupCmdLineOption(char* key)
{
    char* value;
    return LookupCmdLineOption(key, value);
}

bool Class::LookupCmdLineOption(char* key, char*& value)
{
    for(int i = 0; i < num_of_cmd_options; i += 2)
	if(strcmp(key, cmd_options[i]) == 0){
	    value = cmd_options[i + 1];
	    return true;
	}

    return false;
}

void Class::WarnObsoleteness(char* func, char* alt)
{
    MopWarningMessage2(func, " is obsolete.");
    MopMoreWarningMessage("use ", alt);
}


// class TemplateClass

void TemplateClass::InitializeInstance(Ptree* def, Ptree* margs)
{
    Class::InitializeInstance(GetClassInTemplate(def), margs);
    template_definition = def;
}

/*
  This is needed because TemplateClass may be instantiated for a class.
*/
Ptree* TemplateClass::GetClassInTemplate(Ptree* def)
{
    Ptree* decl = def->Ptree::Nth(4);
    if(decl == 0)
	return def;

    Ptree* cdef = Walker::GetClassTemplateSpec(decl);
    if(cdef == 0)
	return def;
    else
	return cdef;
}

bool TemplateClass::Initialize()
{
    return true;
}

char* TemplateClass::MetaclassName()
{
    return "TemplateClass";
}

Ptree* TemplateClass::TemplateArguments()
{
    return template_definition->Third();
}

bool TemplateClass::AcceptTemplate()
{
    return true;
}

/*
  Translate a template instantiation.
*/
Ptree* TemplateClass::TranslateInstantiation(Environment*, Ptree* spec)
{
    return spec;
}

// class ClassArray

ClassArray::ClassArray(int s)
{
    num = 0;
    if(s < 1)
	s = 1;

    size = s;
    array = new (GC) Class*[s];
}

void ClassArray::Append(Class* p)
{
    if(num >= size){
	size += 16;
	Class** a = new (GC) Class*[size];
	memmove(a, array, size_t(num * sizeof(Class*)));
	array = a;
    }

    array[num++] = p;
}

Class*& ClassArray::Ref(uint i)
{
    if(i < num)
	return array[i];
    else{
	MopErrorMessage("ClassArray", "out of range");
	return array[0];
    }
}


//
// class opcxx_ListOfMetaclass	--- not documented class
//

opcxx_ListOfMetaclass* opcxx_ListOfMetaclass::head = 0;

static Class* CreateClass(Ptree* def, Ptree* marg)
{
    Class* metaobject = new Class;
    metaobject->InitializeInstance(def, marg);
    return metaobject;
}

opcxx_ListOfMetaclass* opcxx_init_Class()
{
    return new opcxx_ListOfMetaclass("Class", CreateClass, Class::Initialize,
				     0);
}

static Class* CreateTemplateClass(Ptree* def, Ptree* marg)
{
    Class* metaobject = new TemplateClass;
    metaobject->InitializeInstance(def, marg);
    return metaobject;
}

opcxx_ListOfMetaclass* opcxx_init_TemplateClass()
{
    return new opcxx_ListOfMetaclass("TemplateClass", CreateTemplateClass,
				     TemplateClass::Initialize, 0);
}

opcxx_ListOfMetaclass::opcxx_ListOfMetaclass(char* n,
					     opcxx_MetaclassCreator c,
					     bool (*initialize)(),
					     Ptree* (*fin)())
{
    proc = c;
    name = n;
    if(AlreadyRecorded(n))
	next = 0;
    else{
	next = head;
	head = this;
	finalizer = fin;
	if(!initialize())
	    MopErrorMessage("Initialize()",
			    "the initialization process failed.");
    }
}

Class* opcxx_ListOfMetaclass::New(Ptree* name, Ptree* def, Ptree* marg)
{
    if(name != 0){
	opcxx_ListOfMetaclass* p = head;
	while(p != 0){
	    if(name->Eq(p->name))
		return (*p->proc)(def, marg);
	    else
		p = p->next;
	}
    }

    return 0;		// the metaclass is not loaded.
}

Class* opcxx_ListOfMetaclass::New(char* name, Ptree* def, Ptree* marg)
{
    if(name != 0){
	opcxx_ListOfMetaclass* p = head;
	while(p != 0){
	    if(strcmp(name, p->name) == 0)
		return (*p->proc)(def, marg);
	    else
		p = p->next;
	}
    }

    return 0;		// the metaclass is not loaded.
}

// FinalizeAll() calls all FinalizeClass()s.

void opcxx_ListOfMetaclass::FinalizeAll(std::ostream& out)
{
    for(opcxx_ListOfMetaclass* p = head; p != 0; p = p->next)
	if(p->finalizer != 0){
	    Ptree* code = (*p->finalizer)();
	    if(code != 0){
		code->Write(out);
		out << '\n';
	    }
	}
}

bool opcxx_ListOfMetaclass::AlreadyRecorded(char* name)
{
    for(opcxx_ListOfMetaclass* p = head; p != 0; p = p->next)
	if(strcmp(name, p->name) == 0)
	   return true;

    return false;
}

bool opcxx_ListOfMetaclass::AlreadyRecorded(Ptree* name)
{
    for(opcxx_ListOfMetaclass* p = head; p != 0; p = p->next)
	if(name->Eq(p->name))
	   return true;

    return false;
}

void opcxx_ListOfMetaclass::PrintAllMetaclasses()
{
    for(opcxx_ListOfMetaclass* p = head; p != 0; p = p->next)
	std::cout << p->name << '\n';
}

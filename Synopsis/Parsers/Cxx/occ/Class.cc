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
#include "PTree.hh"
#include <PTree/Writer.hh>
#include "Walker.hh"
#include "ClassWalker.hh"
#include "TypeInfo.hh"
#include "Encoding.hh"

ClassArray* Class::class_list = 0;
int Class::num_of_cmd_options = 0;
char* Class::cmd_options[];

char* Class::metaclass_for_c_functions = 0;
Class* Class::for_c_functions = 0;

PTree::Node *Class::class_t = 0;
PTree::Node *Class::empty_block_t = 0;
PTree::Node *Class::public_t = 0;
PTree::Node *Class::protected_t = 0;
PTree::Node *Class::private_t = 0;
PTree::Node *Class::virtual_t = 0;
PTree::Node *Class::colon_t = 0;
PTree::Node *Class::comma_t = 0;
PTree::Node *Class::semicolon_t = 0;

static opcxx_ListOfMetaclass* classCreator = 0;
static opcxx_ListOfMetaclass* templateCreator = 0;
static Class* CreateClass(PTree::Node *def, PTree::Node *marg);
static Class* CreateTemplateClass(PTree::Node *def, PTree::Node *marg);

void Class::do_init_static()
{
    // Only do this once
    static bool done_init = false;
    if (done_init) return;
    done_init = true;

    class_t = new PTree::Reserved("class", 5);
    empty_block_t = new PTree::ClassBody(new PTree::Atom("{", 1),
					 0,
					 new PTree::Atom("}", 1));
    public_t = new PTree::AtomPUBLIC("public", 6);
    protected_t = new PTree::AtomPROTECTED("protected", 9);
    private_t = new PTree::AtomPRIVATE("private", 7);
    virtual_t = new PTree::AtomVIRTUAL("virtual", 7);
    colon_t = new PTree::Atom(":", 1);
    comma_t = new PTree::Atom(",", 1);
    semicolon_t = new PTree::Atom(";", 1);

    classCreator = new opcxx_ListOfMetaclass(
	    "Class", CreateClass, Class::Initialize, 0);

    templateCreator = new opcxx_ListOfMetaclass(
	    "TemplateClass", CreateTemplateClass,
	    TemplateClass::Initialize, 0);
}

// class Class

void Class::Construct(Environment* e, PTree::Node *name)
{
    PTree::Node *def;
    Encoding encode;

    encode.SimpleName(name);
    def = PTree::list(name, 0, empty_block_t);
    def = new PTree::ClassSpec(class_t, def, 0, encode.Get());

    full_definition = def;
    definition = def;
    class_environment = 0;
    member_list = 0;
    done_decl_translation = false;
    removed = false;
    changed_member_list = 0;
    appended_member_list = 0;
    appended_code = 0;
    new_base_classes = PTree::third(def);
    new_class_specifier = 0;

    SetEnvironment(new Environment(e));
}

void Class::InitializeInstance(PTree::Node *def, PTree::Node *)
{
    full_definition = def;
    if(def->car()->is_atom())
	definition = def;
    else
	definition = def->cdr();	// if coming with a user keyword

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
    new_base_classes = PTree::third(definition);
    new_class_specifier = 0;
    new_class_name = 0;
}

Class::~Class() {}

// introspection

const char* Class::MetaclassName()
{
  return "Class";
}

PTree::Node *Class::Comments()
{
  if (PTree::is_a(definition, Token::ntClassSpec))
    return ((PTree::ClassSpec*)definition)->GetComments();
  return 0;
}

PTree::Node *Class::Name()
{
  return PTree::second(definition);
}

PTree::Node *Class::BaseClasses()
{
  return PTree::third(definition);
}

PTree::Node *Class::Members()
{
  return PTree::second(PTree::nth(definition, 3));
}

Class* Class::NthBaseClass(int n)
{
  PTree::Node *bases = PTree::third(definition);
  while(bases)
  {
    bases = bases->cdr();		// skip : or ,
    if(n-- == 0)
    {
      PTree::Node *base_class = PTree::last(bases->car())->car();
      return class_environment->LookupClassMetaobject(base_class);
    }
    bases = bases->cdr();
  }
  return 0;
}

bool Class::IsSubclassOf(PTree::Node *name)
{
  PTree::Node *bases = PTree::third(definition);
  while(bases)
  {
    bases = bases->cdr();		// skip : or ,
    PTree::Node *base_class = PTree::last(bases->car())->car();
    if(base_class && *base_class == *name) return true;
    else
    {
      Class *metaobject = class_environment->LookupClassMetaobject(base_class);
      if(metaobject != 0 && metaobject->IsSubclassOf(name)) return true;
    }
    bases = bases->cdr();
  }
  return false;
}

bool Class::IsImmediateSubclassOf(PTree::Node *name)
{
  PTree::Node *bases = PTree::third(definition);
  while(bases)
  {
    bases = bases->cdr();		// skip : or ,
    PTree::Node *base_class = PTree::last(bases->car())->car();
    if(base_class && *base_class == *name) return true;
    bases = bases->cdr();
  }
  return false;
}

PTree::Node *Class::NthBaseClassName(int n)
{
  PTree::Node *bases = PTree::third(definition);
  while(bases)
  {
    bases = bases->cdr();		// skip : or ,
    if(n-- == 0) return PTree::last(bases->car())->car();
    bases = bases->cdr();
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

bool Class::LookupMember(PTree::Node *name)
{
    Member m;
    return LookupMember(name, m);
}

bool Class::LookupMember(PTree::Node *name, Member& mem, int index)
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

int Class::Subclasses(PTree::Node *name, ClassArray& subclasses)
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

int Class::ImmediateSubclasses(PTree::Node *name, ClassArray& subclasses)
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

PTree::Node *Class::NthMemberName(int nth)
{
    Member m;

    if(NthMember(nth, m))
	return m.Name();
    else
	return 0;
}

int Class::IsMember(PTree::Node *name)
{
    Member mem;
    if(LookupMember(name, mem, 0))
	return mem.Nth();
    else
	return -1;
}

bool Class::LookupMemberType(PTree::Node *name, TypeInfo& mem_type)
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
void Class::AddClassSpecifier(PTree::Node *spec)
{
    new_class_specifier = spec;
}

void Class::ChangeName(PTree::Node *name)
{
    new_class_name = name;
}

void Class::ChangeBaseClasses(PTree::Node *list)
{
    CheckValidity("ChangeBaseClasses()");
    if(list->is_atom())
      list = PTree::list(list);

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
    AppendBaseClass(new PTree::Atom(name, strlen(name)), specifier, is_virtual);
}

void Class::AppendBaseClass(PTree::Node *name, int specifier, bool is_virtual)
{
    CheckValidity("AppendBaseClass()");

    PTree::Node *lf;
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

    PTree::Node *super = PTree::list(lf, name);

    if(is_virtual)
      super = PTree::cons(virtual_t, super);

    if(new_base_classes == 0)
      new_base_classes = PTree::list(colon_t, super);
    else
      new_base_classes = PTree::append(new_base_classes,
				       PTree::list(comma_t, super));
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

void Class::AppendMember(PTree::Node *p)
{
    CheckValidity("AppendMember()");
    appended_code = PTree::snoc(appended_code, p);
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

ChangedMemberList::Cmem* Class::GetChangedMember(PTree::Node *decl)
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
PTree::Node *Class::TranslateInitializer(Environment* env, PTree::Node *,
				   PTree::Node *init)
{
  if(*init->car() == '(') return TranslateArguments(env, init);
  else
  {
    PTree::Node *exp = PTree::second(init);
    PTree::Node *exp2 = TranslateExpression(env, exp);
    if(exp == exp2) return init;
    else return PTree::list(init->car(), exp2);
  }
}

PTree::Node *Class::TranslateNew(Environment* env, PTree::Node *header,
			   PTree::Node *op, PTree::Node *placement, PTree::Node *tname,
			   PTree::Node *arglist)
{
    PTree::Node *exp2;

    if(header && *header != "::")
	ErrorMessage(env, "unsupported user keyword: ", header, op);

    PTree::Node *tname2 = TranslateNewType(env, tname);
    if(arglist == 0)
	exp2 = PTree::list(TranslateArguments(env, placement), tname2);
    else
	exp2 = PTree::list(TranslateArguments(env, placement), tname2,
			   TranslateArguments(env, arglist));

    if(header == 0)
	return new PTree::NewExpr(op, exp2);
    else
	return new PTree::NewExpr(header, PTree::cons(op, exp2));
}

PTree::Node *Class::TranslateDelete(Environment* env, PTree::Node *op, PTree::Node *obj)
{
    PTree::Node *obj2 = TranslateExpression(env, obj);
    return new PTree::DeleteExpr(op, PTree::list(obj2));
}

PTree::Node *Class::TranslateAssign(Environment* env, PTree::Node *left, PTree::Node *op,
			      PTree::Node *right)
{
    PTree::Node *left2 = TranslateExpression(env, left);
    PTree::Node *right2 = TranslateExpression(env, right);
    return new PTree::AssignExpr(left2, PTree::list(op, right2));
}

PTree::Node *Class::TranslateBinary(Environment* env, PTree::Node *lexpr, PTree::Node *op,
			      PTree::Node *rexpr)
{
    return new PTree::InfixExpr(TranslateExpression(env, lexpr),
			PTree::list(op, TranslateExpression(env, rexpr)));
}

PTree::Node *Class::TranslateUnary(Environment* env, PTree::Node *op, PTree::Node *object)
{
    return new PTree::UnaryExpr(op, PTree::list(TranslateExpression(env,
								  object)));
}

PTree::Node *Class::TranslateSubscript(Environment* env, PTree::Node *object,
				 PTree::Node *index)
{
  PTree::Node *object2 = TranslateExpression(env, object);
  PTree::Node *exp = PTree::second(index);
  PTree::Node *exp2 = TranslateExpression(env, exp);
  if(exp == exp2)
    return new PTree::ArrayExpr(object2, index);
  else
    return new PTree::ArrayExpr(object2,
				PTree::shallow_subst(exp2, exp, index));
}

PTree::Node *Class::TranslatePostfix(Environment* env, PTree::Node *object,
			       PTree::Node *op)
{
    return new PTree::PostfixExpr(TranslateExpression(env, object),
				PTree::list(op));
}

/*
   TranslateFunctionCall() is for the overloaded function call operator ().
*/
PTree::Node *Class::TranslateFunctionCall(Environment* env, PTree::Node *object,
				    PTree::Node *arglist)
{
    return new PTree::FuncallExpr(TranslateExpression(env, object),
				TranslateArguments(env, arglist));
}

PTree::Node *Class::TranslateMemberCall(Environment* env, PTree::Node *object,
				  PTree::Node *op, PTree::Node *member, PTree::Node *arglist)
{
    PTree::Node *func;

    object = TranslateExpression(env, object);
    func = PTree::list(op, member);
    if(op && *op == '.')
	func = new PTree::DotMemberExpr(object, func);
    else
	func = new PTree::ArrowMemberExpr(object, func);

    arglist = TranslateArguments(env, arglist);
    return new PTree::FuncallExpr(func, arglist);
}

PTree::Node *Class::TranslateMemberCall(Environment* env,
				  PTree::Node *member, PTree::Node *arglist)
{
    return new PTree::FuncallExpr(member, TranslateArguments(env, arglist));
}

PTree::Node *Class::TranslateMemberRead(Environment* env, PTree::Node *object,
				  PTree::Node *op, PTree::Node *member)
{
    object = TranslateExpression(env, object);
    PTree::Node *rest = PTree::list(op, member);
    if(op && *op == '.')
	return new PTree::DotMemberExpr(object, rest);
    else
	return new PTree::ArrowMemberExpr(object, rest);
}

PTree::Node *Class::TranslateMemberRead(Environment*, PTree::Node *member)
{
    return member;
}

PTree::Node *Class::TranslateMemberWrite(Environment* env, PTree::Node *object,
				   PTree::Node *op, PTree::Node *member, PTree::Node *assign_op,
				   PTree::Node *expr)
{
    // Note: If this function is invoked, TranslateAssign() on the
    // member does not work.  Suppose that the expression is p->m = 3.
    // Although TranslateMemberWrite() is invoked on p's class,
    // TranslateAssign() is not invoked on m's class.  This is a sort
    // of bug, but I don't know how to fix.

    PTree::Node *left;
    object = TranslateExpression(env, object),
    left = PTree::list(op, member);
    if(op && *op == '.')
	left = new PTree::DotMemberExpr(object, left);
    else
	left = new PTree::ArrowMemberExpr(object, left);

    expr = TranslateExpression(env, expr);
    return new PTree::AssignExpr(left, PTree::list(assign_op, expr));
}

PTree::Node *Class::TranslateMemberWrite(Environment* env, PTree::Node *member,
				   PTree::Node *assign_op, PTree::Node *expr)
{
    return new PTree::AssignExpr(member,
			       PTree::list(assign_op,
					   TranslateExpression(env, expr)));
}

PTree::Node *Class::TranslateUnaryOnMember(Environment* env, PTree::Node *unary_op,
					   PTree::Node *object, PTree::Node *access_op,
					   PTree::Node *member_name)
{
  PTree::Node *right;
  object = TranslateExpression(env, object),
    right = PTree::list(access_op, member_name);
  if(access_op && *access_op == '.')
    right = new PTree::DotMemberExpr(object, right);
  else
    right = new PTree::ArrowMemberExpr(object, right);
  
  return new PTree::UnaryExpr(unary_op, PTree::list(right));
}

PTree::Node *Class::TranslateUnaryOnMember(Environment*, PTree::Node *unary_op,
					   PTree::Node *member_name)
{
  return new PTree::UnaryExpr(unary_op, PTree::list(member_name));
}

PTree::Node *Class::TranslatePostfixOnMember(Environment* env,
					     PTree::Node *object, PTree::Node *access_op,
					     PTree::Node *member_name, PTree::Node *postfix_op)
{
  PTree::Node *left;
  object = TranslateExpression(env, object),
    left = PTree::list(access_op, member_name);
  if(access_op && *access_op == '.')
    left = new PTree::DotMemberExpr(object, left);
  else
    left = new PTree::ArrowMemberExpr(object, left);

  return new PTree::PostfixExpr(left, PTree::list(postfix_op));
}

PTree::Node *Class::TranslatePostfixOnMember(Environment*,
				       PTree::Node *member_name, PTree::Node *postfix_op)
{
  return new PTree::PostfixExpr(member_name, PTree::list(postfix_op));
}

PTree::Node *Class::TranslatePointer(Environment*, PTree::Node *var_name)
{
  return var_name;
}

PTree::Node *Class::TranslateUserStatement(Environment* env, PTree::Node *,
					   PTree::Node *,
					   PTree::Node *keyword, PTree::Node *)
{
  ErrorMessage(env, "unsupported user statement: ", keyword, keyword);
  return 0;
}

PTree::Node *Class::TranslateStaticUserStatement(Environment* env,
						 PTree::Node *keyword, PTree::Node *)
{
  ErrorMessage(env, "unsupported user statement: ", keyword, keyword);
  return 0;
}

PTree::Node *Class::StripClassQualifier(PTree::Node *qualified_name)
{
  if(qualified_name->is_atom()) return qualified_name;
  else return PTree::first(PTree::last(qualified_name));
}


// utilities for translation

PTree::Node *Class::TranslateExpression(Environment* env, PTree::Node *exp)
{
    if(exp == 0)
	return exp;
    else
	return env->GetWalker()->Translate(exp);
}

PTree::Node *Class::TranslateExpression(Environment* env, PTree::Node *exp,
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

PTree::Node *Class::TranslateStatement(Environment* env, PTree::Node *exp)
{
    WarnObsoleteness("Class::TranslateStatement()",
		    "Class::TranslateExpression()");
    return TranslateExpression(env, exp);
}

PTree::Node *Class::TranslateNewType(Environment* env, PTree::Node *type)
{
    return env->GetWalker()->TranslateNew3(type);
}

PTree::Node *Class::TranslateArguments(Environment* env, PTree::Node *arglist)
{
    return env->GetWalker()->TranslateArguments(arglist);
}

PTree::Node *Class::TranslateFunctionBody(Environment* env, Member& m, PTree::Node *body)
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
	    PTree::Node *p = c->FinalizeInstance();
	    if(p != 0){
                PTree::Writer writer(out);
		writer.write(p);
		out << '\n';
	    }
	}
    }
}

PTree::Node *Class::FinalizeInstance()
{
    return Finalize();
}

/*
  *Obsolete*

  Finalize() is called on each metaobject at the end.  The returned
  code is appended to the resulting file.  Note that Initialize()
  is called on each metaclass although Finalize() is on each metaobject.
*/
PTree::Node *Class::Finalize()
{
    return 0;
}

PTree::Node *Class::FinalizeClass()
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

void Class::InsertBeforeStatement(Environment* env, PTree::Node *p)
{
    Walker* w = env->GetWalker();
    if(w->IsClassWalker())
	((ClassWalker*)w)->InsertBeforeStatement(p);
    else
	MopWarningMessage("Class::InsertBeforeStatement()",
			  "cannot insert");
}

void Class::AppendAfterStatement(Environment* env, PTree::Node *p)
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

void Class::InsertBeforeToplevel(Environment* env, PTree::Node *p)
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

void Class::AppendAfterToplevel(Environment* env, PTree::Node *p)
{
    Walker* w = env->GetWalker();
    if(w->IsClassWalker())
	((ClassWalker*)w)->AppendAfterToplevel(p);
    else
	MopWarningMessage("Class::AppendAfterToplevel()",
			  "cannot append");
}

bool Class::InsertDeclaration(Environment* env, PTree::Node *decl)
{
    return InsertDeclaration(env, decl, 0, 0);
}

bool Class::InsertDeclaration(Environment* env, PTree::Node *decl,
			      PTree::Node *key, void* client_data)
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

void* Class::LookupClientData(Environment* env, PTree::Node *key)
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
			 PTree::Node *name, PTree::Node *where)
{
    env->GetWalker()->ErrorMessage(msg, name, where);
}

void Class::WarningMessage(Environment* env, char* msg,
			   PTree::Node *name, PTree::Node *where)
{
    env->GetWalker()->WarningMessage(msg, name, where);
}

void Class::ErrorMessage(char* msg, PTree::Node *name, PTree::Node *where)
{
    Walker::InaccurateErrorMessage(msg, name, where);
}

void Class::WarningMessage(char* msg, PTree::Node *name, PTree::Node *where)
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

void TemplateClass::InitializeInstance(PTree::Node *def, PTree::Node *margs)
{
    Class::InitializeInstance(GetClassInTemplate(def), margs);
    template_definition = def;
}

/*
  This is needed because TemplateClass may be instantiated for a class.
*/
PTree::Node *TemplateClass::GetClassInTemplate(PTree::Node *def)
{
  PTree::Node *decl = PTree::nth(def, 4);
  if(!decl) return def;

  PTree::Node *cdef = Walker::GetClassTemplateSpec(decl);
  if(!cdef) return def;
  else return cdef;
}

bool TemplateClass::Initialize()
{
  return true;
}

const char* TemplateClass::MetaclassName()
{
  return "TemplateClass";
}

PTree::Node *TemplateClass::TemplateArguments()
{
  return PTree::third(template_definition);
}

bool TemplateClass::AcceptTemplate()
{
    return true;
}

/*
  Translate a template instantiation.
*/
PTree::Node *TemplateClass::TranslateInstantiation(Environment*, PTree::Node *spec)
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

static Class* CreateClass(PTree::Node *def, PTree::Node *marg)
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

static Class* CreateTemplateClass(PTree::Node *def, PTree::Node *marg)
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

opcxx_ListOfMetaclass::opcxx_ListOfMetaclass(const char* n,
					     opcxx_MetaclassCreator c,
					     bool (*initialize)(),
					     PTree::Node *(*fin)())
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

Class* opcxx_ListOfMetaclass::New(PTree::Node *name, PTree::Node *def, PTree::Node *marg)
{
    if(name != 0){
	opcxx_ListOfMetaclass* p = head;
	while(p != 0){
	    if(p->name && *name == *p->name)
		return (*p->proc)(def, marg);
	    else
		p = p->next;
	}
    }

    return 0;		// the metaclass is not loaded.
}

Class* opcxx_ListOfMetaclass::New(const char* name, PTree::Node *def, PTree::Node *marg)
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
	    PTree::Node *code = (*p->finalizer)();
	    if(code != 0){
                PTree::Writer writer(out);
		writer.write(code);
		out << '\n';
	    }
	}
}

bool opcxx_ListOfMetaclass::AlreadyRecorded(const char* name)
{
    for(opcxx_ListOfMetaclass* p = head; p != 0; p = p->next)
	if(strcmp(name, p->name) == 0)
	   return true;

    return false;
}

bool opcxx_ListOfMetaclass::AlreadyRecorded(PTree::Node *name)
{
    for(opcxx_ListOfMetaclass* p = head; p != 0; p = p->next)
	if(name && p->name && *name == *p->name)
	   return true;

    return false;
}

void opcxx_ListOfMetaclass::PrintAllMetaclasses()
{
    for(opcxx_ListOfMetaclass* p = head; p != 0; p = p->next)
	std::cout << p->name << '\n';
}

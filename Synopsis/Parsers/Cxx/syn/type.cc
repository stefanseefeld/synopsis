// File: type.cc

#include "type.hh"

using namespace Type;

//
// Type::Visitor
//

//Visitor::Visitor() {}
Visitor::~Visitor() {}
void Visitor::visitType(Type*) {}
void Visitor::visitForward(Forward *t) { visitType(t); }
void Visitor::visitBase(Forward *t) { visitType(t); }
void Visitor::viistDeclared(Declared *t) { visitType(t); }

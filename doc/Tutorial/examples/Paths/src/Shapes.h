#ifndef _Shapes_h
#define _Shapes_h

#include "Paths.h"

class Shape
{
public:
  virtual ~Shape() {}
  virtual void draw() = 0;
  virtual const Path &outline() = 0;
};

class Polygon : public Shape
{
public:
  Polygon();
  virtual ~Polygone() {}
  virtual void draw();
  virtual const Path &outline() { return _outline;}
private:
  Paths::Polyline _outline;
};

class Ellipse : public Shape
{
public:
  Ellipse();
  virtual ~Ellipse() {}
  virtual void draw();
  virtual const Path &outline() { return _outline;}
private:
  Paths::Nurbs _outline;
};

#endif

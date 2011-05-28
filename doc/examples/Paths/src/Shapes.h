#ifndef Shapes_h_
#define Shapes_h_

#include <Paths.h>

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
  virtual ~Polygon() {}
  virtual void draw();
  virtual const Path &outline() { return outline_;}
private:
  Paths::Polyline outline_;
};

class Ellipse : public Shape
{
public:
  Ellipse();
  virtual ~Ellipse() {}
  virtual void draw();
  virtual const Path &outline() { return outline_;}
private:
  Paths::Nurbs<2> outline_;
};

#endif

#ifndef _Path_h
#define _Path_h

//. A Vertex is a 2D point.
struct Vertex
{
  double x; //.< the x coordinate
  double y; //.< the y coordinate
};

//. Path is the basic abstraction
//. used for drawing (curved) paths.
class Path
{
public:
  virtual ~Path() {}
  //. Draw this path.
  virtual void draw() = 0;
  // temporarily commented out...
  // void intersects(const Path &);
private:
};

#endif

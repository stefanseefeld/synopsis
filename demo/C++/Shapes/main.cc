// vim: set ts=8 sts=2 sw=2 et:

/**
 * xref.c
 * Demo's the xref-output capability of the Synopsis C++ parser
 */

//. Forward declare a class pretending to be a drawing class
class Graphics;

//. Base class for the shape hierarchy. This simple base class has methods to
//. draw itself, and retrieve its 'hotspot' coordinates.
class Shape
{
  public:
    //. Constructor, initialises position
    Shape(int x, int y);

    //. Draw this shape.
    virtual void draw(Graphics*) = 0;

    //. Returns the x coordinate of the hotspot
    int x() { return m_x; }

    //. Returns the y coordinate of the hotspot
    int y() { return m_y; }

  private:
    //. The x,y coordinates of the hotspot
    int m_x, m_y;
};

//. A Circle drawing shape
class Circle : public Shape
{
  public:
    //. Constructor
    Circle(int x, int y)
      : Shape(x, y) { }
    //. Draw this circle
    void draw(Graphics*)
    {
      x() + y();
    }
};

//. A triangle drawing shape
class Triangle : public Shape
{
  public:
    //. Constructor
    Triangle(int x, int y)
      : Shape(x, y) { }
    //. Draw this triangle
    void draw(Graphics*)
    {
      int x = this.x();
      int some_y = y();
    }
};

//. Some function
void main()
{
  Circle* circle = new Circle(1,2);
  Triangle* tri = new Triangle(3,4);
  Shape* shape = circle;
  Graphics* g = NULL;
  shape->draw(g);

  tri->draw(g);

  int avg_x = circle->x() + triangle->y();
  int avg_y = circle->y() + triangle->y();

  avg_x /= 2;
  avg_y /= 2;
}


#ifndef Bezier_h_
#define Bezier_h_

#include "Path.h"
#include <vector>

namespace Paths
{

/**
 * The Bezier class. It implements a Bezier curve
 * for the given order.
 * (@see http://en.wikipedia.org/wiki/B%C3%A9zier_curve for more
 *  on Bezier curves.)
 */
template <size_t Order>
class Bezier : public Path
{
public:
  /** Create a new Bezier.*/
  Bezier();

  /** @group Manipulators {*/

  /**
   * Add a new control point.
   * @param p A point
   */
  void add_control_point(const Vertex &);

  /**
   * Remove the control point at index i.
   * @param i An index
   */
  void remove_control_point(size_t i);
  /** }*/
  virtual void draw();
private:
  /** The data...*/
  std::vector<Vertex> controls_;
};

}

#endif

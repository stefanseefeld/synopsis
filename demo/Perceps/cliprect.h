/*
	$Id: cliprect.h,v 1.1 2001/04/05 15:25:46 chalky Exp $

	------------------------------------------------------------------------
	ClanLib, the platform independent game SDK.

	This library is distributed under the GNU LIBRARY GENERAL PUBLIC LICENSE
	version 2. See COPYING for details.

	For a total list of contributers see CREDITS.

	------------------------------------------------------------------------
*/

//! clanDisplay="Display 2D"

#ifndef header_cliprect
#define header_cliprect

class CL_ClipRect
//: Clipping rectangle class in ClanLib.
// This class is used to define clipping regions when drawing graphics to the
// backbufffer. This is useful if it is too expensive to update the entire
// screen each time - instead you can define a limited area where everything
// outside the rectangle should be discarded.
// <br>
// The clipping rectangle is used together with the clipping rect functions
// in CL_Display and CL_DisplayCard.
//!also: CL_Display - The DisplayCard wrapper class.
//!also: CL_DisplayCard - The DisplayCard class.
{
public:
	int m_x1;
	//: Min x-coordinate of the rectangle.

	int m_y1;
	//: Min y-coordinate of the rectangle.

	int m_x2;
	//: Max x-coordinate of the rectangle - NOT included in clipping area

	int m_y2;
	//: Max y-coordinate of the rectangle - NOT included in clipping area

	CL_ClipRect();
	//: Constructs an uninitialized clip rectangle. (x1, y1) and (x2, y2) 
	//: contain random values, and should be manually initialized before 
	//: usage of the clip rect.

	CL_ClipRect(const CL_ClipRect &rect);
	//: Copy constructor.

	CL_ClipRect(int x1, int y1, int x2, int y2);
	//: Constructs a clipping rectangle from (x1,y1) to (x2,y2).
	//!param: (x1,y1) - Upper left corner of the rectangle.
	//!param: (x2,y2) - Lower right corner of the rectangle (not included)

	bool test_clipped(const CL_ClipRect &rect) const;
	//: Tests if the specified rectangle needs to be clipped with this clip 
	//: rect.
	//!param: rect - The rectangle to be tested.
	//!retval: True if the passed rectangle needs to be clipped.

	bool test_unclipped(const CL_ClipRect &rect) const;
	//: Tests whether the specified rectangle is entirely contained within
	//: this clip rect.
	//!param: rect - The rectangle to be tested.
	//!retval: True if the passed rectangle is contained within this rectangle.

	bool test_all_clipped(const CL_ClipRect &rect) const;
	//: Tests whether all of the specified rectangle is outside this rectangle.
	//!param: rect - The rectangle to be tested.
	//!retval: True if the entire specified rect is outside this rectangle.

	CL_ClipRect clip(const CL_ClipRect &rect) const;
	//: Clips the given rectangle and returns the result.
	//!param: rect - The rectangle to be clipped.
	//!retval: The clipped rectangle.
	
	bool operator == (const CL_ClipRect &rect) const;
	//: Standard C++ == operator.
	//!retval: True if specified rectangle equals this rectangle.

//	friend ostream& operator << (ostream &os, CL_ClipRect &rect);
};

#endif

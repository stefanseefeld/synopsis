/*
	$Id: display.h,v 1.1 2001/04/05 15:25:46 chalky Exp $

	------------------------------------------------------------------------
	ClanLib, the platform independent game SDK.

	This library is distributed under the GNU LIBRARY GENERAL PUBLIC LICENSE
	version 2. See COPYING for details.

	For a total list of contributers see CREDITS.

	------------------------------------------------------------------------
*/

//! clanDisplay="Display 2D"

#ifndef header_display
#define header_display

#include <vector>
class CL_Palette;
class CL_DisplayCard;
class CL_VidMode;
class CL_Target;
class CL_Surface;
class CL_Rect;

#include "cliprect.h"
#include "signals.h"

class CL_Display
//: Main Display class.
// <p>CL_Display is the "main" class when dealing with 2D graphics. It
// contains simple drawing operations, backbuffer clipping and access to the
// display cards available on the system.</p>
//
// <p>A large amount of the functions in the class have the same purpose as
// their name equalient in CL_DisplayCard. The difference is that those
// placed here operate on a selected display card, while those in
// CL_DisplayCard class require an instance to the card.</p>
//
// <p>All backbuffer related drawing operations in other classes (CL_Surface
// for instance) work on the currently selected display card, unless you
// explict pass a pointer pointing to a display card.</p>
//
// <p>The main purpose of this system is to avoid passing around a pointer
// to the display card in those games, where it only access one single
// card anyway.</p>
//
// <p>When using OpenGL the GL context will always point to the selected
// card. So if you need to use OpenGL commands to draw onto another card,
// you have to select it here.</p>
{
public:
	virtual ~CL_Display() { ; }

	static void flip_display(bool sync=false);
//: Flips the front and backbuffer. Everything is normally drawn to the 
//: backbuffer, and flip_display() needs to be called before it can be seen 
//: onto the screen.
//!param: sync - VSync on/off

	static void put_display(const CL_Rect &rect);
//: Copies the specified area of the backbuffer to the front buffer. This is
//: in particular useful if you only want to update a limited region, and not
//: do a full-blown flipping.

	static void sync_buffers();
//: Copies the contents of the frontbuffer to all other buffers (usually just
//: the backbuffer). This ensures that all buffers contain the same image.

	static void clear_display(float red=0, float green=0, float blue=0, float alpha=1);
//: Clears backbuffer with the specified color.
//!param: red - red component of the color.
//!param: green - green component of the color.
//!param: blue - blue component of the color.
//!param: alpha - alpha (transparency) component of the color.

	static void set_palette(CL_Palette *palette);
//: Set system palette on this card.
//: <br>
//: If the display card is in a non-palettelized mode, this will be the palette 
//: used when surfaceproviders doesn't specify a palette themself.
//!param: palette - Palette to use as new system palette.

	static CL_Palette *get_palette();
//: Return the current system palette on this card.
//!retval: The system palette.

	static void select_card(CL_DisplayCard *card);
//: Selects the display card used by the other memberfunctions of this class.
//!param: card - new selected display card.

	static CL_DisplayCard *get_current_card();
//: Returns the currently selected display card.
//!retval: Current display card.

	static void set_videomode(CL_VidMode *mode);
//: Change the display card's video mode.
//!param: mode - videomode to be changed to.
//!also: CL_VidMode - Video mode description class.

	static void set_videomode(
		int width,
		int height,
		int bpp,
		bool fullscreen = true,
		bool allow_resize = false,
		bool video_memory = true);
//: Change the display card's video mode.
//!param: width - width in pixels of the new video mode.
//!param: height - height in pixels of the new video mode.
//!param: bpp - Bits per pixel. The depth of the new video mode. (8, 16, 24, 32)
//!param: video_memory - Use video memory if possible. System memory may be faster if alpha blending is used a lot.

	static CL_Target *get_target();
//: Returns either NULL or the framebuffer
//!retval: NULL or the framebuffer

	static int get_width();
//: Returns the width of the current video mode.
//!retval: Width of current video mode.

	static int get_height();
//: Returns the height of the current video mode.
//!retval: Height of current video mode.

	static int get_bpp();
//: Returns the depth of the current video mode.
//!retval: Depth of current video mode.

	static std::vector<CL_DisplayCard*> cards;
//: The list of display cards available to the application.
	
	static void push_clip_rect();
//: Pushes the current clipping rectangle onto the cliprect stack.

	static void push_clip_rect(const CL_ClipRect &rect);
//: Pushes the current clipping rectangle onto the cliprect stack. It then clips 
//: the passed rectangle 'rect' with the current one, and uses the result as the 
//: new clipping rect.
//!param: rect - The new clipping rectangle to be with the old one and then used.

	static CL_ClipRect get_clip_rect();
//: Returns the current clipping rectangle.
//!retval: The current clipping rectangle.

	static void set_clip_rect(const CL_ClipRect &rect);
//: Sets the current clipping rectangle. This is an absolute set, so it doesn't
//: get clipped with the previous one.

	static void pop_clip_rect();
//: Pop the clipping rectangle last pushed onto the stack.

	static void push_translate_offset();
//: Pushes the current translation rectangle onto the cliprect stack.

	static void push_translate_offset(int x, int y);
//: Push translation offset onto translation stack. This offset will
//: affect any subsequent display operations on the current displaycard, by
//: translating the position of the display operation with the offset.
//: The offset will be offset by any previous offsets pushed onto the stack,
//: eg. it inherits the previous offset.

	static int  get_translate_offset_x();
//: Returns the current effective x-axis translation offset.

	static int  get_translate_offset_y();
//: Returns the current effective y-axis translation offset.

	static void set_translate_offset(int x, int y);
//: Sets the translation offset as a new absolute translation offset.
//: The new offset will disregard any previous offset's, but will not
//: empty the translation stack. The new translation offset will affect
//: any subsequent display operations on the current displaycard, by
//: translating the position of the display operation with the offset

	static void pop_translate_offset();
//: Pops the last pushed translation offset from the translation offset
//: stack. If the stack is empty, nothing will happen, and if the last
//: translation offset is popped, the translation offset will be set to 0,0

	static void draw_rect(int x1, int y1, int x2, int y2, float r, float g, float b, float a=1.0f);
//: Draw a rectangle from ('x1', 'y1') to ('x2', 'y2') using the color
//: ('r', 'g', 'b', 'a').
//!param: x1 - Leftmost x-coordinate.
//!param: y1 - Upper y-coordinate.
//!param: x2 - Rightmost x-coordinate.
//!param: y2 - Lower y-coordinate.
//!param: r - Red component of the filled color.
//!param: g - Green component of the filled color.
//!param: b - Blue component of the filled color.
//!param: a - Alpha component of the filled color.

	static void fill_rect(int x1, int y1, int x2, int y2, float r, float g, float b, float a=1.0f);
//: Draw a filled rectangle from ('x1', 'y1') to ('x2', 'y2') using the color
//: ('r', 'g', 'b', 'a').
//!param: x1 - Leftmost x-coordinate.
//!param: y1 - Upper y-coordinate.
//!param: x2 - Rightmost x-coordinate.
//!param: y2 - Lower y-coordinate.
//!param: r - Red component of the filled color.
//!param: g - Green component of the filled color.
//!param: b - Blue component of the filled color.
//!param: a - Alpha component of the filled color.

	static void draw_line(int x1, int y1, int x2, int y2, float r, float g, float b, float a=1.0f);
//: Draw a line from ('x1', 'y1') to ('x2', 'y2') using the color
//: ('r', 'g', 'b', 'a').
//!param: x1 - Leftmost x-coordinate. //FIXME
//!param: y1 - Upper y-coordinate.
//!param: x2 - Rightmost x-coordinate.
//!param: y2 - Lower y-coordinate.
//!param: r - Red component of the filled color.
//!param: g - Green component of the filled color.
//!param: b - Blue component of the filled color.
//!param: a - Alpha component of the filled color.
	
	static void fill_rect(int x1, int y1, int x2, int y2, CL_Surface *fill_surface, int focus_x=0, int focus_y=0);
//: Draw a filled rectangle from ('x1', 'y1') to ('x2', 'y2') using the surface 'fill_surface' as tiled background
//!param: x1 - Leftmost x-coordinate.
//!param: y1 - Upper y-coordinate.
//!param: x2 - Rightmost x-coordinate.
//!param: y2 - Lower y-coordinate.
//!param: fill_surface - surface used to fill the area (tiled)
//!param: focus_x - destination x offset used to offset (0, 0) in fill_surface (controls tiling position)
//!param: focus_y - destination y offset used to offset (0, 0) in fill_surface (controls tiling position)

	static CL_Signal_v2<int, int> &sig_resized();
//: Returns the resize signal for the currently selected display card.
// You can use this signal to listen for window resize events.
// The parameters passed by the signal are the new width and height of the window.

	static CL_Signal_v1<const CL_Rect &> &sig_paint();
//: Returns the paint signal for the currently selected display card.
// Use this signal to listen for invalidated screen areas that need to be repainted.
// The parameter passed by the signal is the area that need a repaint.
};

#endif

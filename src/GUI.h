#pragma once
#include <string>
// high-level code
// presumes this is included at the end of other declarations

// GUI context -- keep as integer-castable because
// you can use bitwise comparisons to check
// multiple contexts
enum {
  _hide_GUI = 0,
  _show_HUD = 1,
  _show_dashboard = 2,
  _show_pixel_ID = 4,
  _show_custom_msg = 8
};

const uint8_t hex_0_0_at_X = 107;
const uint8_t hex_0_0_at_Y =  99;

struct GUI_Object {
	int context;
	std::string HUD_footer;
	std::string custom_message;
	int value;
	int X;
	int Y;

	void draw() {
		
		if (context & _show_dashboard) {
      u8g2.setFont(u8g2_font_osr41_tn);
      u8g2.drawStr(_LEFT_MARGIN, 10, std::to_string(settings[_globlBrt].i).c_str());
      u8g2.setFont(u8g2_font_6x12_tr);
		}
		
		if (context & _show_HUD) {
			for (auto& b : hexBoard.btn) {
				int atX = hex_0_0_at_X + 2 * b.coord.x 
																	- (b.coord.x <= -10 ? 1 : 0);
				int atY = hex_0_0_at_Y + 3 * b.coord.y;
				u8g2.drawPixel(atX,atY);
				if (b.pressure) {
																u8g2.drawPixel(atX  ,atY-1);   // off low mid hi
																u8g2.drawPixel(atX  ,atY+1);   //      *   *  ***
				if (b.pressure >  64) { u8g2.drawPixel(atX-1,atY  );   //  *   *  *** ***
																u8g2.drawPixel(atX+1,atY  ); } //      *   *  ***
				if (b.pressure >  96) { u8g2.drawPixel(atX-1,atY-1);   //
																u8g2.drawPixel(atX-1,atY+1);   //
																u8g2.drawPixel(atX+1,atY-1);   //
																u8g2.drawPixel(atX+1,atY+1); } //          
				}
			}
      u8g2.setFont(u8g2_font_4x6_tr);
			u8g2.drawStr(_LEFT_MARGIN, 122, HUD_footer.c_str());
      u8g2.setFont(u8g2_font_6x12_tr);
		}

		
		if (context & _show_pixel_ID) {
			Hex selHex;
			selHex.x = settings[_anchorX].i;
			selHex.y = settings[_anchorY].i;
			if (hexBoard.in_bounds(selHex)) {
				u8g2.drawStr(_LEFT_MARGIN, 8, "Anchor hex: ");
				u8g2.drawStr(_LEFT_MARGIN + 84, 8, 
				  std::to_string(hexBoard.btn_by_coord.at(selHex)->pixel).c_str());
			} else {
				u8g2.drawStr(_LEFT_MARGIN, 8, "Press anchor hex >>");
			}
		}
		if (context & _show_custom_msg) {
			u8g2.drawStr(_LEFT_MARGIN, 18, custom_message.c_str());
		}
	}
};

GUI_Object GUI;
#pragma once
#include <stdint.h>
#include <map>
#include <array>
#include "config.h"

// a hexboard obviously needs to have a class of hexagon coordinates.
// https://www.redblobgames.com/grids/hexagons/ for more details.

struct Hex { 
	int x;      
	int y;
  Hex(int c[2]) : x(c[0]), y(c[1]) {}
	Hex(int x=0, int y=0) : x(x), y(y) {}
  // overload the = operator
  Hex& operator=(const Hex& rhs) {
		x = rhs.x;
		y = rhs.y;
		return *this;
	}
  // two hexes are == if their coordinates are ==
	bool operator==(const Hex& rhs) const {
		return (x == rhs.x && y == rhs.y);
	}
  // left-to-right, top-to-bottom order
  bool operator<(const Hex& rhs) const {
    if (y == rhs.y) {
      return (x < rhs.x);
    } else {
      return (y < rhs.y);
    }
  }
  // you can + two hexes by adding the coordinates
	Hex operator+(const Hex& rhs) const {
		return Hex(x + rhs.x, y + rhs.y);
	}
  // you can * a hex by a scalar to multi-step
	Hex operator*(const int& rhs) const {
		return Hex(rhs * x, rhs * y);
	}
  // subtraction is + hex*-1
  Hex operator-(const Hex& rhs) const {
        return *this + (rhs * -1);
    }
};
// dot product of two vectors (i.e. distance & # of musical steps per direction)
int dot_product(const Hex& A, const Hex& B) {
    return (A.x * B.x) + (A.y * B.y);
}

enum {
  //
  //                     | -y axis
  //              [-1,-1] [ 1,-1]
  //  -x axis [-2, 0] [ 0, 0] [ 2, 0]  +x axis
  //              [-1, 1] [ 1, 1]
  //                     | +y axis
  // keep this as a non-class enum because
  // we need to be able to cycle directions
	dir_e = 0,
	dir_ne = 1,
	dir_nw = 2,
	dir_w = 3,
	dir_sw = 4,
	dir_se = 5
};

Hex unitHex[] = {
  // E       NE      NW      W       SW      SE
  { 2, 0},{ 1,-1},{-1,-1},{-2, 0},{-1, 1},{ 1, 1}
};

struct axial_Hex {
  int a;
  int b;
  axial_Hex(const Hex& h, const Hex& axisA, const Hex& axisB) {
    if ((axisA == unitHex[dir_e]) || (axisA == unitHex[dir_w])) {
      b = h.y / axisB.y;
      a = (h.x - b * axisB.x) / axisA.x;
    } else if ((axisB == unitHex[dir_e]) || (axisB == unitHex[dir_w])) {
      a = h.y / axisA.y;
      b = (h.x - a * axisA.x) / axisB.x;
    } else {
      a = h.x / axisA.x;
      b = (h.y - a * axisA.y) / 2 / axisB.y;
      a += (h.y - a * axisA.y) / 2 / axisA.y;
    }
  }  
};

struct Hardwire_Switch {
  // hardware defined, static
  size_t  pinID;
  // volatile
  uint8_t state;
};

struct Physical_Button {
  size_t   pixel;               // associated pixel
  size_t   pinID;               // linear index of muxPin/colPin
  Hex      coord;               // physical location
  uint32_t timeLastUpdate = 0; // store time that key level was last updated
  uint32_t timePressBegan = 0; // store time that full press occurred
  uint32_t timeHeldSince  = 0;
  uint8_t  pressure       = 0; // press level currently
  uint8_t  velocity       = 0; // proxy for velocity
  bool     just_pressed   = false;
  bool     just_released  = false;
  void     * pxl_data_ptr = nullptr; // pointer to pixel color data
  void     * app_data_ptr = nullptr; // pointer to application data
  void update_levels(uint32_t& timestamp, uint8_t& new_level) {
    if (pressure == new_level) return;
    timeLastUpdate = timestamp;
    if (new_level == 0) {
      just_released = true;
      velocity = 0;
      timeHeldSince = 0;
    } else if (new_level >= 127) {
      just_pressed = true;
      velocity = 127;
      // velocity = function of timeLastUpdate - timePressBegan;
      // need a velocity curve, eventually. this is ignored in v1.2.
      timePressBegan = 0;
      timeHeldSince = timeLastUpdate;
    } else if (timePressBegan == 0) {
      timePressBegan = timeLastUpdate;
    }
    pressure = new_level;
  }

  bool check_and_reset_just_pressed() {
    bool result = just_pressed;
    just_pressed = false;
    return result;
  }
  
  bool check_and_reset_just_released() {
    bool result = just_released;
    just_released = false;
    return result;
  }

};

struct Button_Grid {
  std::array<Physical_Button, buttons_count>  btn;
  std::array<Hardwire_Switch, hardwire_count> dip;
  std::array<Physical_Button*, keys_count>    btn_at_index;
  std::array<Hardwire_Switch*, keys_count>    dip_at_index;
  std::map<Hex, Physical_Button*>             btn_by_coord;

  Button_Grid(const int def[buttons_count][4]) {
    for (auto& ptr : btn_at_index) {ptr = nullptr;}
    for (auto& ptr : dip_at_index) {ptr = nullptr;}

    for (size_t pxl = 0; pxl < buttons_count; ++pxl) {
      Hex x(def[pxl][0],def[pxl][1]);
      size_t i = linear_index(def[pxl][2], def[pxl][3]);
      btn[pxl].pixel = pxl;
      btn[pxl].pinID = i;
      btn[pxl].coord = x;
      btn_at_index[i] = &(btn[pxl]);
      btn_by_coord[x] = &(btn[pxl]);
    }
    size_t h = 0;
    for (size_t k = 0; k < keys_count; ++k) {
      if (btn_at_index[k] == nullptr) {
        dip[h].pinID = k;
        dip_at_index[k] = &(dip[h]);
        if (++h == hardwire_count) break;
      }
    }
  }
  void* appData_at_index(size_t i) {
    return btn_at_index[i]->app_data_ptr;
  }
  void* pxlData_at_index(size_t i) {
    return btn_at_index[i]->pxl_data_ptr;
  }
  bool in_bounds(const Hex& coord) {
    return (btn_by_coord.find(coord) != btn_by_coord.end());
  }
  void* appData_at_coord(Hex h) {
    if (in_bounds(h)) {
      return btn_by_coord.find(h)->second->app_data_ptr;
    }
    return nullptr;
  }
  void* pxlData_at_coord(Hex h) {
    if (in_bounds(h)) {   
      return btn_by_coord.find(h)->second->pxl_data_ptr;
    }
    return nullptr;
  }

};
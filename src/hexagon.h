#pragma once

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

//
//                     | -y axis
//              [-1,-1] [ 1,-1]
//  -x axis [-2, 0] [ 0, 0] [ 2, 0]  +x axis
//              [-1, 1] [ 1, 1]
//                     | +y axis
// keep this as a non-class enum because
// we need to be able to cycle directions
enum {
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
// handle changes due to mouse motion, or keys
#ifndef Input_hpp
#define Input_hpp

#include "Vec.hpp"
#include "View.hpp"

// only used by pointer or reference, don't need full include
struct GLFWwindow;
class View;
class Geometry;
struct Cylinder;

class Input {
// private data
private:
	int button, oldButton;		// which mouse button was pressed?
	double oldX, oldY;			// location of mouse at last event

	double updateTime;		 // time (in seconds) of last update
	Vec<3> moveRate;		 // for key change, motion in units/sec

	bool wireframe;				// draw polygons or outlines?

// public methods
public:
	// initialize
	Input();

	// handle mouse press / release, return true if redraw required
	bool mousePress(GLFWwindow &win, int button, int action);

	// handle mouse motion, return true if redraw required
	bool mouseMove(View &view, GLFWwindow &win, double x, double y);

	// handle key press/release, return true if redraw required
	bool keyPress(Geometry &geom, GLFWwindow &win, int key, int action, int mods);

	// update object position based on key input, return true if redraw required
	bool keyUpdate(Geometry &geom, GLFWwindow &win, View &view);
};

#endif

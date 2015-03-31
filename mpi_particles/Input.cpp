// handle mouse and keyboard input

#include "Input.hpp"
#include "View.hpp"
#include "Geometry.hpp"
#include "Generic.hpp"
#include "Cylinder.hpp"
#include "Xform.hpp"
#include "Mat.inl"
#include "Vec.inl"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#ifndef F_PI
#define F_PI 3.1415926f
#endif

// initialize
Input::Input() {
	button = oldButton = -1;
	oldX = oldY = 0;
	wireframe = false;
	moveRate = Vec3(0,0,0);
}

// called when a mouse button is pressed.
// Remember where we were, and what mouse button it was.
bool Input::mousePress(GLFWwindow &win, int b, int action) {
	if (action == GLFW_PRESS) {
		// hide cursor, record button
		glfwSetInputMode(&win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		button = b;
	}
	else {
		// display cursor, update button state
		glfwSetInputMode(&win, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		button = -1;	   // no button
	}
	return false;				// no redraw yet
}

// called when the mouse moves
// use difference between oldX,oldY and x,y to define a rotation
bool Input::mouseMove(View &view, GLFWwindow &win, double x, double y) {
	// only update view after at least one old position is stored
	bool redraw = false;
	if (button == GLFW_MOUSE_BUTTON_LEFT && button == oldButton) {
		// record differences & update last position
		float dx = float(x - oldX);
		float dy = float(y - oldY);

		// rotation angle, scaled so across the window = one rotation
		view.viewSph.x += float(F_PI * dx / view.width);
		view.viewSph.y += float(0.5f*F_PI * dy / view.height);
		view.update(&win);
		redraw = true;			// need to redraw
	}

	// update prior mouse state
	oldButton = button;
	oldX = x;
	oldY = y;
	return redraw;
}

// called when any key is pressed or released
bool Input::keyPress(Geometry &geom, GLFWwindow &win, int key, 
					 int action, int mods) 
{
	if (key=='W' || key=='A' || key=='S' || key=='D') {
		// rotation keys, respond to both GLFW_PRESS and GLFW_RELEASE
		if (key=='A')			// move left (in units/second)
			moveRate.x = (action==GLFW_RELEASE) ? 0.f : -20.f;
		else if (key=='D')		// move right (in units/second)
			moveRate.x = (action==GLFW_RELEASE) ? 0.f :	 20.f;
		else if (key=='W')		// move forward (in units/second)
			moveRate.y = (action==GLFW_RELEASE) ? 0.f :	 20.f;
		else if (key=='S')		// move back (in units/second)
			moveRate.y = (action==GLFW_RELEASE) ? 0.f : -20.f;

		updateTime = glfwGetTime();	// record time of keypress
		return true;					// need to redraw
	}

	if (action == GLFW_PRESS && key=='R') {
		geom.updateShaders();	// reload shaders
		return true;			// need to redraw
	}

	if (action == GLFW_PRESS && key=='L') { // toggle wireframe ('L'ines)
		wireframe = !wireframe;
		glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
		return true;			// need to redraw
	}

	if (action==GLFW_PRESS && key==GLFW_KEY_ESCAPE)
		glfwSetWindowShouldClose(&win, true); // Escape: exit

	return false;				// no redraw
}

// update view if necessary based on a/d keys
bool Input::keyUpdate(Geometry &geom, GLFWwindow &win) {
	if (length(moveRate) > 0) {
		// elapsed time since last update
		double now = glfwGetTime();
		//float dt = float(now - updateTime);
		updateTime = now;

		// move model based on time elapsed since last update
		// ensures uniform rate of change
		//Xform &xform = geom.getModelUniforms(drawId_to_move).modelMats;
		//xform = xform * Xform::translate(moveRate * dt);
		return true;			// need to redraw
	}
	return false;				// no redraw
}

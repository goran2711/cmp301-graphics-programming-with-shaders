// Input class
// Functions for retrieving input events/state.
#include "Input.h"

Input::Input()
	:	currentKeys(256, false)
	,	previousKeys(256, false)
{
}

void Input::update()
{
	previousKeys = currentKeys;
}

void Input::setMouseActive(bool active)
{
	mouse.isActive = active;
}

void Input::setMouseX(int xPosition)
{
	mouse.x = xPosition;
}

void Input::setMouseY(int yPosition)
{
	mouse.y = yPosition;
}

void Input::setLeftMouse(bool down)
{
	mouse.left = down;
}

void Input::setRightMouse(bool down)
{
	mouse.right = down;
}

void Input::setKeyDown(WPARAM key)
{
	currentKeys[key] = true;
}

void Input::setKeyUp(WPARAM key)
{
	currentKeys[key] = false;
}

bool Input::isMouseActive() const
{
	return mouse.isActive;
}

int Input::getMouseX() const
{
	return mouse.x;
}

int Input::getMouseY() const
{
	return mouse.y;
}

bool Input::isLeftMouseDown() const
{
	return mouse.left;
}

bool Input::isRightMouseDown() const
{
	return mouse.right;
}

bool Input::isKeyDown(int key) const
{
	return currentKeys[key];
}

bool Input::isKeyPressed(int key) const
{
	return currentKeys[key] == true && currentKeys[key] != previousKeys[key];
}

bool Input::isKeyReleased(int key) const
{
	return currentKeys[key] == false && currentKeys[key] != previousKeys[key];
}
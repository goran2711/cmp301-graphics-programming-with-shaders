// Input class
// Stores keyboard and mouse input.

#ifndef INPUT_H
#define INPUT_H

#include <vector>
#include <Windows.h>

class Input
{
	struct Mouse
	{
		int x, y;
		bool left, right, isActive;
	};

public:
	Input();

	void update();

	void setMouseActive(bool active);
	void setMouseX(int xPosition);
	void setMouseY(int yPosition);
	void setLeftMouse(bool down);
	void setRightMouse(bool down);
	void setKeyDown(WPARAM key);
	void setKeyUp(WPARAM key);

	bool isMouseActive() const;
	int getMouseX() const;
	int getMouseY() const;
	bool isLeftMouseDown() const;
	bool isRightMouseDown() const;
	bool isKeyDown(int key) const;
	bool isKeyPressed(int key) const;
	bool isKeyReleased(int key) const;

private:
	Mouse mouse;

	std::vector<bool> currentKeys;
	std::vector<bool> previousKeys;
};

#endif
#ifndef UI_H
#define UI_H

#include "vmath.h"

#include <vector>
#include <string>

class UIElement
{
public:
	UIElement(Vector4f pos, int type, int group);

	Vector4f pos; //4-vector coords of top-left and bottom right corner (in theory...)

	float red, gre, blu, alpha; //3-float colors and 1 float alpha
	
	int type; //button type. 0= unclick-able object (eg: panels), 1= clickable button, 2= toggleable button, 3= temporary notification
	int state; //the state of the button. typically 0= off, 1= on, -1= disabled
	std::string text; //text contained in the (top) of the UI Element. Also used as the Name of the element

	int group; //element group ID. When multiple buttons share a group ID, they all toggle off when one of them is turned on
	UIElement panel; //element's panel element. When buttons are assigned to a panel, other buttons can make them appear/disappear together

	void setState(int set);

	bool isClicked(Vector2f mousepos); //bool method for if button was clicked by mouse or not

};

#endif // UI_H
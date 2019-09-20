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
	
	int type; //button type. 0= unclick-able object (eg: panels), 1= clickable button, 2= toggleable button, 3= 
	int state; //for butons with requirements for long-period tracking of state, this exists
	std::string text; //text contained in the (top) of the UI Element

	int group; //element group ID. When multiple buttons share a group ID, they all toggle off when one of them is turned on
	UIElement panel; //element panel element. When buttons are assigned to a panel, other buttons can make them appear/disappear together

	bool isClicked(Vector2f mousepos); //bool method for if button was clicked or not

};

#endif // UI_H
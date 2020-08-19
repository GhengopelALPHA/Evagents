#include "UIElement.h"

#include "settings.h"


using namespace std;

UIElement::UIElement(int x, int y, int w, int h, std::string key, std::string title, bool shown, bool clickable) :
	x(x), y(y), w(w), h(h), key(key), title(title), shown(shown), clickable(clickable) {}


void UIElement::moveElement(int x, int y)
{
	int parentoldx= this->x;
	int parentoldy= this->y;
	this->x= x;
	this->y= y;
	for(int i=0; i<this->children.size(); i++){ //when we move an Element, we want to move all children too
		int childoldx= this->children[i].x;
		int childoldy= this->children[i].y;
		this->children[i].moveElement(childoldx+(x-parentoldx), childoldy+(y-parentoldy));
		//this of course should chain down through any their children
	}
}


void UIElement::addChild(UIElement &child)
{
	if(child.x < this->x) child.x= this->x + UID::TILEMARGIN;
	if(child.y < this->y) child.x= this->y + UID::TILEMARGIN;
	if(child.x + child.w > this->x + this->w) child.x= this->x + this->w - UID::TILEMARGIN;
	if(child.y + child.h > this->y + this->h) child.y= this->y + this->h - UID::TILEMARGIN;
	this->children.push_back(child);
}


void UIElement::show()
{
	//show this Element and all children
	this->shown= true;
	for(int i=0; i<this->children.size(); i++){
		this->children[i].show();
	}
}


void UIElement::hide()
{
	//deactivate this Element and all children
	this->shown= false;
	for(int i=0; i<this->children.size(); i++){
		this->children[i].hide();
	}
}
#ifndef VIEW_H
#define VIEW_H

#include "Agent.h"
class View
{
public:
    virtual void drawAgent(const Agent &a, float x, float y, bool ghost = 0) = 0;
    virtual void drawCell(int x, int y, const float values[Layer::LAYERS]) = 0;
    virtual void drawData() = 0;
	virtual void drawStatic() = 0;

	virtual void trySaveWorld(bool autosave = false) = 0;
//	virtual void trySaveAgent() = 0;
//	virtual void tryLoadAgent() = 0;
};

#endif // VIEW_H

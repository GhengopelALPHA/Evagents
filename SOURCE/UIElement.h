#pragma once

#include "vmath.h"
#include <vector>
#include <string>

class UIElement
{
public:
    UIElement(int x = 0, int y = 0, int w = 10, int h = 10, std::string key = "", std::string title = "", bool shown = true, bool clickable = true);

    void moveElement(int x, int y);
    void addChild(UIElement& child);
    void show();
    void hide();

    int x, y; // Top-left point x, y
    int w, h; // Width, height

    std::string title;
    std::string key; // "unique" function identifier (can be shared, but... why?)

    std::vector<UIElement> children;

    // Status flags
    bool shown;     // Is the UI element shown or hidden?
    bool clickable; // Is the UI clickable? (still blocks clicks on elements above it / the world space)
};
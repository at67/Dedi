#pragma once

#include <raylib.h>
#include <raygui.h>

#include <string>


namespace Status
{
    void reset(Font& font);

    void addText(const std::string& text);
    void draw(const std::string& title, const Rectangle& bounds);
}
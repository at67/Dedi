#pragma once

#include <raylib.h>
#include <raygui.h>

#include <string>


namespace Status
{
    void reset(Font& font);

    void drawText(const std::string& text, int i, int c, int x, int y, const Color& color, int alignment=TEXT_ALIGN_LEFT);

    void addText(const std::string& text);
    void draw(const std::string& title, const Rectangle& bounds);
}
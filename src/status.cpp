#include <status.h>
#include <util.h>
#include <win.h>

#include <gui.h>

#include <algorithm>


namespace Status
{
    struct TextLine
    {
        int _id = 0;
        std::string _text;
    };


    static std::vector<TextLine> _status;

    static int _displayLines = 0;
    static int _scrollIndex = 0;
    static int _startIndex = 0;
    static int _endIndex = 0;
    static int _maxIndex = 0;
    static int _maxLines = 7;

    static int _charIndex = 0;
    static int _maxPixels = 0;
    static int _savedIndex = 0;

    static Rectangle _bounds;


    int getTextPixels(const std::string& text)
    {
        return int(MeasureTextEx(GuiGetFont(), text.c_str(), float(GuiGetFont().baseSize), float(GuiGetStyle(DEFAULT, TEXT_SPACING))).x);
    }

    void reset(Font& font)
    {
        _charIndex = 0;
        _maxPixels = 0;
        _savedIndex = 0;

        for(size_t i=0; i<_status.size(); i++)
        {
            int pixels = getTextPixels(_status[i]._text);
            if(pixels > _maxPixels)
            {
                _maxPixels = pixels;
                _savedIndex = int(i);
            }
        }
    }

    void drawText(const std::string& text, int i, int c, int x, int y, const Color& color, int alignment)
    {
        static char num[16];
        sprintf(num, "%d", _status[i]._id);
        int index = (c > text.size()) ? int(text.size()) : c;
        GuiDrawText(num, {float(x), float(y), _bounds.width - 30, float(GuiGetFont().baseSize)}, alignment, color);
        GuiDrawText(text.c_str() + index, {float(x + 35), float(y), _bounds.width - 75, float(GuiGetFont().baseSize)}, alignment, color);
    }

    void addText(const std::string& text)
    {
        if(text.size() == 0) return;

        _maxIndex++;
        _scrollIndex = _maxIndex;
        if(_maxIndex > _maxLines)
        {
            bool autoScroll = (_startIndex + _maxLines == _scrollIndex - 1);
            if(Gui::getMiscOptions(Gui::EnableStatusAutoScroll)  ||  (autoScroll  &&  _startIndex < _maxIndex - _maxLines)) _startIndex++;
            _scrollIndex = _maxIndex - _maxLines;
        }

        int pixels = getTextPixels(text);
        _status.push_back({int(_status.size()), text});
        if(pixels > _maxPixels)
        {
            _maxPixels = pixels;
            _savedIndex = int(_status.size()) - 1;
        }
    }

    void input()
    {
        Vector2 mousePoint = GetMousePosition();
        if(_maxIndex > _maxLines  &&  CheckCollisionPointRec(mousePoint, _bounds))
        {
            int wheel = int(GetMouseWheelMove());
            if(wheel)
            {
                _startIndex -= wheel;
                _startIndex = std::max(_startIndex, 0);
            }
        }
    }

    void update()
    {
        if(_maxIndex > _maxLines  &&  _startIndex > _maxIndex - _maxLines) _startIndex = _maxIndex - _maxLines;

        _endIndex = _startIndex + _maxLines;

        if(_endIndex > _maxIndex) _endIndex = _maxIndex;
    }

    void draw(const std::string& title, const Rectangle& bounds)
    {
        _bounds = bounds;
        _maxLines = int((_bounds.height - 15) / GuiGetStyle(DEFAULT, TEXT_SIZE));

        // Vertical scroll bar
        GuiSetStyle(SCROLLBAR, ARROWS_VISIBLE, 1);
        Rectangle scrollV = {bounds.x + bounds.width - 13, bounds.y + 1, 12, bounds.height - 14};
        if(_maxIndex > _maxLines) _startIndex = GuiScrollBar(scrollV, _startIndex, 0, _scrollIndex);
        else GuiScrollBar(scrollV, _startIndex, 0, _scrollIndex);

        // Horizontal scroll bar
        Rectangle scrollH = {bounds.x + 1, bounds.y + bounds.height - 13, bounds.width - 14, 12};
        int endCharIndex = int(_status[_savedIndex]._text.size()) - 80;
        _charIndex = GuiScrollBar(scrollH, _charIndex, 0, endCharIndex);

        input();
        update();

        // Panel
        GuiGroupBox(_bounds, title.c_str());

        // Text list
        for(int i=_startIndex; i<_endIndex; i++)
        {
            int y = (i - _startIndex) * GuiGetStyle(DEFAULT, TEXT_SIZE);
            drawText(_status[i]._text, i, _charIndex, int(_bounds.x + 10), int(_bounds.y + y + 5), GetColor(GuiGetStyle(LABEL, TEXT_COLOR_DISABLED)));
        }
    }
}

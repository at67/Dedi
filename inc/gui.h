#pragma once

#include <raylib.h>
#include <raygui.h>
#undef RAYGUI_IMPLEMENTATION
#include <dialog.h>

#include <ini.h>
#include <clip.h>

#include <access.h>

#include <cstdint>
#include <string>
#include <vector>
#include <functional>


#define MAX_CONFIG_TEXT      256
#define CUSTOM_GAME_CONTROL   11
#define NORMAL_GAME            0
#define CUSTOM_GAME            1


namespace Gui
{
    enum GuiType {CheckBox=0, TextBox, DropdownBox, Spinner};


    class GuiStateTimeout
    {
    public:
        GuiStateTimeout() : _timeout(120) {}
        GuiStateTimeout(uint64_t timeout) : _timeout(timeout) {}

        void update(bool& active)
        {
            if(_timeout == 0) return;

            if(GuiGetLocalState() == STATE_FOCUSED)
            {
                _ticks = 0;
            }
            else
            {
                if(_ticks++ >= _timeout)
                {
                    _ticks = 0;    
                    active = false;
                }
            }
        }

    private:
        uint64_t _ticks = 0;
        uint64_t _timeout = 0;
    };

    struct GuiEntry
    {
        const std::string _key;
        const std::string _name;
        const std::string _toolTip;
        const std::string _options;
        const std::string _default;
        const std::string _property;
        const GuiType _type = TextBox;
        const bool _disable = false;
        int _option = 0;
        bool _toggle = false;
        GuiState _state = STATE_NORMAL;
        GuiStateTimeout _timeout = 0;
        char _value[MAX_CONFIG_TEXT] = {0};
    };

    using GuiEntryFunc = std::function<bool (GuiEntry guiEntries[], int index)>;


    int getTextPixels(const std::string& text);

    bool setWorldSeed();

    void defaultSection(const std::string& section, GuiEntry configEntries[], int numEntries);
    void createSection(std::ofstream& outfile, const std::string& section, GuiEntry configEntries[], int numEntries, bool useDefaults=false);

    void defaultConfig();
    void defaultOptions();
    
    bool updateValues(INIReader& iniReader, const std::string& section, GuiEntry guiEntries[], int numEntries, const GuiEntryFunc& userFunc=nullptr);

    void handleTooltips(GuiEntry guiEntries[], int x, int y, int startEntry, int endEntry);

    bool loadConfig(const std::string& config);
    bool loadOptions(const std::string& options);

    bool saveConfig(const std::string& config);
    bool saveOptions(const std::string& options);

    void handle();

    void handleServer(bool render=true);
    void handleServerEntries();

    void handleConfig();
    void handleConfigEntries();
    void handleConfigTooltips();

    void handleSaves();
    void handleSavesEntries();

    void handleWorld();
    void handleWorldEntries();
    void handleWorldTooltips();

    void handleOptions();
    void handleOptionsEntries();
    void handleOptionsTooltips();

    GuiWindowFileDialogState& getFileDialogState();
    void handleFileDialog(const std::string& folder);

    bool initialise();

    bool initWorldProperties();
}
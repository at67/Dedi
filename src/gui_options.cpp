#include <ini.h>

#include <gui.h>
#include <util.h>

#include <fstream>


namespace Gui
{
    static INIReader _optionsIniReader;

    static GuiEntry _miscOptions[NumMiscEntries] =
    {
        {"EnableToolTips",         "Enable or Disable Tooltips",                 "", "true",  "", CheckBox, false, 0, false, STATE_NORMAL, 0, ""},
        {"EnableMenusTimeout",     "Enable or Disable Dropdown Box timeout",     "", "true",  "", CheckBox, false, 0, false, STATE_NORMAL, 0, ""},
        {"EnableStylesTimeout",    "Enable or Disable Styles list timeout",      "", "true",  "", CheckBox, false, 0, false, STATE_NORMAL, 0, ""},
        {"EnableStatusAutoScroll", "Enable or Disable Status panel auto scroll", "", "false", "", CheckBox, false, 0, false, STATE_NORMAL, 0, ""},
        {"EnableWorldGen",         "Enable or Disable WorldGen modification",    "", "false", "", CheckBox, false, 0, false, STATE_NORMAL, 0, ""}
    };


    bool getMiscOptions(MiscEntries entry) {return _miscOptions[entry]._toggle;}

    size_t getOptionsKeysAndValues(const std::string& section, std::vector<std::string>& keys, std::vector<std::string>& values)
    {
        _optionsIniReader.GetKeysAndValues(section, keys, values);
        return keys.size();
    }

    void defaultOptions()
    {
        defaultSection("Misc", _miscOptions, NumMiscEntries);
    }

    bool createOptions(const std::string& options, bool useDefaults=false)
    {
        std::ofstream outfile(options, std::ios::out);
        if(!outfile.is_open())
        {
            log(Util::FatalError, stderr, _f, _F, _L, "Failed to open '%s'", options.c_str());
            return false;
        }

        createSection(outfile, "Misc", _miscOptions, NumMiscEntries, useDefaults);

        return true;
    }

    bool updateOptions(void)
    {
        return updateValues(_optionsIniReader, "Misc", _miscOptions, NumMiscEntries);
    }

    bool loadOptions(const std::string& options)
    {
        const int kMaxRetries = 3;
        const uint64_t kRetrySleep = 1000;

        int retries = kMaxRetries;
        bool success = false;

        while(retries--  &&  !success)
        {
            _optionsIniReader.Parse(options);
            if(_optionsIniReader.ParseError() < 0)
            {
                log(Util::WarnError, stderr, _f, _F, _L, "Couldn't load %s file, creating default : retry %d", options.c_str(), kMaxRetries - retries);
                createOptions(options, true);
                Util::sleep_ms(kRetrySleep);
            }
            else
            {
                success = true;
            }
        }

        // Each sections key values
        success &= updateOptions();
        if(success)
        {
            log(Util::Success, stderr, _f, _F, _L, "Successfully loaded %s", options.c_str());
        }
        else
        {
            defaultOptions();
            log(Util::WarnError, stderr, _f, _F, _L, "Corrupt or incorrect options file : %s : restoring defaults!", options.c_str());
        }

        return success;
    }

    bool saveOptions(const std::string& options)
    {
        if(!createOptions(options))
        {
            log(Util::FatalError, stderr, _f, _F, _L, "Couldn't save %s file", options.c_str());
            return false;
        }

        log(Util::Success, stderr, _f, _F, _L, "Successfully saved %s", options.c_str());

        return true;
    }

    void handleOptionsEntries(GuiEntry optionsEntries[], int x, int y, int startEntry, int endEntry)
    {
        for(auto i=endEntry; i>=startEntry; i--)
        {
            switch(optionsEntries[i]._type)
            {
                case CheckBox:
                {
                    if(GuiCheckBox({float(x), float(y) + float(i-startEntry)*30, 20, 20}, optionsEntries[i]._key.c_str(), &optionsEntries[i]._toggle)) optionsEntries[i]._toggle = !optionsEntries[i]._toggle;
                    optionsEntries[i]._state = GuiGetLocalState();
                }
                break;

                default: break;
            }
        } 
    }

    void handleOptionsEntries()
    {
        handleOptionsEntries(_miscOptions, 20, 75, 0, NumMiscEntries - 1);
    }

    void handleOptionsTooltips()
    {
        handleTooltips(_miscOptions, 20, 75, 0, NumMiscEntries - 1);
    }

    void handleOptions()
    {
        GuiPanel({10.0f, 40.0f, 850.0f, 705.0f}, "Options");
    }
}

#include <ini.h>

#include <util.h>
#include <win.h>
#include <gui.h>

#include <fstream>


#define NUM_TEXT_BOX_COLS 4
#define NUM_TEXT_BOX_ROWS 8


namespace Gui
{
    static INIReader _configIniReader;

    // Dedi config
    static GuiEntry _dediConfig[NumDediEntries] =
    {
        {"InstallPath",     "Install path, (don't install Dedi into your Aska or Aska Dedicated Server folders!)",  "",      "c:/Dedi", "", TextBox, false, 0, false, STATE_NORMAL,   0, ""},
        {"StylesFolder",    "Styles folder, (you should never need to modify this)",                                "",      "styles",  "", TextBox, false, 0, false, STATE_NORMAL,   0, ""},
        {"SavesFolder",     "Saves folder, (you should never need to modify this)",                                 "",      "saves",   "", TextBox, false, 0, false, STATE_NORMAL,   0, ""},
        {"BackupFolder",    "Backup folder, (you should never need to modify this)",                                "",      "backup",  "", TextBox, false, 0, false, STATE_NORMAL,   0, ""},
        {"InitialStyle",    "Startup style, choose one from the Styles button list",                                "",      "Mono",    "", TextBox, false, 0, false, STATE_NORMAL,   0, ""},
        {"MaxArchiveSaves", "Maximum number of archive saves",                                                      "1;20",  "5",       "", Spinner, false, 0, false, STATE_NORMAL, 300, ""},
        {"MaxUsersSaves",   "Maximum number of users saves",                                                        "1;50",  "10",      "", Spinner, false, 0, false, STATE_NORMAL, 300, ""}
    };

    // Steam config
    static GuiEntry _steamConfig[NumSteamEntries] =
    {
        {"HKLMRegSubKey",  "Registry subkey for Steam path, (you should never need to modify this)", "", "SOFTWARE\\WOW6432Node\\Valve\\Steam\\",            "", TextBox, false, 0, false, STATE_NORMAL, 0, ""},
        {"HKLMRegValue",   "Registry value for Steam path, (you should never need to modify this)",  "", "InstallPath",                                      "", TextBox, false, 0, false, STATE_NORMAL, 0, ""},
        {"SteamApps",      "Steam apps folder, (you should never need to modify this)",              "", "steamapps",                                        "", TextBox, false, 0, false, STATE_NORMAL, 0, ""},
        {"AppManifest",    "Steam apps manifest, (you should never need to modify this)",            "", "appmanifest",                                      "", TextBox, false, 0, false, STATE_NORMAL, 0, ""},
        {"LibraryFolders", "Library folders, (you should never need to modify this)",                "", "libraryfolders.vdf",                               "", TextBox, false, 0, false, STATE_NORMAL, 0, ""},
        {"HTTPSteamSvr",   "Link to Steam server site, (you should never need to modify this)",      "", "https://steamcommunity.com/dev/managegameservers", "", TextBox, false, 0, false, STATE_NORMAL, 0, ""}
    };

    // SteamCmd config
    static GuiEntry _steamCmdConfig[NumSteamCmdEntries] =
    {
        {"PathSteamCmd", "SteamCmd folder in Dedi, (don't modify this unless you know what you are doing)",          "", "steam",           "", TextBox, false, 0, false, STATE_NORMAL, 0, ""},
        {"DoneSteamCmd", "SteamCmd ready, (don't modify this unless you know what you are doing)",                   "", "Steam>",          "", TextBox, false, 0, false, STATE_NORMAL, 0, ""},
        {"CoupSteamCmd", "SteamCmd success, (don't modify this unless you know what you are doing)",                 "", "Success!",        "", TextBox, false, 0, false, STATE_NORMAL, 0, ""},
        {"QuitSteamCmd", "SteamCmd's exit command, (don't modify this unless you know what you are doing)",          "", "quit",            "", TextBox, false, 0, false, STATE_NORMAL, 0, ""},
        {"AppUSteamCmd", "SteamCmd's update command, (don't modify this unless you know what you are doing)",        "", "app_update",      "", TextBox, false, 0, false, STATE_NORMAL, 0, ""},
        {"ExecSteamCmd", "SteamCmd's executable, (don't modify this unless you know what you are doing)",            "", "steamcmd.exe",    "", TextBox, false, 0, false, STATE_NORMAL, 0, ""},
        {"OpenSteamCmd", "SteamCmd anonymous login, (don't modify this unless you know what you are doing)",         "", "login anonymous", "", TextBox, false, 0, false, STATE_NORMAL, 0, ""},
        {"HTTPSteamCmd", "Link to SteamCmd zip archive", "", "https://steamcdn-a.akamaihd.net/client/installer/steamcmd.zip",               "", TextBox, false, 0, false, STATE_NORMAL, 0, ""}
    };

    // Aska config
    static GuiEntry _askaConfig[NumAskaEntries] =
    {
        {"AskaSvrBat",   "Name of Aska Server .bat file, (only modify this if the name ever changes)",       "", "AskaServer.bat",                        "", TextBox, false, 0, false, STATE_NORMAL, 0, ""},
        {"AskaSvrProps", "Name of Aska Server properties file, (only modify this if the name ever changes)", "", "server properties.txt",                 "", TextBox, false, 0, false, STATE_NORMAL, 0, ""},
        {"AskaSvrPath",  "Steam partial path to Aska server, (you should never need to modify this)",        "", "common/ASKA Dedicated Server",          "", TextBox, false, 0, false, STATE_NORMAL, 0, ""},
        {"AskaExePath",  "Steam partial path to Aska game, (you should never need to modify this)",          "", "common/ASKA",                           "", TextBox, false, 0, false, STATE_NORMAL, 0, ""},
        {"AskaSvrAppId", "Steam app ID for Aska Server, (you should never need to modify this)",             "", "3246670",                               "", TextBox, false, 0, false, STATE_NORMAL, 0, ""},
        {"AskaExeAppId", "Steam app ID for Aska Game, (you should never need to modify this)",               "", "1898300",                               "", TextBox, false, 0, false, STATE_NORMAL, 0, ""},
        {"AskaSavePath", "System partial path to Aska saves, (you should never need to modify this)",        "", "LocalLow/Sand Sailor Studio/Aska/data", "", TextBox, false, 0, false, STATE_NORMAL, 0, ""}
    };

    // Server config
    static GuiEntry _serverConfig[NumServerEntries] =
    {
        {"DediName",             "Name that identifies this server to Dedi", "",                          "",                "",                        TextBox,     false, 0, false, STATE_NORMAL,   0, ""},
        {"SaveId",               "<id> from savegame_<id>",                  "",                          "",                "save id",                 TextBox,     false, 0, false, STATE_NORMAL,   0, ""},
        {"DisplayName",          "Display name in session list",             "",                          "Default Session", "display name",            TextBox,     false, 0, false, STATE_NORMAL,   0, ""},
        {"ServerName",           "Server name in session list",              "",                          "My Aska Server",  "server name",             TextBox,     false, 0, false, STATE_NORMAL,   0, ""},
        {"Seed",                 "Seed of the save",                         "",                          "",                "seed",                    TextBox,     false, 0, false, STATE_NORMAL,   0, ""},
        {"Password",             "Session password, makes session private",  "",                          "",                "password",                TextBox,     false, 0, false, STATE_NORMAL,   0, ""},
        {"SteamGamePort",        "Steam gameplay port",                      "",                          "27015",           "steam game port",         TextBox,     false, 0, false, STATE_NORMAL,   0, ""},
        {"SteamQueryPort",       "Steam queries port",                       "",                          "27016",           "steam query port",        TextBox,     false, 0, false, STATE_NORMAL,   0, ""},
        {"AuthenticationToken",  "Steam server authentication token",        "",                          "",                "authentication token",    TextBox,     false, 0, false, STATE_NORMAL,   0, ""},
        {"Region",               "Session region", "default;asia;australia;canada east;europe;hong kong;india;japan;south america;south korea;turkey;united arab emirates;usa east;usa south central;usa west",
                                                                                                          "default",         "region",                  DropdownBox, false, 0, false, STATE_NORMAL, 300, ""},
        {"KeepServerWorldAlive", "Keeps world running after last player logs out", "false;true",          "false",           "keep server world alive", DropdownBox, false, 0, false, STATE_NORMAL, 300, ""},
        {"AutosaveStyle",        "Frequency of server and client auto saves", "every morning;disabled;every 5 minutes;every 10 minutes;every 15 minutes;every 20 minutes",
                                                                                                          "every morning",   "autosave style",          DropdownBox, false, 0, false, STATE_NORMAL, 300, ""},
        {"Mode",                 "Normal or custom game", "normal;custom",                                "normal",          "mode",                    DropdownBox, false, 0, false, STATE_NORMAL, 300, ""},
        {"TerrainAspect",        "Custom game, Terrain aspect", "smooth;normal;rocky",                    "normal",          "terrain aspect",          DropdownBox, true,  1, false, STATE_NORMAL, 300, ""},
        {"TerrainHeight",        "Custom game, Terrain height", "flat;normal;varied",                     "normal",          "terrain height",          DropdownBox, true,  1, false, STATE_NORMAL, 300, ""},
        {"StartingSeason",       "Custom game, Season to start in", "spring;summer;autumn;winter",        "spring",          "starting season",         DropdownBox, true,  0, false, STATE_NORMAL, 300, ""},
        {"YearLength",           "Custom game, Year length", "minimum;reduced;default;extended;maximum",  "default",         "year length",             DropdownBox, true,  2, false, STATE_NORMAL, 300, ""},
        {"Precipitation",        "Custom game, Weather setting, 0 (sunny) to 6 (soggy)", "0;1;2;3;4;5;6", "3",               "precipitation",           DropdownBox, true,  3, false, STATE_NORMAL, 300, ""},
        {"DayLength",            "Custom game, Day setting", "minimum;reduced;default;extended;maximum",  "default",         "day length",              DropdownBox, true,  2, false, STATE_NORMAL, 300, ""},
        {"StructureDecay",       "Custom game, Structure decay", "low;medium;high",                       "medium",          "structure decay",         DropdownBox, true,  1, false, STATE_NORMAL, 300, ""},
        {"InvasionDificulty",    "Custom game, Invasion difficulty", "off;easy;normal;viking",            "normal",          "invasion dificulty",      DropdownBox, true,  2, false, STATE_NORMAL, 300, ""},
        {"MonsterDensity",       "Custom game, Monster density", "off;low;medium;high",                   "medium",          "monster density",         DropdownBox, true,  2, false, STATE_NORMAL, 300, ""},
        {"MonsterPopulation",    "Custom game, Monster population", "low;medium;high",                    "medium",          "monster population",      DropdownBox, true,  1, false, STATE_NORMAL, 300, ""},
        {"WulfarPopulation",     "Custom game, Wulfar population", "low;medium;high",                     "medium",          "wulfar population",       DropdownBox, true,  1, false, STATE_NORMAL, 300, ""},
        {"HerbivorePopulation",  "Custom game, Herbivore population", "low;medium;high",                  "medium",          "herbivore population",    DropdownBox, true,  1, false, STATE_NORMAL, 300, ""},
        {"BearPopulation",       "Custom game, Bear population", "low;medium;high",                       "medium",          "bear population",         DropdownBox, true,  1, false, STATE_NORMAL, 300, ""},
    };


    std::string getDediConfig(DediEntries entry)         {return _dediConfig[entry]._value;    }
    std::string getSteamConfig(SteamEntries entry)       {return _steamConfig[entry]._value;   }
    std::string getSteamCmdConfig(SteamCmdEntries entry) {return _steamCmdConfig[entry]._value;}
    std::string getAskaConfig(AskaEntries entry)         {return _askaConfig[entry]._value;    }
    std::string getServerConfig(ServerEntries entry)     {return _serverConfig[entry]._value;  }

    void setServerConfig(ServerEntries entry, const std::string& value)
    {
        Util::strcpy(_serverConfig[entry]._value, value, MAX_CONFIG_TEXT, _F, _L);
    }

    size_t getConfigKeysAndValues(const std::string& section, std::vector<std::string>& keys, std::vector<std::string>& values)
    {
        _configIniReader.GetKeysAndValues(section, keys, values);
        return keys.size();
    }

    void defaultConfig()
    {
        defaultSection("Dedi",     _dediConfig,     NumDediEntries);
        defaultSection("Steam",    _steamConfig,    NumSteamEntries);
        defaultSection("SteamCmd", _steamCmdConfig, NumSteamCmdEntries);
        defaultSection("Aska",     _askaConfig,     NumAskaEntries);
        defaultSection("Server",   _serverConfig,   NumServerEntries);
    }

    bool createConfig(const std::string& config, bool useDefaults=false)
    {
        std::ofstream outfile(config, std::ios::out);
        if(!outfile.is_open())
        {
            log(Util::FatalError, stderr, _f, _F, _L, "Failed to open '%s'", config.c_str());
            return false;
        }

        createSection(outfile, "Dedi",     _dediConfig,     NumDediEntries,     useDefaults);
        createSection(outfile, "Steam",    _steamConfig,    NumSteamEntries,    useDefaults);
        createSection(outfile, "SteamCmd", _steamCmdConfig, NumSteamCmdEntries, useDefaults);
        createSection(outfile, "Aska",     _askaConfig,     NumAskaEntries,     useDefaults);
        createSection(outfile, "Server",   _serverConfig,   NumServerEntries,   useDefaults);

        return true;
    }

    bool updateConfig(void)
    {
        bool success = updateValues(_configIniReader, "Dedi",     _dediConfig,     NumDediEntries);
        success     &= updateValues(_configIniReader, "Steam",    _steamConfig,    NumSteamEntries);
        success     &= updateValues(_configIniReader, "SteamCmd", _steamCmdConfig, NumSteamCmdEntries);
        success     &= updateValues(_configIniReader, "Aska",     _askaConfig,     NumAskaEntries);
        success     &= updateValues(_configIniReader, "Server",   _serverConfig,   NumServerEntries);

        // Read saveid from server properties and save in config
        std::string saveid;
        if(readServerProp(getDediConfig(InstallPath) + "/" + getAskaConfig(AskaSvrProps), "saveid=", saveid))
        {
            setServerConfig(SaveId, saveid);
        }

        return success;
    }

    bool loadConfig(const std::string& config)
    {
        const int kMaxRetries = 3;
        const uint64_t kRetrySleep = 1000;

        int retries = kMaxRetries;
        bool success = false;

        while(retries--  &&  !success)
        {
            _configIniReader.Parse(config);
            if(_configIniReader.ParseError() < 0)
            {
                log(Util::WarnError, stderr, _f, _F, _L, "Couldn't load %s file, creating default : retry %d", config.c_str(), kMaxRetries - retries);
                createConfig(config, true);
                Util::sleep_ms(kRetrySleep);
            }
            else
            {
                success = true;
            }
        }

        // Each sections key values
        success &= updateConfig();
        if(success)
        {
            log(Util::Success, stderr, _f, _F, _L, "Successfully loaded %s", config.c_str());
        }
        else
        {
            defaultConfig();
            log(Util::WarnError, stderr, _f, _F, _L, "Corrupt or incorrect config file : %s : restoring defaults!", config.c_str());
        }

        return success;
    }

    bool saveConfig(const std::string& config)
    {
        if(!createConfig(config))
        {
            log(Util::FatalError, stderr, _f, _F, _L, "Couldn't save %s file", config.c_str());
            return false;
        }

        log(Util::Success, stderr, _f, _F, _L, "Successfully saved %s", config.c_str());

        return true;
    }

    bool saveServerProperties()
    {
        std::string props = _configIniReader.Get("Dedi", "InstallPath", "") + "/server properties.txt";
        std::ofstream outfile(props, std::ios::out);
        if(!outfile.is_open())
        {
            log(Util::FatalError, stderr, _f, _F, _L, "Failed to open '%s'", props.c_str());
            return false;
        }

        // Skip server name
        for(size_t i=1; i<NumServerEntries; i++)
        {
            outfile << _serverConfig[i]._property << " = " << _serverConfig[i]._value << std::endl;
        }

        log(Util::Success, stderr, _f, _F, _L, "Generated '%s'", props.c_str());

        return true;
    }

    void handleConfigEntries(GuiEntry configEntries[], int x, int y, int startEntry, int endEntry)
    {
        for(auto i=endEntry; i>=startEntry; i--)
        {
            // Disable custom game control's children in normal mode
            if(configEntries[i]._disable  &&  configEntries[CUSTOM_GAME_CONTROL]._option == NORMAL_GAME) GuiSetState(STATE_DISABLED);

            switch(configEntries[i]._type)
            {
                case TextBox:
                {
                    if(GuiTextBox({float(x), float(y) + float(i-startEntry)*30, 180, 25}, configEntries[i]._value, MAX_CONFIG_TEXT, configEntries[i]._toggle))
                    {
                        configEntries[i]._toggle = !configEntries[i]._toggle;
                    }
                    configEntries[i]._state = GuiGetLocalState();
                }
                break;

                case DropdownBox:
                {
                    if(configEntries[i]._toggle) GuiUnlock();
                    if(GuiDropdownBox({float(x), float(y) + float(i-startEntry)*30, 180, 25}, configEntries[i]._options.c_str(), &configEntries[i]._option, configEntries[i]._toggle, configEntries[i]._value))
                    {
                        configEntries[i]._toggle = !configEntries[i]._toggle;
                    }
                    if(configEntries[i]._toggle  &&  getMiscOptions(EnableMenusTimeout))
                    {
                        configEntries[i]._timeout.update(configEntries[i]._toggle);
                        (configEntries[i]._toggle) ? GuiLock() : GuiUnlock(); // lock out other controls
                    }
                    configEntries[i]._state = GuiGetLocalState();
                }
                break;

                // TODO: uses option as an integer value, a bit hacky but will do for now
                case Spinner:
                {
                    int min = 1, max = 1, itemCount = 0;
                    const char **items = GuiTextSplit(configEntries[i]._options.c_str(), ';', &itemCount, nullptr);
                    if(itemCount == 2)
                    {
                        min = std::stoi(items[0], nullptr, 10);
                        max = std::stoi(items[1], nullptr, 10);
                    }

                    if(GuiSpinner({float(x), float(y) + float(i-startEntry)*30, 180, 25}, "", &configEntries[i]._option, min, max, configEntries[i]._toggle))
                    {
                        configEntries[i]._toggle = !configEntries[i]._toggle;
                    }
                    Util::strcpy(configEntries[i]._value, std::to_string(configEntries[i]._option), MAX_CONFIG_TEXT, _F, _L);
                    configEntries[i]._state = GuiGetLocalState();
                }
                break;

                default: break;
            }

            GuiSetState(STATE_NORMAL);

#if !defined(_DEBUG)
            // Clipboard
            if(configEntries[i]._type == TextBox  &&  configEntries[i]._state == STATE_PRESSED)
            {
                if(IsKeyDown(KEY_LEFT_CONTROL))
                {
                    if(IsKeyDown(KEY_C)) clip::set_text(configEntries[i]._value);

                    if(IsKeyDown(KEY_V))
                    {
                        std::string ctrlv;
                        clip::get_text(ctrlv);
                        Util::strcpy(configEntries[i]._value, ctrlv, MAX_CONFIG_TEXT, _F, _L);
                    }
                }
            }
#endif
        } 
    }

    void getServerLayout(int i, int& start, int& end)
    {
        start = i*NUM_TEXT_BOX_ROWS;
        end = start + NUM_TEXT_BOX_ROWS - 1; 
        end = (end >= NumServerEntries) ? NumServerEntries - 1 : end;
    }

    void handleConfigEntries()
    {
        // Textboxes and dropdownboxes
        handleConfigEntries(_dediConfig,      30,  85,  0,  NumDediEntries - 1);
        handleConfigEntries(_steamConfig,    240,  85,  0,  NumSteamEntries - 1);
        handleConfigEntries(_steamCmdConfig, 450,  85,  0,  NumSteamCmdEntries - 1);
        handleConfigEntries(_askaConfig,     660,  85,  0,  NumAskaEntries - 1);
        int start, end;
        for(int i=0; i<NUM_TEXT_BOX_COLS; i++)
        {
            getServerLayout(i, start, end);
            handleConfigEntries(_serverConfig, 30 + i*210, 350, start, end);
        }
    }

    void handleConfigTooltips()
    {
        handleTooltips(_dediConfig,      30,  85, 0, NumDediEntries - 1);
        handleTooltips(_steamConfig,    240,  85, 0, NumSteamEntries - 1);
        handleTooltips(_steamCmdConfig, 450,  85, 0, NumSteamCmdEntries - 1);
        handleTooltips(_askaConfig,     660,  85, 0, NumAskaEntries - 1);
        int start, end;
        for(int i=0; i<NUM_TEXT_BOX_COLS; i++)
        {
            getServerLayout(i, start, end);
            handleTooltips(_serverConfig, 30 + i*210, 350, start, end);
        }
    }

    void handleGetToken()
    {
        static bool getToken = false;
        if(GuiButton({290, 755, 90, 20}, "Get Token")) getToken = !getToken;
        if(getToken)
        {
            getToken = false;
            Win::shellExecute(getSteamConfig(HTTPSteamSvr));
        }
    }

    void handleGenerate()
    {
        static bool generate = false;
        if(GuiButton({390, 755, 90, 20}, "Generate")) generate = !generate;
        if(generate)
        {
            generate = false;
            saveServerProperties();
        }
    }

    void handleConfigButtons()
    {
        handleGetToken();
        handleGenerate();
    }

    void handleConfig()
    {
        GuiPanel({10.0f, 40.0f, 850.0f, 705.0f}, "Config");
        GuiGroupBox({20.0f,   75.0f, 200.0f, 255.0f}, "Dedi");
        GuiGroupBox({230.0f,  75.0f, 200.0f, 255.0f}, "Steam");
        GuiGroupBox({440.0f,  75.0f, 200.0f, 255.0f}, "SteamCmd");
        GuiGroupBox({650.0f,  75.0f, 200.0f, 255.0f}, "Aska");
        GuiGroupBox({20.0f,  340.0f, 830.0f, 255.0f}, "Server");

        handleConfigButtons();
    }
}

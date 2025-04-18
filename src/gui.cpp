#define RAYGUI_IMPLEMENTATION
#define GUI_WINDOW_FILE_DIALOG_IMPLEMENTATION

#include <util.h>
#include <win.h>
#include <gui.h>
#include <status.h>

#include <map>
#include <vector>
#include <fstream>


namespace Gui
{
    static std::map<std::string, int> _styleIndices;
    static std::vector<std::string> _styleNames;
    static std::string _stylesFolder;
    static std::string _savesFolder;
    static std::string _backupFolder;
    static std::string _styleList;
    static int _styleCount = 0;
    static int _styleIndex = 0;
    static Page _page = Server;
    static Page _prevPage = Server;

    static GuiWindowFileDialogState _fileDialogState;


    bool changedPage() {return _page != _prevPage;}

    GuiWindowFileDialogState& getFileDialogState() {return _fileDialogState;}


    void defaultSection(const std::string& section, GuiEntry guiEntries[], int numEntries)
    {
        // Keys and values
        for(int i=0; i<numEntries; i++)
        {
            Util::strcpy(guiEntries[i]._value, guiEntries[i]._default, MAX_CONFIG_TEXT, _F, _L);
            if(guiEntries[i]._type == CheckBox) guiEntries[i]._toggle = (std::string(guiEntries[i]._default) == "true") ? true : false;

            // Update option
            if(guiEntries[i]._type == Spinner)
            {
                // TODO: uses option as an integer value, a bit hacky but will do for now
                guiEntries[i]._option = std::stoi(guiEntries[i]._default, nullptr, 10);
            }
            else
            {
                int itemCount = 0;
                const char **items = GuiTextSplit(guiEntries[i]._options.c_str(), ';', &itemCount, nullptr);
                for(int j=0; j<itemCount; j++)
                {
                    if(Util::stricmp(items[j], guiEntries[i]._value) == 0) guiEntries[i]._option = j;
                }
            }
        }
    }

    void createSection(std::ofstream& outfile, const std::string& section, GuiEntry guiEntries[], int numEntries, bool useDefaults)
    {
        // Section
        outfile << "[" << section << "]" << std::endl;

        // Keys and values
        for(int i=0; i<numEntries; i++)
        {
            std::string key = guiEntries[i]._key;
            std::string val = useDefaults ? guiEntries[i]._default : guiEntries[i]._value;
            std::string padding = std::string(32 - key.size(), ' ');
            if(!useDefaults  &&  guiEntries[i]._type == CheckBox) val = guiEntries[i]._toggle ? "true" : "false";
            outfile << key << padding << " = " << val << std::endl;
        }

        outfile << std::endl;
    }

    bool updateValues(INIReader& iniReader, const std::string& section, GuiEntry guiEntries[], int numEntries, const GuiEntryFunc& userFunc)
    {
        for(int i=0; i<numEntries; i++)
        {
            // Update value
            std::string value = iniReader.Get(section, guiEntries[i]._key, "!!!!");
            if(value == "!!!!") return false;
            Util::strcpy(guiEntries[i]._value, value, MAX_CONFIG_TEXT, _F, _L);

            // CheckBox
            if(guiEntries[i]._type == CheckBox) guiEntries[i]._toggle = (value == "true") ? true : false;

            // Update option
            if(guiEntries[i]._type == Spinner)
            {
                // TODO: uses option as an integer value for spinners, a bit hacky but will do for now
                guiEntries[i]._option = std::stoi(guiEntries[i]._value, nullptr, 10);
            }
            else
            {
                int itemCount = 0;
                const char **items = GuiTextSplit(guiEntries[i]._options.c_str(), ';', &itemCount, nullptr);
                for(int j=0; j<itemCount; j++)
                {
                    if(Util::stricmp(items[j], guiEntries[i]._value) == 0) guiEntries[i]._option = j;
                }
            }

            if(userFunc) userFunc(guiEntries, i);
        }

        return true;
    }

    void handleTooltips(GuiEntry guiEntries[], int x, int y, int startEntry, int endEntry)
    {
        // No tooltips if any control is pressed
        for(auto i=startEntry; i<=endEntry; i++)
        {
            if(guiEntries[i]._state == STATE_PRESSED) return;
        }

        GuiEnableTooltip();
        for(auto i=startEntry; i<=endEntry; i++)
        {
            // Skip control that is in focus
            if(guiEntries[i]._state != STATE_FOCUSED) continue;

            GuiSetTooltip(guiEntries[i]._toolTip.c_str());
            GuiTooltip({float(x), float(y) + float(i-startEntry)*30, 180, 20});
        }
        GuiDisableTooltip();
    }

    void initStyles(int style)
    {
        _styleIndex = style;

        _styleNames.clear();
        _styleIndices.clear();

        _stylesFolder = getDediConfig(InstallPath) + "/" + getDediConfig(StylesFolder);
        Win::getFileNames(_stylesFolder, ".rgs", _styleNames);
        _styleCount = int(_styleNames.size());

        // Create styles list
        for(int i=0; i<_styleCount; i++)
        {
            char delim = (i < _styleCount-1) ? ';' : 0;
            _styleList += _styleNames[i] + delim;
            _styleIndices[_styleNames[i]] = i;
        }

        // Initial style
        std::string initialStyle = getDediConfig(InitialStyle);
        Util::lower(initialStyle);
        if(_styleIndices.find(initialStyle) != _styleIndices.end())
        {
            _styleIndex = _styleIndices[initialStyle];
            log(Util::Success, stderr, _f, _F, _L, "Successfully loaded style : %s", initialStyle.c_str());
        }
        GuiLoadStyleDefault();
        if(_styleCount  &&  _styleIndex < _styleCount) GuiLoadStyle((_stylesFolder + "/" + _styleNames[_styleIndex] + ".rgs").c_str());
        Status::reset(GuiGetFont());
    }

    void handleStyles()
    {
        static bool _styles = false;
        static GuiStateTimeout timeout(120);

        if(_styleCount == 0) return;

        if(_styles) GuiUnlock();
        if(GuiButton({10, 755, 90, 20}, "Styles")) _styles = !_styles;
        if(_styles)
        {
            // Open styles list
            float listViewHeight = float(_styleCount*18) + 4;
            GuiSetStyle(LISTVIEW, LIST_ITEMS_HEIGHT, 16);
            GuiListView({10, 750-listViewHeight, 90, listViewHeight}, _styleList.c_str(), nullptr, &_styleIndex);

            // Load selected style
            GuiLoadStyleDefault();
            if(_styleIndex >= 0  &&  _styleIndex < _styleNames.size())
            {
                GuiLoadStyle((_stylesFolder + "/" + _styleNames[_styleIndex] + ".rgs").c_str());
            }

            // Close styles list after short amount of time
            if(getMiscOptions(EnableStylesTimeout)) timeout.update(_styles);
            _styles ? GuiLock() : GuiUnlock();

            Status::reset(GuiGetFont());
        }
    }

    void handleStatus()
    {
        Status::draw("Status", {20.0f, 605.0f, 830.0f, 130.0f});

#if 0
#if 0
        std::vector<std::string> text;
        if(Win::readConsoleText(text))
        {
            for(size_t i=0; i<text.size(); i++)
            {
                Util::rtrim(text[i]);
                if(text[i].size() == 0) text[i].push_back('\n');
                Util::logStatus(text[i]);
            }
        }
#else
        static int tick = 0;
        if(++tick % 120 == 0)
        {
            static char text[MAX_STR_TEXT];
            int len = rand() % 226 + 10;
            int i = 0;
            for(; i<10; i++) text[i] = 48 + i;
            for(; i<len; i++) text[i] = rand() % 26 + 'a';
            for(; i<len+10; i++) text[i] = 48 + (9 - (i-len));
            text[i] = 0;
            Util::logStatus(text);
        }
#endif
#endif
    }

    void handleOptionsReset()
    {
        static bool _reset = false;

        if(GuiButton({570, 755, 90, 20}, "Reset")) _reset = !_reset;
        if(_reset)
        {
            _reset = false;
            defaultOptions();
            log(Util::Success, stderr, _f, _F, _L, "Reset Options");
        }
    }

    void handleConfigReset()
    {
        static bool _reset = false;

        if(GuiButton({570, 755, 90, 20}, "Reset")) _reset = !_reset;
        if(_reset)
        {
            _reset = false;
            defaultConfig();
            log(Util::Success, stderr, _f, _F, _L, "Reset Config");
        }
    }

    void handleReset()
    {
        if(_page != Config  &&  _page != Options) return;

        switch(_page)
        {
            case Config:  handleConfigReset();  break;
            case Options: handleOptionsReset(); break;

            default: break;
        }
    }

    void handleLoad()
    {
        static bool load = false;
        static bool loadFile = false;

        if(_page != Config  &&  _page != Options) return;

        // Load button
        if(GuiButton({670, 755, 90, 20}, "Load")) load = !load;
        if(load  &&  !getFileDialogState().windowActive)
        {
            load = false;
            loadFile = true;
            std::string title;
            switch(_page)
            {
                case Config:  title = "Load Config";  break;
                case Options: title = "Load Options"; break;

                default: return;
            }
            getFileDialogState().windowActive = true;
            getFileDialogState().saveFileMode = false;
            Util::strcpy(getFileDialogState().dialogTitle, title, DLG_STR_LEN, _F, _L);
            Util::strcpy(getFileDialogState().selectText, "Load", DLG_STR_LEN, _F, _L);
        }

        // Dialog load
        if(loadFile  &&  getFileDialogState().SelectFilePressed)
        {
            if(IsFileExtension(getFileDialogState().fileNameText, ".ini"))
            {
                loadFile = false;
                switch(_page)
                {
                    case Config:
                    {
                        std::string config = "Config.ini";
                        config = std::string(getFileDialogState().dirPathText) + PATH_SEPERATOR + std::string(getFileDialogState().fileNameText);
                        loadConfig(config);
                    }
                    break;

                    case Options:
                    {
                        std::string options = "Options.ini";
                        options = std::string(getFileDialogState().dirPathText) + PATH_SEPERATOR + std::string(getFileDialogState().fileNameText);
                        loadOptions(options);
                    }
                    break;
                }

            }

            getFileDialogState().SelectFilePressed = false;
        }
    }

    void handleSave()
    {
        static bool save = false;
        static bool saveFile = false;

        if(_page != Config  &&  _page != Options) return;

        // Save button
        if(GuiButton({770, 755, 90, 20}, "Save")) save = !save;
        if(save  &&  !getFileDialogState().windowActive)
        {
            save = false;
            saveFile = true;
            std::string title;
            switch(_page)
            {
                case Config:  title = "Save Config";  break;
                case Options: title = "Save Options"; break;

                default: return;
            }
            getFileDialogState().windowActive = true;
            getFileDialogState().saveFileMode = true;
            Util::strcpy(getFileDialogState().dialogTitle, title, DLG_STR_LEN, _F, _L);
            Util::strcpy(getFileDialogState().selectText, "Save", DLG_STR_LEN, _F, _L);
        }

        // Dialog save
        if(saveFile  &&  getFileDialogState().SelectFilePressed)
        {
            if(IsFileExtension(getFileDialogState().fileNameText, ".ini"))
            {
                saveFile = false;
                switch(_page)
                {
                    case Config:
                    {
                        std::string config = "Config.ini";
                        config = std::string(getFileDialogState().dirPathText) + PATH_SEPERATOR + std::string(getFileDialogState().fileNameText);
                        saveConfig(config);
                    }
                    break;

                    case Options:
                    {
                        std::string options = "Options.ini";
                        options = std::string(getFileDialogState().dirPathText) + PATH_SEPERATOR + std::string(getFileDialogState().fileNameText);
                        saveOptions(options);
                    }
                    break;
                }
            }

            getFileDialogState().SelectFilePressed = false;
        }
    }

    void handlePages()
    {
        switch(_page)
        {
            case Server:  handleServer();  break;
            case Config:  handleConfig();  break;
            case Saves:   handleSaves();   break;
            case World:   handleWorld();   break;
            case Options: handleOptions(); break;

            default: break;
        }

        if(_page != Server) handleServer(false);
        handleStatus();

        switch(_page)
        {
            case Server:  handleServerEntries();  break;
            case Config:  handleConfigEntries();  break;
            case Saves:   handleSavesEntries();   break;
            case World:   handleWorldEntries();   break;
            case Options: handleOptionsEntries(); break;

            default: break;
        }

        handleStyles();
        handleReset();
        handleLoad();
        handleSave();

        // No tooltips
        if(!getMiscOptions(EnableToolTips)) return;

        // Tooltips, (drawn last so always on top)
        switch(_page)
        {
            case Server:                           break;
            case Config:  handleConfigTooltips();  break;
            case Saves:                            break;
            case World:   handleWorldTooltips();   break;
            case Options: handleOptionsTooltips(); break;

            default: break;
        }
    }

    void handleAbout()
    {
        static bool about = false;
        if(GuiButton({770, 10, 90, 20}, "About")) about = true;
        if(about)
        {
            if(GuiMessageBox({275, 200, 350, 175}, "#191#About", "Dedi: Aska Dedicated Server Manager \nGithub: https://github.com/at67/Dedi\nAuthor: at67\nVersion: v0.1", "OK") >= 0)
            {
                about = false;
            }
        }
    }

    void handleButtons()
    {
        _prevPage = _page;

        if(GuiButton({10,  10, 90, 20}, "Server"))  _page = Server;
        if(GuiButton({110, 10, 90, 20}, "Config"))  _page = Config;
        if(GuiButton({210, 10, 90, 20}, "World"))   _page = World;
        if(GuiButton({310, 10, 90, 20}, "Options")) _page = Options;
        //if(GuiButton({410, 10, 90, 20}, "Saves"))   _page = Saves;

        handlePages();

        handleAbout();
    }

    void handle()
    {
        // Lock out all other controls during file dialog
        if(getFileDialogState().windowActive) GuiLock();

        handleButtons();

        if(getFileDialogState().windowActive) GuiUnlock();

        handleFileDialog("");
    }

    void handleFileDialog(const std::string& folder)
    {
        std::string path = folder;
        if(path == "") path = GetWorkingDirectory();
        Util::strcpy(getFileDialogState().dirPathText, path, DLG_STR_LEN, _F, _L);
        GuiWindowFileDialog(&_fileDialogState);
    }

    bool initialise()
    {
        Win::initialise();

        SetTraceLogLevel(LOG_ERROR);
        InitWindow(870, 785, "Aska Dedicated Server Launcher");
        SetTargetFPS(60);
        SetExitKey(KEY_NULL);

        Image icon = LoadImage("juggler.png");
        SetWindowIcon(icon);
        UnloadImage(icon);

        if(!loadConfig("config.ini"))   return false;
        if(!loadOptions("options.ini")) return false;

        initStyles(0);

        _fileDialogState = InitGuiWindowFileDialog(GetWorkingDirectory());
        
        return true;
    }
}

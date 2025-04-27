#include <util.h>
#include <win.h>
#include <steam.h>
#include <gui.h>
#include <status.h>

#include <set>
#include <fstream>


namespace Gui
{
    static bool _steamConnected = false;
    static bool _chatConnected  = false;
    static bool _worldActive    = false;

    static bool _foundSteam      = false;
    static bool _foundSteamCmd   = false;
    static bool _foundSteamToken = false;

    static bool _foundAska       = false;
    static bool _foundAskaProps  = false;

    static bool _steamCmdInstalled = false;
    static bool _steamCmdUpdated   = false;
    static bool _serverStarted     = false;

    static uint64_t _ticks      = 0;
    static uint64_t _upTime     = 0;
    static uint64_t _activeTime = 0;

    static int _serverCrashes = 0;

    static std::string _appPath;
    static std::vector<std::string> _libraryFolders;

    static std::set<std::string> _playerList;


    const std::string& getAppPath() {return _appPath;}


    bool checkSteam()
    {
        _libraryFolders.clear();
        return Steam::parseSteamVdf(_libraryFolders);
    }

    bool checkSteamCmd()
    {
        return Util::fileExists(getDediConfig(InstallPath) + "/" + getSteamCmdConfig(PathSteamCmd) + "/" + getSteamCmdConfig(ExecSteamCmd));
    }

    bool checkSteamToken()
    {
        // Check if server properties file exists
        std::string props = getDediConfig(InstallPath) + "/" + getAskaConfig(AskaSvrProps);
        if(Util::fileExists(props))
        {
            // Match steam token
            std::string token;
            if(readServerProp(props, "authenticationtoken=", token))
            {
                return (token == Util::lower(getServerConfig(AuthenticationToken)));
            }
        }

        // Check if steam token is at least not empty
        return (getServerConfig(AuthenticationToken).size() > 0);
    }

    bool checkAska()
    {
        return Steam::searchAppManifest(_libraryFolders, getAskaConfig(AskaSvrAppId), getAskaConfig(AskaSvrPath), _appPath);
    }

    bool checkAskaBat()
    {
        std::string backup = getDediConfig(InstallPath) + "/" + getDediConfig(BackupFolder);

        bool found = Util::fileExists(backup + "/" + getAskaConfig(AskaSvrBat));
        if(!found)
        {
            Win::createFolder(backup);
            Win::copyFile(_appPath + "/" + getAskaConfig(AskaSvrBat), backup + "/" + getAskaConfig(AskaSvrBat), true);
        }

        if(!Util::fileExists(getDediConfig(InstallPath) + "/" + getAskaConfig(AskaSvrBat)))
        {
            Steam::createBatFile(_appPath);
            found = false;
        }

        return found;
    }

    bool readServerProp(const std::string& file, const std::string& prop, std::string& value)
    {
        if(Util::matchFileText(file, prop, value, true))
        {
            value = value.substr(prop.length());
            if(value.size() == 0) return false;
            return true;
        }

        return false;
    }

    bool checkAskaProps()
    {
        std::string backup = getDediConfig(InstallPath) + "/" + getDediConfig(BackupFolder);

        bool found = Util::fileExists(backup + "/" + getAskaConfig(AskaSvrProps));
        if(!found)
        {
            Win::createFolder(backup);
            Win::copyFile(_appPath + "/" + getAskaConfig(AskaSvrProps), backup + "/" + getAskaConfig(AskaSvrProps), true);
        }

        found = Util::fileExists(getDediConfig(InstallPath) + "/" + getAskaConfig(AskaSvrProps));
        if(found)
        {
            // Read saveid from server properties and save in config
            std::string saveid;
            if(readServerProp(getDediConfig(InstallPath) + "/" + getAskaConfig(AskaSvrProps), "saveid=", saveid))
            {
                if(getServerConfig(SaveId).size() == 0) setServerConfig(SaveId, saveid);
            }
        }

        return found;
    }

    bool checkSteamConnected()
    {
        std::string line;
        return (Win::matchConsoleText("Connected to Steam successfully", line, true) != std::string::npos);
    }

    bool checkSteamClosed()
    {
        std::string line;
        return (Win::matchConsoleText("Closing connection...", line, true) != std::string::npos);
    }

    bool checkChatConnected()
    {
        std::string line;
        return (Win::matchConsoleText("Connected to chat!", line, true) != std::string::npos);
    }

    bool checkChatDisconnected()
    {
        std::string line;
        return (Win::matchConsoleText("Disconnected from chat", line, true) != std::string::npos);
    }

    bool checkWorldStarted()
    {
        std::string line;
        return (Win::matchConsoleText("Loading game world...", line, true) != std::string::npos);
    }

    bool checkWorldStopped()
    {
        std::string line;
        return (Win::matchConsoleText("Closing game world...", line, true) != std::string::npos);
    }

    bool checkServerCrashed()
    {
        std::string line;
        return (Win::matchConsoleText("A crash has been intercepted by the crash handler.", line, true) != std::string::npos);
    }

    std::string getSaveFile()
    {
        char* appdata = getenv("APPDATA");
        std::string saves = appdata + std::string("/../") + getAskaConfig(AskaSavePath);
        return saves + "/server/savegame_" + getServerConfig(SaveId);
    }

    std::string getOldestSave(const std::string& saves)
    {
        std::set<std::string> folders;
        if(!Win::getFolderNames(saves, getServerConfig(SaveId), folders)) return "";

        return *folders.begin();
    }

    bool delOldestSave(const std::string& folder, int maxFiles)
    {
        // Saves folder
        std::string saves = getDediConfig(InstallPath) + "/" + getDediConfig(SavesFolder) + "/" + folder;

        if(Win::getFolderCount(saves) > maxFiles)
        {
            std::string oldest = getOldestSave(saves);
            if(oldest.size()) return Win::delFolder(saves + "/" + oldest);
        }

        return false;
    }

    bool backupSave(const std::string& folder)
    {
        // Saves folder
        std::string saves = getDediConfig(InstallPath) + "/" + getDediConfig(SavesFolder);
        Win::createFolder(saves);

        // User folder
        saves += "/" + folder;
        Win::createFolder(saves);

        // Save folder
        saves += "/" + getServerConfig(SaveId) + "_" + Util::getDateTime();
        Win::createFolder(saves);

        std::string save = getSaveFile();
        bool success = Win::copyFolder(save, saves);
        if(success) log(Util::Success, stderr, _f, _F, _L, "Backed up Save from %s to %s", save.c_str(), saves.c_str());

        return success;
    }

    bool checkPlayerConnected()
    {
        std::string player;
        size_t pos = Win::matchConsoleText(" connected!", player, true);
        if(pos == std::string::npos  ||  pos < 1) return false;

        player = player.substr(0, pos);
        if(_playerList.find(player) != _playerList.end()) return false;

        Util::logStatus("Player : " + player + " : connected");
        _playerList.insert(player);

        if(backupSave("user")) delOldestSave("user", std::stoi(getDediConfig(MaxUserSaves), nullptr, 10));

        return true;
    }

    bool checkPlayerDisconnected()
    {
        std::string player;
        size_t pos = Win::matchConsoleText(" disconnected!", player, true);
        if(pos == std::string::npos  ||  pos < 1) return false;

        player = player.substr(0, pos);
        if(_playerList.find(player) == _playerList.end()) return false;

        Util::logStatus("Player : " + player + " : disconnected");
        _playerList.erase(player.substr(0, pos));

        return true;
    }

    bool checkWorldSaved()
    {
        std::string line;
        return (Win::matchConsoleText("Saving: Flushing done!", line, true) != std::string::npos);
    }

    void drawEnabledText(const std::string& text, int y, bool enable)
    {
        char icon[MAX_STR_TEXT];
        sprintf(icon, "#%03d# %s", 107 - enable, text.c_str());
        if(!enable) GuiSetState(STATE_DISABLED);
        GuiLabel({30.0f, float(y), 360.0f, 20.0f}, icon);
        if(!enable) GuiSetState(STATE_NORMAL);
    }

    void drawPlayerList()
    {
        static std::string players;

        if(_ticks % 60 == 0)
        {
            players.clear();
            for(const auto& p : _playerList)
            {
                players += p + ";";
            }
        }

        //players = "Cat;Dog;YoMama;Yo Samity Sam";

        float listViewHeight = float(4*28);
        GuiSetStyle(LISTVIEW, LIST_ITEMS_HEIGHT, 24);
        GuiGroupBox({450, 80, 400, listViewHeight + 20}, "Player List");
        int baseColour = GuiGetStyle(DEFAULT, BACKGROUND_COLOR);
        GuiSetStyle(DEFAULT, BACKGROUND_COLOR, GuiGetStyle(DEFAULT, BASE_COLOR_NORMAL));
        GuiListView({460, 90, 380, listViewHeight}, players.c_str(), nullptr, nullptr);
        GuiSetStyle(DEFAULT, BACKGROUND_COLOR, baseColour);
    }

    void drawUpTime()
    {
        static uint64_t upTime = _upTime;
        static char time[MAX_STR_TEXT] = "000h 00m 00s";

        GuiLabel({450, 230, 150, 20}, "Up Time");

        // Tick up time once a second
        if(_steamConnected  &&  (_ticks % 60 == 0)) _upTime++;

        if(upTime != _upTime)
        {
            upTime = _upTime;
            sprintf(time, "%03lldh %02lldm %02llds", _upTime / (60*60), _upTime / 60 % 60, _upTime % 60);
        }

        GuiStatusBar({450 + float(getTextPixels("Crash Restarts ")), 230, 130, 20}, time);
    }

    void drawActiveTime()
    {
        static uint64_t activeTime = _activeTime;
        static char time[MAX_STR_TEXT] = "000h 00m 00s";

        GuiLabel({450, 260, 150, 20}, "Active Time");

        // Tick active time once a second
        if(_worldActive  &&  (_ticks % 60 == 0)) _activeTime++;

        if(activeTime != _activeTime)
        {
            activeTime = _activeTime;
            sprintf(time, "%03lldh %02lldm %02llds", _activeTime / (60*60), _activeTime / 60 % 60, _activeTime % 60);
        }

        GuiStatusBar({450 + float(getTextPixels("Crash Restarts ")), 260, 130, 20}, time);
    }

    void drawCrashRestarts()
    {
        static char crashes[MAX_STR_TEXT] = "0";

        GuiLabel({450, 290, 150, 20}, "Crash Restarts");
        if(_ticks % 60 == 0) sprintf(crashes, "%d", _serverCrashes);
        GuiStatusBar({450 + float(getTextPixels("Crash Restarts ")), 290, 130, 20}, crashes);
    }

    void drawSeed()
    {
        int size = int(getServerConfig(Seed).size());
        if(size == 0  ||  size > MAX_STR_TEXT)
        {
            if(!setWorldSeed()) return;
        }

        static std::vector<char> seed(MAX_STR_TEXT + 1);
        Util::strcpy(&seed[0], getServerConfig(Seed), MAX_STR_TEXT, _F, _L);
        //float x = 850 - (getTextPixels(getServerConfig(Seed).c_str()) + 10);
        //GuiLabel({x - float(getTextPixels("Seed ")), 335, 150, 20}, "Seed ");
        //GuiTextBox({x, 335, float(getTextPixels(getServerConfig(Seed).c_str()) + 10), 20}, &seed[0], size, false);
        GuiLabel({450 - float(getTextPixels("Seed ")), 335, 150, 20}, "Seed ");
        GuiTextBox({450, 335, 400, 20}, &seed[0], size, false);

#if !defined(_DEBUG)
        // Clipboard
        if(GuiGetLocalState() == STATE_FOCUSED  &&  IsKeyDown(KEY_LEFT_CONTROL))
        {
            if(IsKeyDown(KEY_C)) clip::set_text(getServerConfig(Seed));
        }
#endif
    }

    float drawServerEntries(const std::vector<ServerEntries>& serverEntries, float x, float y, float h, int customIndex=-1)
    {
        float maxSizeName = 0;
        float maxSizeValue = 0;

        // Name
        for(int i=0; i<int(serverEntries.size()); i++) 
        {
            float size = float(getTextPixels(getServerConfigName(serverEntries[i]).c_str()));
            if(size > maxSizeName) maxSizeName = size;

            size = float(getTextPixels(getServerConfig(serverEntries[i]).c_str()));
            if(size + 20 > maxSizeValue) maxSizeValue = size + 20;
        }

        // Value
        float xpos = x + maxSizeName + 10;
        for(int i=0; i<int(serverEntries.size()); i++) 
        {
            if(customIndex >= 0  &&  i >= customIndex  &&  getServerConfig(Mode) != "custom") continue;
            GuiLabel({x, y + i*25.0f, maxSizeName, h}, getServerConfigName(serverEntries[i]).c_str());
            GuiStatusBar({xpos, y + i*25.0f, maxSizeValue, h}, getServerConfig(serverEntries[i]).c_str());
        }

        return xpos + maxSizeValue + 20;
    }

    void drawServerInfo()
    {
        GuiGroupBox({20, 365, 830, 230}, "Info");

        const std::vector<ServerEntries> serverEntries0 = {ProfileName, DisplayName, ServerName, Password, SaveId, Region, KeepServerWorldAlive, AutosaveStyle};
        float xpos = drawServerEntries(serverEntries0, 30, 382, 20);

        const std::vector<ServerEntries> serverEntries1 = {Mode, TerrainAspect, TerrainHeight, StartingSeason, YearLength, Precipitation, DayLength, StructureDecay};
        xpos = drawServerEntries(serverEntries1, 365, 382, 20, 2);

        const std::vector<ServerEntries> serverEntries2 = {InvasionDificulty, MonsterDensity, MonsterPopulation, WulfarPopulation, HerbivorePopulation, BearPopulation};
        xpos = drawServerEntries(serverEntries2, 610, 382, 20, 0);
    }

    void handleServerEntries()
    {
    }

    void startServer()
    {
        if(!_serverStarted)
        {
            // Don't execute the batch file as it's harder to terminate the Aska Dedicated Server process
            //Win::createProcess("", getDediConfig(InstallPath) + "/" + getAskaConfig(AskaSvrBat));

            // Launch the Aska Dedicated Server process directly
            std::string cmd = "\"" + _appPath + "/AskaServer.exe\"" + " -propertiesPath " + "\"" + getDediConfig(InstallPath) + "/server properties.txt\"";
            _serverStarted = Win::createProcess("", cmd);
            if(_serverStarted) Util::logStatus("Successfully started the Server");
        }
    }

    void closeServer()
    {
        if(_serverStarted)
        {
            _serverStarted = !Win::endProcess();
            if(!_serverStarted)
            {
                for(const auto& p : _playerList)
                {
                    Util::logStatus("Player : " + p + " : disconnected");
                }
                Util::logStatus("Successfully stopped the Server");
            }
        }
    }

    void handleServerButtons()
    {
        std::string button;

        // Full SteamCmd install
        if(!_foundSteamCmd)
        {
            static bool install = false;

            button = "Install";
            if(GuiButton({390, 755, 90, 20}, button.c_str())) install = !install;
            if(install)
            {
                install = false;
                GuiSetState(STATE_DISABLED);
                Steam::setCmdOp(Steam::CmdInit);
            }
        }
        // Start-Stop server
        else
        {
            static bool start = false;

            bool ready = _foundSteam & _foundSteamCmd & _foundSteamToken & _foundAska & _foundAskaProps & _steamCmdInstalled & _steamCmdUpdated;
            if(!ready) GuiSetState(STATE_DISABLED);

            button = _serverStarted ? "Stop" : "Start";
            if(GuiButton({390, 755, 90, 20}, button.c_str())) start = !start;
            if(start)
            {
                start = false;
                if(_serverStarted)
                {
                    closeServer();
                }
                else
                {
                    // Generate server properties, if it doesn't exist, before starting server
                    if(!Util::fileExists(getDediConfig(InstallPath) + "/" + getAskaConfig(AskaSvrProps))) saveServerProperties();
                    startServer();
                }
            }

            if(!ready) GuiSetState(STATE_NORMAL);
        }
    }

    void handleServer(bool render)
    {
        //if(_ticks % 120 == 0)
        //{
        //    if(backupSave("user"))   delOldestSave("user",   std::stoi(getDediConfig(MaxUserSaves), nullptr, 10));
        //    if(backupSave("server")) delOldestSave("server", std::stoi(getDediConfig(MaxServerSaves), nullptr, 10));
        //}

        // Check once a second
        if(++_ticks % 60 == 0  ||  changedPage())
        {
            _foundSteam      = checkSteam();
            _foundSteamCmd   = checkSteamCmd();
            _foundSteamToken = checkSteamToken();

            _foundAska      = checkAska();
            _foundAskaProps = checkAskaProps();

            _steamConnected = (!_steamConnected) ? checkSteamConnected() : !checkSteamClosed();
            _chatConnected  = (!_chatConnected)  ? checkChatConnected()  : !checkChatDisconnected();
            _worldActive    = (!_worldActive)    ? checkWorldStarted()   : !checkWorldStopped();

            _steamCmdInstalled = Steam::getSteamCmdInstalled();
            _steamCmdUpdated   = Steam::getSteamCmdUpdated();

            checkPlayerConnected();
            checkPlayerDisconnected();

            if(checkWorldSaved())
            {
                if(backupSave("server")) delOldestSave("server", std::stoi(getDediConfig(MaxServerSaves), nullptr, 10));

                Util::logStatus("World saved successfully!");
            }
        }

        if(!_serverStarted)
        {
            _steamConnected = false;
            _chatConnected = false;
            _worldActive = false;
        }

        if(checkServerCrashed())
        {
            Util::logStatus("Server crashed, restarting server");

            _steamConnected = false;
            _chatConnected = false;
            _worldActive = false;
            _serverStarted = false;
            _serverCrashes++;
            _ticks = 0;
            startServer();
        }

        if(!_steamConnected)
        {
            _upTime = 0;
        }

        if(!_worldActive)
        {
            _activeTime = 0;
            _playerList.clear();
        }

        if(!render) return;

        GuiPanel({10.0f, 40.0f, 850.0f, 705.0f}, "Server");

        drawEnabledText("Steam installed",                       80,  _foundSteam);
        drawEnabledText("SteamCmd installed",                    110, _steamCmdInstalled);
        drawEnabledText("SteamCmd updated",                      140, _steamCmdUpdated);
        drawEnabledText("Steam Authentication Token",            170, _foundSteamToken);
        drawEnabledText("Aska Dedicated Server installed",       200, _foundAska);
        drawEnabledText("Aska Server Properties file installed", 230, _foundAskaProps);
        drawEnabledText("Aska Server connected to Steam",        260, _steamConnected);
        drawEnabledText("Aska Server connected to Chat",         290, _chatConnected);
        drawEnabledText("Aska World active",                     320, _worldActive);

        drawPlayerList();
        drawUpTime();
        drawActiveTime();
        drawCrashRestarts();
        drawSeed();
        drawServerInfo();

        handleServerButtons();
    }
}

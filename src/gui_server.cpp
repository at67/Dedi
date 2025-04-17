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
    static bool _foundAskaBat    = false;
    static bool _foundAskaProps  = false;

    static bool _serverStarted  = false;

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
        bool found = Util::fileExists(getDediConfig(InstallPath) + "/" + getSteamCmdConfig(PathSteamCmd) + "/" + getSteamCmdConfig(ExecSteamCmd));
        if(!found  &&  Steam::getCmdOp() == Steam::CmdIdle) Steam::setCmdOp(Steam::CmdInit);
        return found;
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
            Win::createDirectory(backup);
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
            Win::createDirectory(backup);
            Win::copyFile(_appPath + "/" + getAskaConfig(AskaSvrProps), backup + "/" + getAskaConfig(AskaSvrProps), true);
        }

        found = Util::fileExists(getDediConfig(InstallPath) + "/" + getAskaConfig(AskaSvrProps));
        if(found)
        {
            // Read saveid from server properties and save in config
            std::string saveid;
            if(readServerProp(getDediConfig(InstallPath) + "/" + getAskaConfig(AskaSvrProps), "saveid=", saveid))
            {
                setServerConfig(SaveId, saveid);
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

    bool checkPlayerConnected()
    {
        std::string player;
        size_t pos = Win::matchConsoleText(" connected!", player, true);
        if(pos == std::string::npos  ||  pos < 1) return false;

        player = player.substr(0, pos);
        if(_playerList.find(player) != _playerList.end()) return false;

        Util::logStatus("Player : " + player + " : connected");
        _playerList.insert(player);
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

        float listViewHeight = float(4*28);
        GuiSetStyle(LISTVIEW, LIST_ITEMS_HEIGHT, 24);
        GuiGroupBox({450, 80, 400, listViewHeight + 20}, "Player List");
        GuiListView({460, 90, 380, listViewHeight}, players.c_str(), nullptr, nullptr);
    }

    void drawUpTime()
    {
        static char time[MAX_STR_TEXT] = "000h 00m 00s";

        GuiLabel({450, 230, 150, 20}, "Up Time:");
        if(_steamConnected)
        {
            if(_ticks % 60 == 0)
            {
                sprintf(time, "%03lldh %02lldm %02llds", _upTime / (60*60), _upTime / 60 % 60, _upTime % 60);
                _upTime++;
            }
        }
        GuiLabel({450 + float(Status::getTextPixels("Crash Restarts:  ")), 230, 200, 20}, time);
    }

    void drawActiveTime()
    {
        static char time[MAX_STR_TEXT] = "000h 00m 00s";

        GuiLabel({450, 260, 150, 20}, "Active Time:");
        if(_worldActive)
        {
            if(_ticks % 60 == 0)
            {
                sprintf(time, "%03lldh %02lldm %02llds", _activeTime / (60*60), _activeTime / 60 % 60, _activeTime % 60);
                _activeTime++;
            }
        }
        GuiLabel({450 + float(Status::getTextPixels("Crash Restarts:  ")), 260, 200, 20}, time);
    }

    void drawCrashRestarts()
    {
        static char crashes[MAX_STR_TEXT] = "0";

        GuiLabel({450, 290, 150, 20}, "Crash Restarts:");
        if(_ticks % 60 == 0) sprintf(crashes, "%d", _serverCrashes);
        GuiLabel({450 + float(Status::getTextPixels("Crash Restarts:  ")), 290, 200, 20}, crashes);
    }

    void drawChatHistory()
    {
        GuiGroupBox({20, 365, 830, 230}, "Info");
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
        static bool start = false;

        bool ready = _foundSteam & _foundSteamCmd & _foundSteamToken & _foundAska & _foundAskaBat & _foundAskaProps;

        if(!ready) GuiSetState(STATE_DISABLED);

        std::string button = _serverStarted ? "Stop" : "Start";
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
                if(!Util::fileExists(getDediConfig(InstallPath) + "/" + getAskaConfig(AskaSvrProps))) saveServerProperties();
                startServer();
            }
        }

        if(!ready) GuiSetState(STATE_NORMAL);
    }

    void handleServer()
    {
        // Check once a second
        if(++_ticks % 60 == 0  ||  changedPage())
        {
            _foundSteam      = checkSteam();
            _foundSteamCmd   = checkSteamCmd();
            _foundSteamToken = checkSteamToken();

            _foundAska      = checkAska();
            _foundAskaBat   = checkAskaBat();
            _foundAskaProps = checkAskaProps();

            _steamConnected = (!_steamConnected) ? checkSteamConnected() : !checkSteamClosed();
            _chatConnected  = (!_chatConnected)  ? checkChatConnected()  : !checkChatDisconnected();
            _worldActive    = (!_worldActive)    ? checkWorldStarted()   : !checkWorldStopped();

            checkPlayerConnected();
            checkPlayerDisconnected();

            if(checkWorldSaved()) Util::logStatus("World saved successfully!");
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

        GuiPanel({10.0f, 40.0f, 850.0f, 705.0f}, "Server");

        drawEnabledText("Steam installed",                       80,  _foundSteam);
        drawEnabledText("SteamCmd installed",                    110, _foundSteamCmd);
        drawEnabledText("Steam Authentication Token",            140, _foundSteamToken);
        drawEnabledText("Aska Dedicated Server installed",       170, _foundAska);
        drawEnabledText("Aska Server Batch file installed",      200, _foundAskaBat);
        drawEnabledText("Aska Server Properties file installed", 230, _foundAskaProps);
        drawEnabledText("Aska Server connected to Steam",        260, _steamConnected);
        drawEnabledText("Aska Server connected to Chat",         290, _chatConnected);
        drawEnabledText("Aska World active",                     320, _worldActive);

        drawPlayerList();
        drawUpTime();
        drawActiveTime();
        drawCrashRestarts();
        drawChatHistory();

        handleServerButtons();
    }
}

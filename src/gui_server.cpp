#include <subprocess.h>

#include <util.h>
#include <win.h>
#include <steam.h>
#include <gui.h>
#include <status.h>

#include <set>
#include <chrono>
#include <fstream>
#include <sstream>
#include <thread>
#include <atomic>
#include <mutex>


namespace Gui
{
    static std::mutex _playerMutex;
    static std::string _playerName;

    static std::atomic<bool> _steamConnected     = false;
    static std::atomic<bool> _chatConnected      = false;
    static std::atomic<bool> _worldActive        = false;
    static std::atomic<bool> _worldSaved         = false;
    static std::atomic<bool> _playerConnected    = false;
    static std::atomic<bool> _playerDisconnected = false;

    static std::atomic<bool> _serverStarted = false;
    static std::atomic<bool> _serverExited  = false;

    static bool _firstTime = true;

    static bool _foundSteam      = false;
    static bool _foundSteamCmd   = false;
    static bool _foundSteamToken = false;

    static bool _foundAska       = false;
    static bool _foundAskaProps  = false;

    static bool _steamCmdInstalled = false;
    static bool _steamCmdUpdated   = false;


    void checkServerWorker();
    static std::thread _checkServerThread(checkServerWorker);

    void stdinServerWorker();
    static std::thread _stdinServerThread(stdinServerWorker);

    static struct subprocess_s _subProcess;

    static uint64_t _ticks = 0;

    static auto _upTimeStart     = std::chrono::system_clock::now();
    static auto _activeTimeStart = std::chrono::system_clock::now();

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
        if(!_playerConnected) return false;
        _playerConnected = false;

        std::string player;
        {
            std::scoped_lock lock(_playerMutex);
            player = _playerName;
        }

        if(_playerList.find(player) != _playerList.end()) return false;

        Util::logStatus("Player : " + player + " : connected");
        _playerList.insert(player);

        if(backupSave("user")) delOldestSave("user", std::stoi(getDediConfig(MaxUserSaves), nullptr, 10));

        return true;
    }

    bool checkPlayerDisconnected()
    {
        if(!_playerDisconnected) return false;
        _playerDisconnected = false;

        std::string player;
        {
            std::scoped_lock lock(_playerMutex);
            player = _playerName;
        }

        if(_playerList.find(player) == _playerList.end()) return false;

        Util::logStatus("Player : " + player + " : disconnected");
        _playerList.erase(player);

        return true;
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
        static char time[MAX_STR_TEXT] = "000h 00m 00s";

        GuiLabel({450, 230, 150, 20}, "Up Time");

        // Roughly update twice per second
        if(_ticks % 30 == 0)
        {
            if(_steamConnected)
            {
                auto upTime = std::chrono::system_clock::now() - _upTimeStart;

                int64_t hours = std::chrono::duration_cast<std::chrono::hours>(upTime).count();
                int64_t mins  = std::chrono::duration_cast<std::chrono::minutes>(upTime).count() - hours*60;
                int64_t secs  = std::chrono::duration_cast<std::chrono::seconds>(upTime).count() - hours*3600 - mins*60;
                sprintf(time, "%03lldh %02lldm %02llds", hours, mins, secs);
            }
            else
            {
                strcpy(time, "000h 00m 00s");
            }
        }

        GuiStatusBar({450 + float(getTextPixels("Crash Restarts ")), 230, 130, 20}, time);
    }

    void drawActiveTime()
    {
        static char time[MAX_STR_TEXT] = "000h 00m 00s";

        GuiLabel({450, 260, 150, 20}, "Active Time");

        // Roughly update twice per second
        if(_ticks % 30 == 0)
        {
            if(_worldActive)
            {
                auto activeTime = std::chrono::system_clock::now() - _activeTimeStart;

                int64_t hours = std::chrono::duration_cast<std::chrono::hours>(activeTime).count();
                int64_t mins  = std::chrono::duration_cast<std::chrono::minutes>(activeTime).count() - hours*60;
                int64_t secs  = std::chrono::duration_cast<std::chrono::seconds>(activeTime).count() - hours*3600 - mins*60;
                sprintf(time, "%03lldh %02lldm %02llds", hours, mins, secs);
            }
            else
            {
                strcpy(time, "000h 00m 00s");
            }
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

        const std::vector<ServerEntries> serverEntries2 = {ClothingDecay, InvasionDificulty, MonsterDensity, MonsterPopulation, WulfarPopulation, HerbivorePopulation, BearPopulation};
        xpos = drawServerEntries(serverEntries2, 610, 382, 20, 0);
    }

    void handleServerEntries()
    {
    }

    void checkServerWorker()
    {
        while(getGuiStarted())
        {
            if(_serverStarted  &&  Win::waitProcess(100, _subProcess.hProcess))
            {
                // Check _serverStarted again as it's updated in the main thread
                if(_serverStarted) _serverExited = true;
            }

            Util::sleep_ms(100);
        }
    }

    void stdinServerWorker()
    {
        struct MatchText
        {
            std::atomic<bool>* _condition = nullptr;
            bool _result = false;
            bool _player = false;
            std::string _text;
        };

        const std::vector<MatchText> whiteList =
        {
            {&_steamConnected,     true,  false, "Connected to Steam successfully", },
            {nullptr,              false, false, "Openning connection...",          },
            {&_steamConnected,     false, false, "Closing connection...",           },
            {nullptr,              false, false, "The session is now open!",        },
            {nullptr,              false, false, "Received your UserID from server."},
            {&_chatConnected,      true,  false, "Connected to chat!",              },
            {&_chatConnected,      false, false, "Disconnected from chat",          },
            {&_worldActive,        true,  false, "Loading game world...",           },
            {&_worldActive,        false, false, "Closing game world...",           },
            {&_playerConnected,    true,  true , " connected!",                     },
            {&_playerDisconnected, true,  true , " disconnected!",                  },
            {&_worldSaved,         true,  false, "Saving: Flushing done!",          },
        };

        char text[1024];

        while(!getGuiStarted())
        {
            Util::sleep_ms(100);
        }

        while(getGuiStarted())
        {
            if(_serverStarted)
            {
                int read = subprocess_read_stdout(&_subProcess, text, 1023);
                if(read)
                {
                    text[read] = 0;
                    std::string line;
                    std::stringstream stream(text);
                    while(std::getline(stream, line))
                    {
                        for(size_t i=0; i<whiteList.size(); i++)
                        {
                            size_t pos = line.find(whiteList[i]._text);
                            if(pos != std::string::npos)
                            {
                                // Set condition
                                if(whiteList[i]._condition) *whiteList[i]._condition = whiteList[i]._result;

                                // Save player name
                                if(whiteList[i]._player)
                                {
                                    std::scoped_lock lock(_playerMutex);
                                    _playerName = line.substr(0, pos);
                                }
                                printf("%s\n", line.c_str());
                                break;
                            }
                        }
                    }
                }
            }

            Util::sleep_ms(100);
        }
    }

    void startServer()
    {
        if(!_serverStarted)
        {
            std::string cmd = _appPath + "/AskaServer.exe";
            std::string arg = getDediConfig(InstallPath) + "/server properties.txt";
            const char *cmdLine[] = {cmd.c_str(), "-propertiesPath", arg.c_str(), NULL};
            _serverStarted = (subprocess_create(cmdLine, subprocess_option_enable_async | subprocess_option_inherit_environment, &_subProcess) == 0);
            if(_serverStarted) Util::logStatus("Starting the Server");
        }
    }

    void stopServer()
    {
        if(_serverStarted)
        {
            subprocess_terminate(&_subProcess);
            _serverStarted = !(subprocess_destroy(&_subProcess) == 0);
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

    void shutdownServer()
    {
        _serverStarted = false;
        subprocess_terminate(&_subProcess);
        subprocess_destroy(&_subProcess);
        _checkServerThread.join();
        _stdinServerThread.join();
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
                Steam::setSteamCmdOp(Steam::SteamCmdInit);
            }
        }
        // Start-Stop server
        else
        {
            static bool start = false;

            bool ready = _foundSteam && _foundSteamCmd && _foundSteamToken && _foundAska && _foundAskaProps && _steamCmdInstalled && _steamCmdUpdated;
            if(!ready) GuiSetState(STATE_DISABLED);

            button = _serverStarted ? "Stop" : "Start";
            if(GuiButton({390, 755, 90, 20}, button.c_str())) start = !start;
            if(start)
            {
                start = false;
                if(_serverStarted)
                {
                    stopServer();
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

    void initServer()
    {
        _foundSteam    = checkSteam();
        _foundSteamCmd = checkSteamCmd();

        _foundAska = checkAska();
    }

    void updateServer()
    {
        static bool steamConnected = _steamConnected;
        static bool worldActive    = _worldActive;

        checkPlayerConnected();
        checkPlayerDisconnected();
            
        if(_worldSaved)
        {
            _worldSaved = false;
            if(backupSave("server")) delOldestSave("server", std::stoi(getDediConfig(MaxServerSaves), nullptr, 10));
            Util::logStatus("World saved successfully!");
        }

        if(!_steamCmdInstalled) _steamCmdInstalled = Steam::getSteamCmdInstalled();
        if(!_steamCmdUpdated)   _steamCmdUpdated   = Steam::getSteamCmdUpdated();

        // Check if page changes
        if(_firstTime  ||  changedPage())
        {
            _foundSteamToken = checkSteamToken();
            _foundAskaProps = checkAskaProps();
        }

        // Server stopped
        if(!_serverStarted)
        {
            _steamConnected = false;
            _chatConnected = false;
            _worldActive = false;
        }

        // Chat error
        if(_worldActive  &&  !_chatConnected)
        {
            _steamConnected = false;
            _worldActive = false;
            _serverStarted = false;
        }

        // Server crash
        if(_serverExited)
        {
            _serverExited   = false;
            _steamConnected = false;
            _chatConnected  = false;
            _worldActive    = false;
            _serverStarted  = false;
            _serverCrashes++;
            _ticks = 0;
            startServer();
            Util::logStatus("Server crashed, restarting server");
        }

        if(_steamConnected  &&  !steamConnected) _upTimeStart = std::chrono::system_clock::now();

        if(_worldActive  &&  !worldActive) _activeTimeStart = std::chrono::system_clock::now();

        if(worldActive  &&  !_worldActive) _playerList.clear();

        steamConnected = _steamConnected;
        worldActive = _worldActive;

        _firstTime = false;
    }

    void handleServer(bool render)
    {
        updateServer();

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

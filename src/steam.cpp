#include <vdf.h>
#include <dirent.h>

#include <util.h>
#include <win.h>
#include <steam.h>
#include <access.h>

#include <sstream>


namespace Steam
{
    static bool _steamCmdInstalled = false;
    static bool _steamCmdUpdated   = false;

    static SteamCmdOp _steamCmdOp = SteamCmdIdle;


    bool getSteamCmdInstalled() {return _steamCmdInstalled;}
    bool getSteamCmdUpdated()   {return _steamCmdUpdated;  }

    void setSteamCmdOp(SteamCmdOp steamCmdOp) {_steamCmdOp = steamCmdOp;}


    bool parseSteamVdf(std::vector<std::string>& libraryFolders)
    {
        // Get Steam's path from registry :(
        std::string steamPath = Win::getHKLMRegStr(Gui::getSteamConfig(Gui::HKLMRegSubKey), Gui::getSteamConfig(Gui::HKLMRegValue));
        if(steamPath == "") return false;
    
        // Parse libraryfolders.vdf and build a vector of library paths
        std::ifstream file(steamPath + "/" + Gui::getSteamConfig(Gui::SteamApps) + "/" + Gui::getSteamConfig(Gui::LibraryFolders));
        auto root = tyti::vdf::read(file);
        for(const auto& child : root.childs)
        {
            std::string folder = child.first;
            auto attribs = root.childs[folder].get()->attribs;
            if(attribs.find("path") == attribs.end()) continue;
            libraryFolders.push_back(attribs["path"] + "/" + Gui::getSteamConfig(Gui::SteamApps));
        }
    
        return true;
    }

    bool searchAppManifest(const std::vector<std::string>& folders, const std::string& id, const std::string& app, std::string& path)
    {
        for(size_t i=0; i<folders.size(); i++)
        {
            DIR* dir;
            if((dir = opendir(folders[i].c_str())) != nullptr)
            {
                struct dirent* ent;
                while((ent = readdir(dir)) != nullptr)
                {
                    std::string filename = std::string(ent->d_name);
                    if(ent->d_type == DT_DIR) continue;

                    if(ent->d_type == DT_REG  &&  filename.find(Gui::getSteamConfig(Gui::AppManifest)) != std::string::npos)
                    {
                        if(filename.find(id) != std::string::npos)
                        {
                            path = folders[i] + "/" + app;
                            closedir(dir);
                            return true;
                        }
                    }
                }
                closedir(dir);
            }
        }

        return false;
    }

    bool wait(const std::string& match)
    {
        std::string line;
        if(Win::matchConsoleText(match, line) != std::string::npos)
        {
            Win::sendConsoleText(Gui::getSteamCmdConfig(Gui::QuitSteamCmd));
            return true;
        }

        return false;
    }

    bool install()
    {
        // Destination folders
        Win::createFolder(Gui::getDediConfig(Gui::InstallPath));
        std::string steamCmdPath = Gui::getDediConfig(Gui::InstallPath) + "/" + Gui::getSteamCmdConfig(Gui::PathSteamCmd);
        std::string steamCmdExec = steamCmdPath + "/steamcmd.zip";
        Win::createFolder(steamCmdPath);

        // SteamCmd
        Win::downloadLink(Gui::getSteamCmdConfig(Gui::HTTPSteamCmd), steamCmdExec);
        Util::decompressZLibArchive(steamCmdExec, steamCmdPath, _F, _L);

        Win::createProcess(steamCmdPath + "/" + Gui::getSteamCmdConfig(Gui::ExecSteamCmd), "");

        return true;
    }

    bool update()
    {
        std::string steamCmdPath = Gui::getDediConfig(Gui::InstallPath) + "/" + Gui::getSteamCmdConfig(Gui::PathSteamCmd);
        Win::createProcess(steamCmdPath + "/" + Gui::getSteamCmdConfig(Gui::ExecSteamCmd), "");

        Win::sendConsoleText(Gui::getSteamCmdConfig(Gui::OpenSteamCmd));
        Win::sendConsoleText(Gui::getSteamCmdConfig(Gui::AppUSteamCmd) + " " + Gui::getAskaConfig(Gui::AskaSvrAppId));

        return true;
    }

    bool createBatFile(const std::string& path)
    {
        std::string bat = Gui::getDediConfig(Gui::InstallPath) + "/" + Gui::getAskaConfig(Gui::AskaSvrBat);
        std::ofstream outfile(bat, std::ios::out);
        if(!outfile.is_open())
        {
            log(Util::FatalError, stderr, _f, _F, _L, "Failed to open '%s'", bat.c_str());
            return false;
        }

        outfile << "@echo off" << std::endl;
        outfile << "set SteamAppId=" << Gui::getAskaConfig(Gui::AskaExeAppId) << std::endl;
        outfile << "\"" + path + "/AskaServer.exe\"" + " -propertiesPath " + "\"" + Gui::getDediConfig(Gui::InstallPath) + "/server properties.txt\"" << std::endl;
        return true;
    }

    void handle()
    {
        if(_steamCmdOp != SteamCmdIdle) Win::readConsoleText();

        // Simple state machine that handles installs and updates
        switch(_steamCmdOp)
        {
            case SteamCmdIdle: return;

            case SteamCmdInit:
            {
                if(!Util::fileExists(Gui::getDediConfig(Gui::InstallPath) + "/" + Gui::getSteamCmdConfig(Gui::PathSteamCmd) + "/" + Gui::getSteamCmdConfig(Gui::ExecSteamCmd)))
                {
                    _steamCmdOp = SteamCmdInstall;
                    return;
                }

                _steamCmdInstalled = true;
                _steamCmdOp = SteamCmdUpdate;
            }
            break;

            case SteamCmdInstall:
            {
                _steamCmdInstalled = false;
                Util::logStatus("Starting SteamCmd install");
                if(install())
                {
                    _steamCmdOp = SteamCmdInstallWait;
                }
                else
                {
                    _steamCmdOp = SteamCmdIdle;
                    Util::logStatus("Couldn't start SteamCmd install");
                }
            }
            break;

            case SteamCmdInstallWait:
            {
                if(wait(Gui::getSteamCmdConfig(Gui::DoneSteamCmd)))
                {
                    Win::endProcess();
                    _steamCmdInstalled = true;
                    _steamCmdOp = SteamCmdUpdate;
                    Util::logStatus("Successfully installed SteamCmd");
                }
            }
            break;

            case SteamCmdUpdate:
            {
                if(!Util::fileExists(Gui::getDediConfig(Gui::InstallPath) + "/" + Gui::getSteamCmdConfig(Gui::PathSteamCmd) + "/" + Gui::getSteamCmdConfig(Gui::ExecSteamCmd)))
                {
                    _steamCmdOp = SteamCmdInstall;
                    return;
                }

                _steamCmdInstalled = true;
                _steamCmdUpdated = false;
                Util::logStatus("Starting SteamCmd update");
                if(update())
                {
                    _steamCmdOp = SteamCmdUpdateWait;
                }
                else
                {
                    _steamCmdOp = SteamCmdIdle;
                    Util::logStatus("Couldn't start SteamCmd update");
                }
            }
            break;

            case SteamCmdUpdateWait:
            {
                if(wait(Gui::getSteamCmdConfig(Gui::CoupSteamCmd)))
                {
                    Win::endProcess();
                    _steamCmdUpdated = true;
                    _steamCmdOp = SteamCmdIdle;
                    Util::logStatus("Successfully updated SteamCmd");
                }
            }
            break;

            default: break;
        }

        if(_steamCmdOp != SteamCmdIdle) Win::clearConsoleText();
    }
}
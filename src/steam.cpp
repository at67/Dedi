#include <vdf.h>
#include <dirent.h>

#include <util.h>
#include <win.h>
#include <steam.h>
#include <access.h>


namespace Steam
{
    static CmdOp _cmdOp = CmdIdle;


    CmdOp getCmdOp() {return _cmdOp;}
    void setCmdOp(CmdOp cmdOp) {_cmdOp = cmdOp;}


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
        for(auto i=0; i<folders.size(); i++)
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
        if(Win::matchConsoleText(match, line))
        {
            Win::sendConsoleText(Gui::getSteamCmdConfig(Gui::QuitSteamCmd));
            return true;
        }

        return false;
    }

    bool update()
    {
        std::string steamCmdPath = Gui::getDediConfig(Gui::InstallPath) + "/" + Gui::getSteamCmdConfig(Gui::PathSteamCmd);
        Win::createProcess(steamCmdPath + "/" + Gui::getSteamCmdConfig(Gui::ExecSteamCmd), "");

        Win::sendConsoleText(Gui::getSteamCmdConfig(Gui::OpenSteamCmd));
        Win::sendConsoleText(Gui::getSteamCmdConfig(Gui::AppUSteamCmd) + " " + Gui::getAskaConfig(Gui::AskaSvrAppId));

        return true;
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
        // Simple state machine that handles installs and updates
        switch(_cmdOp)
        {
            case CmdIdle: return;

            case CmdWaitCoup:
            {
                if(wait(Gui::getSteamCmdConfig(Gui::CoupSteamCmd)))
                {
                    _cmdOp = CmdIdle;
                    Util::logStatus("Successfully updated SteamCmd");
                }
            }
            break;

            case CmdWaitDone:
            {
                if(wait(Gui::getSteamCmdConfig(Gui::DoneSteamCmd)))
                {
                    _cmdOp = CmdUpdate;
                    Util::logStatus("Successfully installed SteamCmd");
                }
            }
            break;

            case CmdInit:
            {
                if(!Util::fileExists(Gui::getDediConfig(Gui::InstallPath) + "/" + Gui::getSteamCmdConfig(Gui::PathSteamCmd) + "/" + Gui::getSteamCmdConfig(Gui::ExecSteamCmd)))
                {
                    _cmdOp = CmdInstall;
                    return;
                }

                _cmdOp = CmdUpdate;
            }
            break;

            case CmdInstall:
            {
                install();
                _cmdOp = CmdWaitDone;
            }
            break;

            case CmdUpdate:
            {
                update();
                _cmdOp = CmdWaitCoup;
            }
            break;

            default: break;
        }
    }
}
#pragma once

#include <string>
#include <vector>
#include <atomic>


namespace Steam
{
    enum SteamCmdOp {SteamCmdIdle=0, SteamCmdInit, SteamCmdInstall, SteamCmdInstallWait, SteamCmdUpdate, SteamCmdUpdateWait};


    bool getSteamCmdInstalled();
    bool getSteamCmdUpdated();

    void setSteamCmdOp(SteamCmdOp);

    bool parseSteamVdf(std::vector<std::string>& libraryFolders);
    bool searchAppManifest(const std::vector<std::string>& folders, const std::string& search, const std::string& app, std::string& path);

    bool createBatFile(const std::string& path);

    void handle();
}
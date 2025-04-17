#pragma once

#include <string>
#include <vector>


namespace Steam
{
    enum CmdOp {CmdIdle, CmdInit, CmdWaitCoup, CmdWaitDone, CmdInstall, CmdUpdate};


    bool parseSteamVdf(std::vector<std::string>& libraryFolders);
    bool searchAppManifest(const std::vector<std::string>& folders, const std::string& search, const std::string& app, std::string& path);

    CmdOp getCmdOp();
    void setCmdOp(CmdOp cmdOp);

    bool createBatFile(const std::string& path);

    void handle();
}
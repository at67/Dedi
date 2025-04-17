#pragma once

#include <string>
#include <vector>


namespace Gui
{
    enum Page {Server=0, Config, Saves, World, Options, NumPages};

    enum DediEntries {InstallPath=0, StylesFolder, SavesFolder, BackupFolder, InitialStyle, NumDediEntries};
    enum SteamEntries {HKLMRegSubKey=0, HKLMRegValue, SteamApps, AppManifest, LibraryFolders, HTTPSteamSvr, NumSteamEntries};
    enum SteamCmdEntries {PathSteamCmd=0, DoneSteamCmd, CoupSteamCmd, QuitSteamCmd, AppUSteamCmd, ExecSteamCmd, OpenSteamCmd, HTTPSteamCmd, NumSteamCmdEntries};
    enum AskaEntries {AskaSvrBat=0, AskaSvrProps, AskaSvrPath, AskaExePath, AskaSvrAppId, AskaExeAppId, AskaSavePath, NumAskaEntries};
    enum ServerEntries
    {
        DediName=0, SaveId, DisplayName, ServerName, Seed, Password, SteamGamePort, SteamQueryPort, AuthenticationToken, Region, KeepServerWorldAlive,
        AutosaveStyle, Mode, TerrainAspect, TerrainHeight, StartingSeason, YearLength, Precipitation, DayLength, StructureDecay, InvasionDificulty,
        MonsterDensity, MonsterPopulation, WulfarPopulation, HerbivorePopulation, BearPopulation, NumServerEntries
    };
    enum MiscEntries {EnableToolTips=0, EnableMenusTimeout, EnableStylesTimeout, EnableStatusAutoScroll, EnableWorldGen, NumMiscEntries};


    const std::string& getAppPath();

    std::string getDediConfig(DediEntries entry);
    std::string getSteamConfig(SteamEntries entry);
    std::string getSteamCmdConfig(SteamCmdEntries entry);
    std::string getAskaConfig(AskaEntries entry);
    std::string getServerConfig(ServerEntries entry);

    void setServerConfig(ServerEntries entry, const std::string& value);

    bool getMiscOptions(MiscEntries entry);

    bool changedPage();

    bool readServerProp(const std::string& file, const std::string& prop, std::string& value);

    size_t getConfigKeysAndValues(const std::string& section, std::vector<std::string>& keys, std::vector<std::string>& values);
    size_t getOptionsKeysAndValues(const std::string& section, std::vector<std::string>& keys, std::vector<std::string>& values);

    bool saveServerProperties();
}
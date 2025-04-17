#pragma once

#include <string>
#include <unordered_map>


#define SETTING_LENGTH 30


namespace World
{
    enum SettingsType {TerrainAspect=0x05, TerrainHeight=0x06, StartingSeason=0x12, YearLength=0x13, Precipitation=0x14, DayLength=0x15, StructureDecay=0x16,
                       InvasionDifficulty=0x17, MonsterDensity=0x34, MonsterPopulation=0x3D, WulfarPopulation=0x24, HerbivorePopulation=0x32, BearPopulation=0x57};


    bool loadWorldGen(const std::string& worldGen);
    bool saveWorldGen(const std::string& worldGen);

    bool getWorldGen(std::unordered_map<SettingsType, char>& settings);
    bool setWorldGen(const std::unordered_map<SettingsType, char>& settings);

    bool setSetting(SettingsType type, char value);
    bool addSetting(SettingsType type, char value);
}
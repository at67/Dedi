#include <util.h>
#include <win.h>
#include <gui.h>
#include <world.h>


#define NUM_WORLD_ENTRIES 13


namespace Gui
{
    static std::unordered_map<World::SettingsType, char> _worldSettings;

    enum WorldEntries {WorldTerrainAspect=0, WorldTerrainHeight, WorldStartingSeason, WorldYearLength, WorldPrecipitation, WorldDayLength, WorldStructureDecay, WorldInvasionDificulty,
                       WorldMonsterDensity, WorldMonsterPopulation, WorldWulfarPopulation, WorldHerbivorePopulation, WorldBearPopulation, NumWorldEntries};

    struct WorldEntry
    {
        World:: SettingsType _type;
        const std::string _toolTip;
        const std::string _options;
        const std::string _default;
        int _option = 0;
        bool _toggle = false;
        GuiState _state = STATE_NORMAL;
        GuiStateTimeout _timeout = 0;
    };

    // Server config
    static WorldEntry _worldEntries[NumWorldEntries] =
    {
        {World::TerrainAspect,       "Custom game, Terrain aspect",                          "smooth;normal;rocky",                      "normal",  1, false, STATE_NORMAL, 300},
        {World::TerrainHeight,       "Custom game, Terrain height",                          "flat;normal;varied",                       "normal",  1, false, STATE_NORMAL, 300},
        {World::StartingSeason,      "Custom game, Season to start in",                      "spring;summer;autumn;winter",              "spring",  0, false, STATE_NORMAL, 300},
        {World::YearLength,          "Custom game, Year length",                             "minimum;reduced;default;extended;maximum", "default", 2, false, STATE_NORMAL, 300},
        {World::Precipitation,       "Custom game, Weather setting, 0 (sunny) to 6 (soggy)", "0;1;2;3;4;5;6",                            "3",       3, false, STATE_NORMAL, 300},
        {World::DayLength,           "Custom game, Day setting",                             "minimum;reduced;default;extended;maximum", "default", 2, false, STATE_NORMAL, 300},
        {World::StructureDecay,      "Custom game, Structure decay",                         "low;medium;high",                          "medium",  1, false, STATE_NORMAL, 300},
        {World::InvasionDifficulty,  "Custom game, Invasion difficulty",                     "off;easy;normal;viking",                   "normal",  2, false, STATE_NORMAL, 300},
        {World::MonsterDensity,      "Custom game, Monster density",                         "off;low;medium;high",                      "medium",  2, false, STATE_NORMAL, 300},
        {World::MonsterPopulation,   "Custom game, Monster population",                      "low;medium;high",                          "medium",  1, false, STATE_NORMAL, 300},
        {World::WulfarPopulation,    "Custom game, Wulfar population",                       "low;medium;high",                          "medium",  1, false, STATE_NORMAL, 300},
        {World::HerbivorePopulation, "Custom game, Herbivore population",                    "low;medium;high",                          "medium",  1, false, STATE_NORMAL, 300},
        {World::BearPopulation,      "Custom game, Bear population",                         "low;medium;high",                          "medium",  1, false, STATE_NORMAL, 300},
    };


    void handleWorldEntries(int x, int y, int startEntry, int endEntry)
    {
        for(auto i=endEntry; i>=startEntry; i--)
        {
            if(!getMiscOptions(EnableWorldGen)  ||  _worldSettings.find(_worldEntries[i]._type) == _worldSettings.end()) GuiSetState(STATE_DISABLED);

            if(_worldEntries[i]._toggle) GuiUnlock();

            if(GuiDropdownBox({float(x), float(y) + float(i-startEntry)*30, 180, 25}, _worldEntries[i]._options.c_str(), &_worldEntries[i]._option, _worldEntries[i]._toggle, nullptr))
            {
                _worldEntries[i]._toggle = !_worldEntries[i]._toggle;
            }
            if(_worldEntries[i]._toggle  &&  getMiscOptions(EnableMenusTimeout))
            {
                _worldEntries[i]._timeout.update(_worldEntries[i]._toggle);
                (_worldEntries[i]._toggle) ? GuiLock() : GuiUnlock(); // lock out other controls
            }

            _worldEntries[i]._state = GuiGetLocalState();

            GuiSetState(STATE_NORMAL);
        }
    }

    void handleWorldEntries()
    {
        handleWorldEntries(30 + 210*0, 85, 0, 3);
        handleWorldEntries(30 + 210*1, 85, 4, 7);
        handleWorldEntries(30 + 210*2, 85, 8, 11);
        handleWorldEntries(30 + 210*3, 85, 12, 12);
    }

    void handleWorldTooltips(int x, int y, int startEntry, int endEntry)
    {
        // No tooltips if any control is pressed
        for(auto i=startEntry; i<=endEntry; i++)
        {
            if(_worldEntries[i]._state == STATE_PRESSED) return;
        }

        GuiEnableTooltip();
        for(auto i=startEntry; i<=endEntry; i++)
        {
            // Skip control that is in focus
            if(_worldEntries[i]._state != STATE_FOCUSED) continue;

            GuiSetTooltip(_worldEntries[i]._toolTip.c_str());
            GuiTooltip({float(x), float(y) + float(i-startEntry)*30, 180, 20});
        }
        GuiDisableTooltip();
    }

    void handleWorldTooltips()
    {
        handleWorldTooltips(30 + 210*0, 85, 0, 3);
        handleWorldTooltips(30 + 210*1, 85, 4, 7);
        handleWorldTooltips(30 + 210*2, 85, 8, 11);
        handleWorldTooltips(30 + 210*3, 85, 12, 12);
    }

    std::string getWorldGenFile()
    {
        char* appdata = getenv("APPDATA");
        std::string saves = appdata + std::string("/../") + getAskaConfig(AskaSavePath);
        return saves + "/server/savegame_" + getServerConfig(SaveId) + "/worldgen";
    }

    bool backupWorld()
    {
        std::string backup = getDediConfig(InstallPath) + "/" + getDediConfig(BackupFolder) + "/" + getServerConfig(SaveId);

        bool found = Util::fileExists(backup + "/worldgen");
        if(!found)
        {
            Win::createFolder(backup);
            std::string worldGen = getWorldGenFile();
            if(!Win::copyFile(worldGen, backup + "/worldgen", true)) return false;
            log(Util::Success, stderr, _f, _F, _L, "Backed up %s to %s", worldGen.c_str(), backup.c_str());
        }

        return true;
    }

    void updateWorldSettings()
    {
        for(int i=0; i<NumWorldEntries; i++)
        {
            if(_worldSettings.find(_worldEntries[i]._type) == _worldSettings.end()) continue;

            _worldSettings[_worldEntries[i]._type] = _worldEntries[i]._option;
        }
    }

    bool saveWorldProperties()
    {
        if(_worldSettings.size() == 0)
        {
            log(Util::WarnError, stderr, _f, _F, _L, "World Generation settings are invalid");
            return false;
        }

        std::string worldGen = getWorldGenFile();

        World::setWorldGen(_worldSettings);
        World::saveWorldGen(worldGen);

        return true;
    }

    void handleSaveWorld()
    {
        static bool save = false;
        if(!getMiscOptions(EnableWorldGen)) GuiSetState(STATE_DISABLED);
        if(GuiButton({770, 755, 90, 20}, "Save")) save = !save;
        if(save)
        {
            save = false;
            if(getMiscOptions(EnableWorldGen))
            {
                backupWorld();
                updateWorldSettings();
                saveWorldProperties();
            }
        }
        if(!getMiscOptions(EnableWorldGen)) GuiSetState(STATE_NORMAL);
    }

    bool loadWorldProperties()
    {
        std::string worldGen = getWorldGenFile();
        bool fileExists = Util::fileExists(worldGen);
        if(!fileExists)
        {
            log(Util::FatalError, stderr, _f, _F, _L, "Couldn't find World Generation file %s", worldGen.c_str());
            return false;
        }

        //worldGen = "C:/Users/at67.DADMASTERBEAST.000/AppData/LocalLow/Sand Sailor Studio/Aska/data/savegame_234b9_210824040035/worldgen";
        World::loadWorldGen(worldGen);
        World::getWorldGen(_worldSettings);

        return true;
    }

    void updateWorldEntries()
    {
        for(int i=0; i<NumWorldEntries; i++)
        {
            if(_worldSettings.find(_worldEntries[i]._type) == _worldSettings.end()) continue;

            _worldEntries[i]._option = _worldSettings[_worldEntries[i]._type];
        }
    }

    void handleLoadWorld()
    {
        static bool load = false;
        if(!getMiscOptions(EnableWorldGen)) GuiSetState(STATE_DISABLED);
        if(GuiButton({670, 755, 90, 20}, "Load")) load = !load;
        if(load)
        {
            load = false;
            if(getMiscOptions(EnableWorldGen))
            {
                loadWorldProperties();
                updateWorldEntries();
            }
        }
        if(!getMiscOptions(EnableWorldGen)) GuiSetState(STATE_NORMAL);
    }

    void handleWorldButtons()
    {
        handleLoadWorld();
        handleSaveWorld();
    }

    void handleWorld()
    {
        GuiPanel({10.0f, 40.0f, 850.0f, 705.0f}, "World");

        handleWorldButtons();
    }
}

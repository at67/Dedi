#include <util.h>
#include <gui.h>
#include <world.h>
#include <access.h>

#include <fstream>
#include <unordered_map>


#define WORLD_OFFSET0_INDEX  0
#define WORLD_OFFSET1_INDEX  7

#define SETTING_TYPE_INDEX   10
#define SETTING_VALUE_INDEX  24


namespace World
{
    const std::unordered_map<SettingsType, std::string> SettingsNames = 
    {
        {TerrainAspect, "TerrainAspect"}, {TerrainHeight, "TerrainHeight"}, {StartingSeason, "StartingSeason"}, {YearLength, "YearLength"}, {Precipitation, "Precipitation"}, {DayLength, "DayLength"},
        {StructureDecay, "StructureDecay"}, {InvasionDifficulty, "InvasionDifficulty"}, {MonsterDensity, "MonsterDensity"}, {MonsterPopulation, "MonsterPopulation"}, {WulfarPopulation, "WulfarPopulation"},
        {HerbivorePopulation, "HerbivorePopulation"}, {BearPopulation, "BearPopulation"}
    };


    static int _settingsOffset = 0;
    static int _settingsCount  = 0;
    static int _settingsStart  = 0;
    static int _settingsEnd    = 0;

    static uint32_t _settingsLength = 0;

    static std::vector<char> _buffer;
    static std::vector<char> _seed;


    const std::vector<char>& getSeed() {return _seed;}


    void createSeed()
    {
        // Seed
        uint8_t seedLength = _buffer[0x0E];
        int seedEnd = 0x0F + seedLength;

        _seed.clear();
        _seed.resize(seedLength + 1);

        strncpy(&_seed[0], &_buffer[0x0F], seedLength);
        _seed[seedLength] = 0;
    }

    bool loadWorldGen(const std::string& worldGen)
    {
        std::ifstream infile(worldGen, std::ios::binary | std::ios::in);
        if(!infile.is_open())
        {
            log(Util::FatalError, stderr, _f, _F, _L, "Couldn't read World Generation file %s", worldGen.c_str());
            return false;
        }

        // Read entire worldgen file into a buffer
        infile.seekg(0, std::ios_base::end);
        auto length = infile.tellg();
        infile.seekg(0, std::ios_base::beg);
        _buffer.clear();
        _buffer.resize(length);
        infile.read(&_buffer[0], length);
        infile.close();

        // Seed
        createSeed();

        log(Util::Success, stderr, _f, _F, _L, "Loaded World Generation file %s", worldGen.c_str());

        return true;
    }

    bool saveWorldGen(const std::string& worldGen)
    {
        std::ofstream outfile(worldGen, std::ios::binary | std::ios::out);
        if(!outfile.is_open())
        {
            log(Util::FatalError, stderr, _f, _F, _L, "Couldn't write World Generation file %s", worldGen.c_str());
            return false;
        }

        outfile.write(&_buffer[0], _buffer.size());
        outfile.close();

        log(Util::Success, stderr, _f, _F, _L, "Saved World Generation file %s", worldGen.c_str());

        return true;
    }

    int searchReverse(const std::vector<char>& buffer, const std::vector<char> marker, int offset)
    {
        int start = offset - int(marker.size());
        if(start <= 0) return -1;

        int pos = -1;
        for(int i=start; i>=0; i--)
        {
            for(int j=0; j<marker.size(); j++)
            {
                if(buffer[i + j] != marker[j]) break;
                if(j == marker.size() - 1) pos = i; 
            }
        }

        return pos;
    }

    bool getWorldGen(std::unordered_map<SettingsType, char>& settings)
    {
        settings.clear();

        // Find marker 0x00 0x02 0x09 0x00
        uint32_t offset = Util::getUint32(&_buffer[WORLD_OFFSET0_INDEX]);
        std::vector<char> marker = {0x00, 0x02, 0x09, 0x00};
        int markerPos = searchReverse(_buffer, marker, offset);
        if(markerPos == -1)
        {
            log(Util::FatalError, stderr, _f, _F, _L, "Worldgen does not contain marker");
            return false;
        }

        // Settings length, (subtract 5 bytes for footer)
        _settingsOffset = markerPos + int(marker.size());
        _settingsLength = Util::getUint32(&_buffer[_settingsOffset]) - 5;
        if(_settingsLength % SETTING_LENGTH != 0)
        {
            log(Util::FatalError, stderr, _f, _F, _L, "Worldgen has settings of incorrect size");
            return false;
        }

        // Settings count
        _settingsCount = _settingsLength / SETTING_LENGTH;
        _settingsStart = _settingsOffset + 4;
        _settingsEnd = _settingsStart + _settingsLength;

        // Settings
        for(int i=0; i<_settingsCount; i++)
        {
            SettingsType settingsType = SettingsType(_buffer[_settingsStart + SETTING_TYPE_INDEX + i*SETTING_LENGTH]);
            char settingsValue = _buffer[_settingsStart + SETTING_VALUE_INDEX + i*SETTING_LENGTH];
            settings[settingsType] = settingsValue;
        }

        if(settings.size() == 0) log(Util::WarnError, stderr, _f, _F, _L, "Worldgen has no editable settings");

        return settings.size() > 0;
    }

    bool setWorldGen(const std::unordered_map<SettingsType, char>& settings)
    {
        if(_settingsCount != settings.size())
        {
            log(Util::FatalError, stderr, _f, _F, _L, "Worldgen settings mismatch");
            return false;
        }

        for(int i=0; i<_settingsCount; i++)
        {
            SettingsType settingsType = SettingsType(_buffer[_settingsStart + SETTING_TYPE_INDEX + i*SETTING_LENGTH]);
            if(settings.find(settingsType) == settings.end())
            {
                log(Util::FatalError, stderr, _f, _F, _L, "Worldgen settings mismatch");
                return false;
            }

            _buffer[_settingsStart + SETTING_VALUE_INDEX + i*SETTING_LENGTH] = settings.at(settingsType);
        }

        return true;
    }

    bool setSetting(SettingsType type, char value)
    {
        for(int i=0; i<_settingsCount; i++)
        {
            SettingsType settingsType = SettingsType(_buffer[_settingsStart + SETTING_TYPE_INDEX + i*SETTING_LENGTH]);
            if(settingsType == type)
            {
                _buffer[_settingsStart + SETTING_VALUE_INDEX + i*SETTING_LENGTH] = value;
                return true;
            }
        }

        return false;
    }

    bool addSetting(SettingsType type, char value)
    {
        // Custom games will have one or more settings
        if(_settingsCount == 0) return false;

        std::vector<char> settings =
        {
            0x01, 0x00, 0x00, 0x1B, 0x00, 0x00, 0x00, 0x03, 0x05, 0x00, char(type), 0x00, 0x00, 0x00, 0x01, 0x06, 0x00, 0x0C, 0x00, 0x00, 0x00, 0x03, 0x0A, 0x00, value, 0x00, 0x00, 0x00, 0x00, 0x00
        };

        for(int i=0; i<int(settings.size()); i++)
        {
            _buffer.insert(_buffer.begin() + _settingsStart + i, settings[i]);
        }

        // Add setting length to offset0, (taking into account host endianess)
        Util::addUint32(&_buffer[WORLD_OFFSET0_INDEX], SETTING_LENGTH);

        // Add setting length to offset1, (taking into account host endianess)
        Util::addUint32(&_buffer[WORLD_OFFSET1_INDEX], SETTING_LENGTH);

        // Add setting length to settings length, (taking into account host endianess)
        Util::addUint32(&_buffer[_settingsOffset], SETTING_LENGTH);

        return true;
    }
}
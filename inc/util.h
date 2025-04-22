#pragma once

#include <cstdint>
#include <string>
#include <vector>


#define MAX_STR_TEXT 1024

#define _F __FILE__
#define _L __LINE__
#define _f __func__


namespace Util
{
    const std::string redANSI = "\033[3;41;30m";
    const std::string grnANSI = "\033[3;42;30m";
    const std::string yloANSI = "\033[3;43;30m";
    const std::string whtANSI = "\033[0m";

    enum LogType {NoError=0, Success, WarnError, FatalError}; 

    template <typename ... Args> void log(LogType error, FILE* file, const char* FUNC, const char* FILE, int LINE, const char* format, Args ... args)
    {
        switch(error)
        {
            case NoError:    fprintf(file, "%s", whtANSI.c_str()); break;
            case Success:    fprintf(file, "%s", grnANSI.c_str()); break;
            case WarnError:  fprintf(file, "%s", yloANSI.c_str()); break;
            case FatalError: fprintf(file, "%s", redANSI.c_str()); break;

            default: fprintf(file, "%s", whtANSI.c_str()); break;
        }

        if(error == FatalError)
        {
            fprintf(file, "%s() : ", FUNC);
            fprintf(file, "%s : %d : ", FILE, LINE);
        }
        fprintf(file, format, args ...);
        fprintf(file, "%s\n", whtANSI.c_str());
        fflush(file);

        char status[MAX_STR_TEXT] = "";
        sprintf(status, format, args ...);
        logStatus(status);
    }

    template<typename T> inline T clamp(const T val, const T min, const T max)
    {
        return std::min(std::max(val, min), max);
    }


    enum Endianness {LittleEndian=0x03020100ul, BigEndian=0x00010203ul};

    Endianness getEndianness(void);
    uint16_t getUint16(uint16_t value);
    uint32_t getUint32(uint32_t value);
    uint64_t getUint64(uint64_t value);
    uint16_t getUint16(char* buffer);
    uint32_t getUint32(char* buffer);
    uint64_t getUint64(char* buffer);
    void addUint16(char* buffer, uint16_t x);
    void addUint32(char* buffer, uint32_t x);
    void addUint64(char* buffer, uint64_t x);

    void sleep_ms(uint64_t ms);

    std::string getDateTime();

    void logStatus(const std::string& text);

    bool fileExists(const std::string& filename);
    bool pathExists(const std::string& pathname);

    std::string& strip(std::string& text);
    std::string& lower(std::string& text);
    std::string& rtrim(std::string& text, char chr=' ');

    int stricmp(const char* src1, const char* src2);
    char* strcpy(char* dst, const std::string& src, int maxLength, const char* FILE, int LINE);

    bool matchFileText(const std::string& filename, const std::string& text, std::string& match, bool mangle=false);

    std::vector<uint8_t> decompressZlibData(const std::vector<uint8_t>& compressed_data, const char* FILE, int LINE);
    bool decompressZLibArchive(const std::string& archive, const std::string& outputPath, const char* FILE, int LINE);
}
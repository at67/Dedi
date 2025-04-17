#include <miniz.h>

#include <util.h>
#include <win.h>
#include <status.h>

#include <fstream>
#include <iostream>
#include <chrono>
#include <thread>
#include <algorithm>

#include <sys/stat.h>


namespace Util
{
    static const uint8_t _endianBytes[] = {0x00, 0x01, 0x02, 0x03};

    Endianness getEndianness(void)
    {
        return *((Endianness*)_endianBytes);
    }

    static const bool _isBigEndian = (getEndianness() == BigEndian);


    uint16_t getUint16(uint16_t val)
    {
        uint16_t value = _isBigEndian ? (val >>8)  |  (val <<8) : val;
        return value;
    }

    uint32_t getUint32(uint32_t val)
    {
        uint32_t value = _isBigEndian ? (val >>24)  |  ((val >>8) & 0x0000FF00)  |  ((val <<8) & 0x00FF0000)  |  (val <<24) : val;
        return value;
    }

    uint64_t getUint64(uint64_t val)
    {
        uint64_t value = _isBigEndian ? (val >>56)  |  ((val >>40) & 0x000000000000FF00LL)  |  ((val >>24) & 0x0000000000FF0000LL)  |  ((val >>8) & 0x00000000FF000000LL)  |
                                        ((val <<8) & 0x000000FF00000000LL)  |  ((val <<24) & 0x0000FF0000000000LL)  |  ((val <<40) & 0x00FF000000000000LL)  |  (val <<56) : val;
        return value;
    }

    uint16_t getUint16(char* buffer)
    {
        return getUint16(*reinterpret_cast<uint16_t*>(buffer));
    }

    uint32_t getUint32(char* buffer)
    {
        return getUint32(*reinterpret_cast<uint32_t*>(buffer));
    }

    uint64_t getUint64(char* buffer)
    {
        return getUint64(*reinterpret_cast<uint64_t*>(buffer));
    }

    void addUint16(char* buffer, uint16_t x)
    {
        uint16_t value = Util::getUint16(*reinterpret_cast<uint16_t*>(buffer)) + x;
        *reinterpret_cast<uint16_t*>(buffer) = Util::getUint16(value);
    }

    void addUint32(char* buffer, uint32_t x)
    {
        uint32_t value = Util::getUint32(*reinterpret_cast<uint32_t*>(buffer)) + x;
        *reinterpret_cast<uint32_t*>(buffer) = Util::getUint32(value);
    }

    void addUint64(char* buffer, uint64_t x)
    {
        uint64_t value = Util::getUint64(*reinterpret_cast<uint64_t*>(buffer)) + x;
        *reinterpret_cast<uint64_t*>(buffer) = Util::getUint64(value);
    }

    void sleep_ms(uint64_t ms)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    }

    void logStatus(const std::string& text)
    {
        Status::addText(text);
    }

    bool fileExists(const std::string& filename)
    {
        struct stat info;
        if(stat(filename.c_str(), &info) == -1) return false;
        if((info.st_mode & S_IFMT) == 0) return false;
        return true;
    }

    bool pathExists(const std::string& pathname)
    {
        struct stat info;
        if(stat(pathname.c_str(), &info) == -1) return false;
        if((info.st_mode & S_IFDIR) == 0) return false;
        return true;
    }

    std::string& strip(std::string& text)
    {
        text.erase(remove_if(text.begin(), text.end(), ::isspace), text.end());
        return text;
    }

    std::string& rtrim(std::string& text)
    {
        text.erase(text.find_last_not_of(" \n\r\t") + 1);
        return text;
    }

    std::string& lower(std::string& text)
    {
        std::transform(text.begin(), text.end(), text.begin(), ::tolower);
        return text;
    }

    int stricmp(const char* src1, const char* src2)
    {
        std::string s1 = src1;
        std::string s2 = src2;
        lower(s1);
        lower(s2);
        return (s1 > s2) - (s1 < s2);
    }

    char* strcpy(char* dst, const std::string& src, int maxLength, const char* FILE, int LINE)
    {
        if(dst == nullptr)
        {
            log(FatalError, stderr, _f, FILE, LINE, "bad dst");
            return nullptr;
        }

        int length = src.size() % maxLength;
        if(length) strncpy(dst, src.c_str(), length);
        dst[length] = 0;
        return dst;
    }

    bool matchFileText(const std::string& filename, const std::string& text, std::string& match, bool mangle)
    {
        std::ifstream file(filename);
        if(!file.is_open()) return false;

        while(!file.eof()  &&  file.good())
        {
            std::string line;
            std::getline(file, line);
            if(mangle)
            {
                strip(line);
                lower(line);
            }
            size_t pos = line.find(text);
            if(pos != std::string::npos)
            {
                match = line.substr(pos);
                return true;
            }
        }

        return false;
    }

    std::vector<uint8_t> decompressZlibData(const std::vector<uint8_t>& compressedData, const char* FILE, int LINE)
    {
        z_stream stream;
        std::vector<uint8_t> uncompressedData;
        
        // Initialize zlib
        stream.zalloc = Z_NULL;
        stream.zfree = Z_NULL;
        stream.opaque = Z_NULL;
        stream.avail_in = static_cast<uInt>(compressedData.size());
        stream.next_in = const_cast<Bytef*>(compressedData.data());
        
        if(inflateInit(&stream) != Z_OK)
        {
            log(FatalError, stderr, _f, FILE, LINE, "Failed to initialize Zlib");
            return uncompressedData;
        }
        
        // Decompress the data
        const size_t chunk_size = 16384;
        std::vector<uint8_t> chunk(chunk_size);
        do
        {
            stream.avail_out = chunk_size;
            stream.next_out = chunk.data();
            
            if(inflate(&stream, Z_NO_FLUSH) == Z_STREAM_ERROR)
            {
                log(FatalError, stderr, _f, FILE, LINE, "Zlib decompression error");
                inflateEnd(&stream);
                return uncompressedData;
            }
            
            size_t have = chunk_size - stream.avail_out;
            uncompressedData.insert(uncompressedData.end(), chunk.begin(), chunk.begin() + have);
        }
        while(stream.avail_out == 0);
        
        inflateEnd(&stream);
        return uncompressedData;
    }

    bool decompressZLibArchive(const std::string& archive, const std::string& outputPath, const char* FILE, int LINE)
    {
        mz_zip_archive zipArchive;
        memset(&zipArchive, 0, sizeof(zipArchive));

        if(!mz_zip_reader_init_file(&zipArchive, archive.c_str(), 0))
        {
            log(FatalError, stderr, _f, FILE, LINE, "Invalid zip archive");
            return false;
        }

        mz_uint numFiles = mz_zip_reader_get_num_files(&zipArchive);
        if(numFiles == 0)
        {
            log(FatalError, stderr, _f, FILE, LINE, "Empty zip archive");
            return false;
        }

        // Add some measure of hacking protection
        if(numFiles != 1)
        {
            log(FatalError, stderr, _f, FILE, LINE, "Bad zip archive");
            return false;
        }

        for(mz_uint i=0; i<numFiles; i++)
        {
            mz_zip_archive_file_stat fileStat;

            // Entry stats
            if(!mz_zip_reader_file_stat(&zipArchive, i, &fileStat))
            {
                log(FatalError, stderr, _f, FILE, LINE, "Zip file stat error");
                mz_zip_reader_end(&zipArchive);
                return false;
            }

            // Add some measure of hacking protection
            if(std::string(fileStat.m_filename) != "steamcmd.exe")
            {
                log(FatalError, stderr, _f, FILE, LINE, "Bad zip content");
                return false;
            }

            std::string path = outputPath + '/' + fileStat.m_filename;

            // Folder
            if(mz_zip_reader_is_file_a_directory(&zipArchive, i))
            {
                if(!Win::createDirectory(path)) return false;
                continue;
            }

            // File
            if(!mz_zip_reader_extract_to_file(&zipArchive, i, path.c_str(), 0))
            {
                log(FatalError, stderr, _f, FILE, LINE, "Zip file write error : %d : %s", i, path.c_str());
                mz_zip_reader_end(&zipArchive);
                return false;
            }
        }

        if(!mz_zip_reader_end(&zipArchive))
        {
            log(FatalError, stderr, _f, FILE, LINE, "Zip shutdown error");
            return false;
        }

        return true;
    }
}
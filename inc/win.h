#pragma once

#include <set>
#include <string>
#include <vector>


namespace Win
{
    void initialise();

    void setDarkMode(void* handle);

    bool getLastErrorStr(std::string& errorStr);

    bool shellExecute(const std::string& url);
    bool downloadLink(const std::string& url, const std::string& file);

    bool delFile(const std::string& path);
    bool moveFile(const std::string& src, const std::string& dst, bool overwrite=false);
    bool copyFile(const std::string& src, const std::string& dst, bool overwrite=false);

    int getFileCount(const std::string& path);
    int getFolderCount(const std::string& path);

    bool createFolder(const std::string& path);
    bool copyFolder(const std::string& src, const std::string& dst);
    bool delFolder(const std::string& path);

    bool getFileNames(const std::string& path, const std::string& search, std::vector<std::string>& files);
    bool getFolderNames(const std::string& path, const std::string& search, std::set<std::string>& folders);

    bool createProcess(const std::string& name, const std::string& command);
    bool waitProcess(int ms=0);
    bool endProcess();

    bool readConsoleText(std::vector<std::string>& text);
    size_t matchConsoleText(const std::string& match, std::string& line, bool erase=false);
    void sendConsoleText(const std::string& text);

    std::string getHKLMRegStr(const std::string& subKey, const std::string& value);
}

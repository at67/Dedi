#pragma once

#include <string>
#include <vector>


namespace Win
{
    void initialise();

    bool getLastErrorStr(std::string& errorStr);

    bool shellExecute(const std::string& url);
    bool downloadLink(const std::string& url, const std::string& file);

    bool moveFile(const std::string& src, const std::string& dst, bool overwrite=false);
    bool copyFile(const std::string& src, const std::string& dst, bool overwrite=false);
    bool createDirectory(const std::string& path);

    bool getFilenames(const std::string& path, const std::string& search, std::vector<std::string>& filenames);

    bool createProcess(const std::string& name, const std::string& command);
    bool endProcess();

    bool readConsoleText(std::vector<std::string>& text);
    size_t matchConsoleText(const std::string& match, std::string& line, bool erase=false);
    void sendConsoleText(const std::string& text);

    std::string getHKLMRegStr(const std::string& subKey, const std::string& value);
}

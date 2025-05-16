#include <win.h>
#include <util.h>

#include <windows.h>
#include <shellapi.h>
#include <Urlmon.h>
#include <dwmapi.h>

#include <dirent.h>

#include <vector>
#include <string>
#include <algorithm>


namespace Win
{
    // We are guaranteed to create only one process at a time!
    static _PROCESS_INFORMATION _processInfo;

    static std::vector<std::string> _consoleText;


    void initialise()
    {
        SetConsoleCtrlHandler(nullptr, true);
        SetEnvironmentVariable("SteamAppId", "1898300");
    }

    void setDarkMode(void* handle)
    {
        HWND hWnd = HWND(handle);
        BOOL value = TRUE;
        ::DwmSetWindowAttribute(hWnd, 20, &value, sizeof(value));
    }

    bool getLastErrorStr(std::string& error)
    {
        char* buffer = nullptr;
        if(FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, GetLastError(), LANG_SYSTEM_DEFAULT, (LPTSTR)&buffer, 0, nullptr) == 0)
        {
            log(Util::FatalError, stderr, _f, _F, _L, "FormatMessage() : FAILED");
            return false;
        }

        // Remove newlines
        error = buffer;
        Util::rtrim(error, 0);
        LocalFree(buffer);

        return true;
    }

    bool shellExecute(const std::string& url)
    {
        // https://learn.microsoft.com/en-us/windows/win32/api/shellapi/nf-shellapi-shellexecutea
        const auto kShellExecuteErrorLimit = 32; 

        if(CoInitialize(nullptr) != S_OK)
        {
            log(Util::FatalError, stderr, _f, _F, _L, "%s", "CoInitialize() : failed!");
            return false;
        }

        SHELLEXECUTEINFO info{sizeof(info)};
        info.fMask  = SEE_MASK_NOASYNC; // because we exit process after ShellExecuteEx()
        info.lpVerb = "open";
        info.lpFile = url.c_str();
        info.nShow  = SW_SHOWDEFAULT;
        if(!ShellExecuteEx(&info)) 
        {
            std::string errorStr;
            if(getLastErrorStr(errorStr)) log(Util::FatalError, stderr, _f, _F, _L, "%s", errorStr.c_str());
            return false;
        }

        CoUninitialize();

        return true;
    }

    bool downloadLink(const std::string& url, const std::string& file)
    {
        HRESULT result = URLDownloadToFile(nullptr, url.c_str(), file.c_str(), BINDF_GETNEWESTVERSION, nullptr);
        if(result == S_OK) return true;

        switch(result)
        {
            case E_OUTOFMEMORY:           log(Util::FatalError, stderr, _f, _F, _L, "OUT_OF_MEMORY");   break;
            case INET_E_DOWNLOAD_FAILURE: log(Util::WarnError,  stderr, _f, _F, _L, "DOWNLOAD_FAILED"); break;

            default: log(Util::FatalError, stderr, _f, _F, _L, "UNKNOWN ERROR"); break;
        }

        return false;
    }

    bool delFile(const std::string& path)
    {
        return DeleteFile(path.c_str());
    }

    bool moveFile(const std::string& src, const std::string& dst, bool overwrite)
    {
        return MoveFileEx(src.c_str(), dst.c_str(), overwrite ? MOVEFILE_REPLACE_EXISTING : 0);
    }

    bool copyFile(const std::string& src, const std::string& dst, bool overwrite)
    {
        return CopyFile(src.c_str(), dst.c_str(), !overwrite);
    }

    int getFileCount(const std::string& path)
    {
        DIR* dir;
        if((dir = opendir(path.c_str())) == nullptr) return 0;

        int count = 0;
        struct dirent* ent;
        while((ent = readdir(dir)) != nullptr)
        {
            if(ent->d_type == DT_DIR) continue;

            if(ent->d_type == DT_REG) count++;
        }

        closedir(dir);

        return count;
    }

    int getFolderCount(const std::string& path)
    {
        DIR* dir;
        if((dir = opendir(path.c_str())) == nullptr) return 0;

        int count = 0;
        struct dirent* ent;
        while((ent = readdir(dir)) != nullptr)
        {
            if(ent->d_type == DT_REG) continue;

            if(ent->d_type == DT_DIR)
            {
                if(std::string(ent->d_name) == "."  ||  std::string(ent->d_name) == "..") continue;
                count++;
            }
        }

        closedir(dir);

        return count;
    }

    bool createFolder(const std::string& path)
    {
        bool result = CreateDirectory(path.c_str(), nullptr);
        if(!result)
        {
            switch(GetLastError())
            {
                case ERROR_ALREADY_EXISTS: return true; // we're ok with this
                case ERROR_PATH_NOT_FOUND: log(Util::WarnError, stderr, _f, _F, _L, "ERROR_PATH_NOT_FOUND"); break;

                default: log(Util::FatalError, stderr, _f, _F, _L, "UNKNOWN ERROR"); break;
            }
        }

        return result;
    }

    // Only copies files in a folder, NOT sub-folders!
    bool copyFolder(const std::string& src, const std::string& dst)
    {
        DIR* dir;
        if((dir = opendir(src.c_str())) == nullptr) return false;

        struct dirent* ent;
        while((ent = readdir(dir)) != nullptr)
        {
            if(ent->d_type == DT_DIR) continue;

            if(ent->d_type == DT_REG)
            {
                std::string file = std::string(ent->d_name);
                copyFile(src + "/" + file, dst + "/" + file, true);
            }
        }

        closedir(dir);

        return true;
    }

    // Deletes all files in a folder and the folder itself, (if it can), but NOT sub-folders!
    bool delFolder(const std::string& path)
    {
        DIR* dir;
        if((dir = opendir(path.c_str())) == nullptr) return false;

        struct dirent* ent;
        while((ent = readdir(dir)) != nullptr)
        {
            if(ent->d_type == DT_DIR) continue;

            if(ent->d_type == DT_REG)
            {
                std::string file = std::string(ent->d_name);
                delFile(path + "/" + file);
            }
        }

        closedir(dir);

        return RemoveDirectory(path.c_str());
    }

    // Returns file names WITHOUT search terms, (i.e trimming extensions)
    bool getFileNames(const std::string& path, const std::string& search, std::vector<std::string>& files)
    {
        DIR* dir;
        if((dir = opendir(path.c_str())) == nullptr) return false;

        files.clear();

        struct dirent* ent;
        while((ent = readdir(dir)) != nullptr)
        {
            if(ent->d_type == DT_DIR) continue;

            if(ent->d_type == DT_REG)
            {
                std::string filename = std::string(ent->d_name);
                Util::lower(filename);
                
                std::string ext = search;
                Util::lower(ext);
                size_t pos = filename.find(ext);
                if(pos != std::string::npos) files.push_back(filename.substr(0, pos));
            }
        }

        closedir(dir);

        return !(files.size() == 0);
    }

    // Returns FULL folder names
    bool getFolderNames(const std::string& path, const std::string& search, std::set<std::string>& folders)
    {
        DIR* dir;
        if((dir = opendir(path.c_str())) == nullptr) return false;

        folders.clear();

        struct dirent* ent;
        while((ent = readdir(dir)) != nullptr)
        {
            if(ent->d_type == DT_REG) continue;

            if(ent->d_type == DT_DIR)
            {
                if(std::string(ent->d_name) == "."  ||  std::string(ent->d_name) == "..") continue;

                std::string folder = std::string(ent->d_name);
                Util::lower(folder);
                
                std::string pat = search;
                Util::lower(pat);
                size_t pos = folder.find(pat);
                if(pos != std::string::npos) folders.insert(folder);
            }
        }

        closedir(dir);

        return !(folders.size() == 0);
    }

    bool createProcess(const std::string& name, const std::string& commandLine)
    {
        std::string error;
        STARTUPINFO startupInfo;

        memset(&startupInfo, 0, sizeof(STARTUPINFO));
        startupInfo.cb = sizeof(STARTUPINFO);
        memset(&_processInfo, 0, sizeof(PROCESS_INFORMATION));

        if(commandLine.size())
        {
            std::vector<char> args(MAX_STR_TEXT + 1);
            Util::strcpy(&args[0], commandLine, MAX_STR_TEXT, _F, _L);
            if(!CreateProcess(nullptr, &args[0], nullptr, nullptr, false, 0, nullptr, nullptr, &startupInfo, &_processInfo))
            {
                if(getLastErrorStr(error)) log(Util::WarnError, stderr, _f, _F, _L, "%s", error.c_str());
                return false;
            }
        }
        else
        {
            if(!CreateProcess(name.c_str(), nullptr, nullptr, nullptr, false, 0, nullptr, nullptr, &startupInfo, &_processInfo))
            {
                if(getLastErrorStr(error)) log(Util::WarnError, stderr, _f, _F, _L, "%s", error.c_str());
                return false;
            }
        }

        return true;
    }

    bool waitProcess(int ms)
    {
        if(ms == 0) ms = INFINITE;
        DWORD result = WaitForSingleObject(_processInfo.hProcess, DWORD(ms));
        if(result != WAIT_OBJECT_0  &&  result != WAIT_TIMEOUT)
        {
            std::string error;
            if(getLastErrorStr(error)) log(Util::WarnError, stderr, _f, _F, _L, "%s", error.c_str());
            return false;
        }

        return (result == WAIT_OBJECT_0);
    }

    bool endProcess()
    {
        if(!TerminateProcess(_processInfo.hProcess, 0))
        {
            std::string error;
            if(getLastErrorStr(error)) log(Util::WarnError, stderr, _f, _F, _L, "%s", error.c_str());
            return false;
        }

        CloseHandle(_processInfo.hProcess);
        CloseHandle(_processInfo.hThread);

        return true;
    }

    void clearConsoleText()
    {
        _consoleText.clear();
    }

    bool readConsoleText()
    {
        return readConsoleText(_consoleText);
    }

    bool readConsoleText(std::vector<std::string>& text)
    {
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
        int width = csbi.dwSize.X;
        int height = csbi.dwSize.Y;

        DWORD charsRead = 0;
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

        static SHORT cursorY = 0;
        if(csbi.dwCursorPosition.Y == cursorY   &&  csbi.dwCursorPosition.Y < height-1) return false;

        if(csbi.dwCursorPosition.Y < height-1)
        {
            for(SHORT y=cursorY; y<csbi.dwCursorPosition.Y; y++)
            {
                std::vector<char> chars(width + 1);
                ReadConsoleOutputCharacter(hConsole, &chars[0], width, {0, y}, &charsRead);
                chars[charsRead] = 0;
                text.push_back(&chars[0]);
            }
        }
        else
        {
            const int lookAhead = 3;
            static std::string prev;
            std::vector<char> chars(width*lookAhead + 1);
            ReadConsoleOutputCharacter(hConsole, &chars[0], width*lookAhead, {0, SHORT(csbi.dwCursorPosition.Y-lookAhead)}, &charsRead);
            chars[charsRead] = 0;
            if(prev != &chars[0])
            {
                std::string wad = &chars[0];
                for(int i=0; i<lookAhead; i++)
                {
                    std::string line = wad.substr(i*width, width);
                    text.push_back(line);
                }
            }
            prev = &chars[0];
        }

        cursorY = csbi.dwCursorPosition.Y;

        return true;
    }

    size_t matchConsoleText(const std::string& match, std::string& line)
    {
        for(size_t i=0; i<_consoleText.size(); i++)
        {
            size_t pos = _consoleText[i].find(match);
            if(pos != std::string::npos)
            {
                line = _consoleText[i];
                return pos;
            }
        }

        return std::string::npos;
    }

    void createKeyEvent(char key)
    {
        DWORD events;
        INPUT_RECORD inpRec[2];
        inpRec[0].EventType = KEY_EVENT;
        inpRec[0].Event.KeyEvent.bKeyDown = TRUE;
        inpRec[0].Event.KeyEvent.dwControlKeyState = 0;
        inpRec[0].Event.KeyEvent.uChar.AsciiChar = key;
        inpRec[0].Event.KeyEvent.wVirtualKeyCode = key;
        inpRec[0].Event.KeyEvent.wVirtualScanCode = MapVirtualKey(key, MAPVK_VK_TO_VSC);
        inpRec[1] = inpRec[0];
        inpRec[1].Event.KeyEvent.bKeyDown = FALSE;
        WriteConsoleInput(GetStdHandle(STD_INPUT_HANDLE), inpRec, 2, &events);
    }
    void sendConsoleText(const std::string& text)
    {
        for(size_t i=0; i<text.size(); i++)
        {
            createKeyEvent(text[i]);
        }
        createKeyEvent(VK_RETURN);
    }

    std::string getHKLMRegStr(const std::string& subKey, const std::string& value)
    {
        const auto kMaxDataSize = 1024;
        std::vector<char> data(kMaxDataSize);
        DWORD dataSize = kMaxDataSize;

        // Guarantees a null terminated data stream
        if(RegGetValue(HKEY_LOCAL_MACHINE, subKey.c_str(), value.c_str(), RRF_RT_REG_SZ, nullptr, &data[0], &dataSize) != ERROR_SUCCESS)
        {
            log(Util::WarnError, stderr, _f, _F, _L, "RegGetValue() : FAILED");
            return "";
        }

        std::string keyStr = reinterpret_cast<char *>(&data[0]);
        return keyStr;
    }
}

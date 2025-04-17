#include <win.h>
#include <util.h>

#include <windows.h>
#include <shellapi.h>
#include <Urlmon.h>

#include <dirent.h>

#include <vector>
#include <string>
#include <algorithm>


namespace Win
{
    // We are guaranteed to create only one process at a time!
    static _PROCESS_INFORMATION _processInfo;


    void initialise()
    {
        SetConsoleCtrlHandler(nullptr, true);
        SetEnvironmentVariable("SteamAppId", "1898300");
    }

    bool getLastErrorStr(std::string& errorStr)
    {
        const auto kMaxErrorStringSize = 1024;

        DWORD error = GetLastError();
        errorStr.resize(kMaxErrorStringSize);
        std::fill(&errorStr[0], &errorStr[0] + kMaxErrorStringSize - 1, 0);
        if(FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, error, LANG_SYSTEM_DEFAULT, &errorStr[0], kMaxErrorStringSize, nullptr) == 0)
        {
            log(Util::FatalError, stderr, _f, _F, _L, "FormatMessage() : FAILED");
            return false;
        }

        return true;
    }

    bool shellExecute(const std::string& url)
    {
        // https://learn.microsoft.com/en-us/windows/win32/api/shellapi/nf-shellapi-shellexecutea
        const auto kShellExecuteErrorLimit = 32; 

        CoInitialize(nullptr);

        SHELLEXECUTEINFO info{sizeof(info)};
        info.fMask  = SEE_MASK_NOASYNC; // because we exit process after ShellExecuteEx()
        info.lpVerb = "open";
        info.lpFile = url.c_str();
        info.nShow  = SW_SHOWDEFAULT;
        bool result = ShellExecuteEx(&info);
        if(!result) 
        {
            std::string errorStr;
            if(!getLastErrorStr(errorStr)) return false;
            log(Util::FatalError, stderr, _f, _F, _L, "%s", errorStr.c_str());
            return false;
        }

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

    bool moveFile(const std::string& src, const std::string& dst, bool overwrite)
    {
        return MoveFileEx(src.c_str(), dst.c_str(), overwrite ? MOVEFILE_REPLACE_EXISTING : 0);
    }

    bool copyFile(const std::string& src, const std::string& dst, bool overwrite)
    {
        return CopyFile(src.c_str(), dst.c_str(), !overwrite);
    }

    bool createDirectory(const std::string& path)
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

    bool getFilenames(const std::string& path, const std::string& search, std::vector<std::string>& filenames)
    {
        DIR* dir;
        if((dir = opendir(path.c_str())) == nullptr) return false;

        filenames.clear();

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
                size_t epos = filename.find(search);
                if(epos != std::string::npos)
                {
                    filenames.push_back(filename.substr(0, epos));
                }
            }
        }

        closedir(dir);

        if(filenames.size() == 0) return false;

        return true;
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

    bool endProcess()
    {
        bool result = TerminateProcess(_processInfo.hProcess, 0);
        if(!result)
        {
            std::string error;
            if(getLastErrorStr(error)) log(Util::WarnError, stderr, _f, _F, _L, "%s", error.c_str());
            return false;
        }

        const DWORD wait = WaitForSingleObject(_processInfo.hProcess, 500);
        result = (wait == WAIT_OBJECT_0);
        if(!result)
        {
            std::string error;
            if(getLastErrorStr(error)) log(Util::WarnError, stderr, _f, _F, _L, "%s", error.c_str());
            return false;
        }

        CloseHandle(_processInfo.hProcess);
        CloseHandle(_processInfo.hThread);

        return true;
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
            static std::string prev;
            std::vector<char> chars(width + 1);
            ReadConsoleOutputCharacter(hConsole, &chars[0], width, {0, csbi.dwCursorPosition.Y-1}, &charsRead);
            chars[charsRead] = 0;
            if(prev != &chars[0]) text.push_back(&chars[0]);
            prev = &chars[0];
        }

        cursorY = csbi.dwCursorPosition.Y;

        return true;
    }

    size_t matchConsoleText(const std::string& match, std::string& line, bool erase)
    {
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);

        DWORD charsRead = 0;
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        std::vector<char> chars(csbi.dwSize.X + 1);

        for(SHORT y=0; y<csbi.dwSize.Y; y++)
        {
            ReadConsoleOutputCharacter(hConsole, &chars[0], csbi.dwSize.X, {0, y}, &charsRead);
            if(charsRead)
            {
                chars[csbi.dwSize.X] = 0;
                line = &chars[0];
                size_t pos = line.find(match);
                if(pos != std::string::npos)
                {
                    if(erase)
                    {
                        DWORD charsWritten = 0;
                        std::string blank(pos + match.size(), ' ');
                        WriteConsoleOutputCharacter(hConsole, blank.c_str(), DWORD(pos + match.size()), {0, y}, &charsWritten);
                    }
                    return pos;
                }
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

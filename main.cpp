#include <io.h>
#include <fcntl.h>
#include <tchar.h>
#include <windows.h>
#include <iostream>
#include <algorithm>
#include <vector>
#include <string>

std::string query_name_from_pid(DWORD pid) {
    std::string s;
    DWORD dwSize = MAX_PATH;
    s.resize(dwSize);
    auto process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, false, pid);
    QueryFullProcessImageNameA(process, 0, &s[0], &dwSize);
    CloseHandle(process);
    s.resize(dwSize);
    return s;
}

namespace console {

std::vector<DWORD> get_list() {
    std::vector<DWORD> rgpl;
    DWORD siz = 1;
    for (;;) {
        rgpl.resize(siz);
        siz = GetConsoleProcessList(&rgpl[0], siz);
        if (siz > rgpl.size()) {
            continue;
        }
        if(siz <= 0) {
            rgpl.resize(0);
        }
        return rgpl;
    }
}

namespace host {
    enum type {
        Invalid,
        Cmd,
        Powershell,
        PowershellISE
    };
}

//
// get the first in the list - that appears to be the actual context when nesting
//
host::type get_host() {
    for (auto& pid : get_list()) {
        auto s = query_name_from_pid(pid);

        // Name of EXE is PowerShell_ISE.exe
        if(s.find("cmd.exe") != std::string::npos) {
            return host::Cmd;
        }

        if(s.find("powershell_ise.exe") != std::string::npos) {
            return host::PowershellISE;
        }

        if(s.find("powershell.exe") != std::string::npos) {
            return host::Powershell;
        }
    }

    return host::Invalid;
}

bool is_font_true_type() {
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_FONT_INFOEX cfie = {};
    cfie.cbSize = sizeof(cfie);
    if (GetCurrentConsoleFontEx(out, false, &cfie)) {
        return (cfie.FontFamily & TMPF_TRUETYPE) == TMPF_TRUETYPE;
    }
    return false;
}

bool is_redirected() {
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    bool retval = GetConsoleMode(out, &mode);
    if ((retval == false) && (GetLastError() == ERROR_INVALID_HANDLE)) {
        return true;
    }
    return false;
}

}

std::string cp_encode(int cp, const std::wstring &wstr)
{
    int size_needed = WideCharToMultiByte(
        cp, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string to(size_needed, 0);
    WideCharToMultiByte(
        cp, 0, &wstr[0], (int)wstr.size(), &to[0], size_needed, NULL, NULL);
    return to;
}

std::wstring cp_decode(int cp, const std::string &str)
{
    int size_needed = MultiByteToWideChar(
        cp, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring to(size_needed, 0);
    MultiByteToWideChar(
        cp, 0, &str[0], (int)str.size(), &to[0], size_needed);
    return to;
}

void write_console(const std::wstring s) {
    auto outcp = GetConsoleOutputCP();
    if (console::is_redirected()) {
        auto sutf8 = cp_encode(outcp, s);
        HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD written = 0;
        //printf("WF - ");
        WriteFile(out, sutf8.c_str(), sutf8.size(), &written, nullptr);
        return;
    } else if (console::get_host() != console::host::Invalid || console::is_font_true_type()) {
        HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD written = 0;
        //printf("WC - ");
        WriteConsoleW(out, s.c_str(), s.size(), &written, nullptr);
        return;
    }
    auto sutf8 = cp_encode(outcp, s);
    printf(sutf8.c_str());
    return;
}
void write_console(const std::string& s) {
    write_console(cp_decode(CP_UTF8, s));
    return;
}
std::wstring get_command_line() {
    auto outcp = GetConsoleOutputCP();
    if (outcp == CP_UTF8 && console::get_host() > console::host::Cmd) {
        return std::wstring(cp_decode(CP_UTF8, GetCommandLineA()));
    } else {
        return std::wstring(GetCommandLineW());
    }
}

int _tmain(int argc, _TCHAR** argv)
{
#if 0
    auto outcp = GetConsoleOutputCP();
    SetConsoleOutputCP(CP_UTF8);
#endif

#if 0 // UNICODE
    if (outcp == CP_UTF8 && !IsPowerShellIse()) {
        _setmode(_fileno(stdout), _O_U8TEXT);
        _setmode(_fileno(stderr), _O_U8TEXT);
    }
#endif

    for (auto& pid : console::get_list()) {
        write_console(query_name_from_pid(pid) + "\n");
    }

#if 1
    write_console(get_command_line() + L"\n");
    
    static const CHAR sn[]="﨨狝";
    std::string utf8(sn);
    std::wstring wutf8(utf8.begin(), utf8.end());
    auto ucs16 = cp_decode(CP_UTF8, utf8);
    std::vector<byte> ucs16bytes((byte*)&ucs16[0], (byte*)&ucs16[0] + (ucs16.size() * sizeof(ucs16[0])));
    std::string cmda(GetCommandLineA());
    std::wstring cmdw(GetCommandLineW());
    std::vector<byte> cmdwbytes((byte*)&cmdw[0], (byte*)&cmdw[0] + (cmdw.size() * sizeof(cmdw[0])));

    if (std::search(cmda.begin(), cmda.end(), utf8.begin(), utf8.end()) == cmda.end()) {
        write_console("- utf8 not found in cmda\n");
    } else {
        write_console("+ utf8 found in cmda\n");
    }

    if (std::search(cmda.begin(), cmda.end(), ucs16bytes.begin(), ucs16bytes.end()) == cmda.end()) {
        write_console("- ucs16 not found in cmda\n");
    } else {
        write_console("+ ucs16 found in cmda\n");
    }

    if (std::search(cmdw.begin(), cmdw.end(), wutf8.begin(), wutf8.end()) == cmdw.end()) {
        write_console("- utf8 not found in cmdw\n");
    } else {
        write_console("+ utf8 found in cmdw\n");
    }

    if (std::search(cmdw.begin(), cmdw.end(), ucs16.begin(), ucs16.end()) == cmdw.end()) {
        write_console("- ucs16 not found in cmdw\n");
    } else {
        write_console("+ ucs16 found in cmdw\n");
    }
#else
    static const CHAR sn[]="﨨狝 﨨狝 tränenüberströmt™";
    write_console(sn);
    printf(" <CHAR LITERAL> -> %hs\n", sn);

    static const WCHAR sl[]=L"﨨狝 﨨狝 tränenüberströmt™";
    write_console(sl);
    wprintf(L" <WCHAR LITERAL> -> %ls\n", sl);

    static const _TCHAR s[]=_T("﨨狝 﨨狝 tränenüberströmt™");
    write_console(s);
    _tprintf(_T(" <TCHAR LITERAL> -> %s\n"), s);

    write_console(GetCommandLineA());
    printf(" <CHAR CMDLN> -> %hs\n", GetCommandLine());

    write_console(GetCommandLineW());
    wprintf(L" <WCHAR CMDLN> -> %ls\n", GetCommandLineW());

    write_console(GetCommandLine());
    _tprintf(_T(" <TCHAR CMDLN> -> %s\n"), GetCommandLine());

    for (int i = 0; i < argc; ++i) {
        write_console(argv[i]);
        _tprintf(_T(" <TCHAR ARG> -> %s\n"), argv[i]);
    }
#endif

#if 0
    SetConsoleOutputCP(oldoutcp);
#endif
}
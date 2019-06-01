// MsgGetter.cpp --- Win32 message tracer
// Copyright (C) 2019 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// This file is public domain software.
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NON_CONFORMING_WCSTOK
#include "targetver.h"
#include <windows.h>
#include <windowsx.h>
#include <tchar.h>
#include <shlwapi.h>
#include <strsafe.h>
#include <string>
#include <cstdio>
#include <cassert>
#include "common.h"
#include "MResizable.hpp"
#include "ConstantsDB.hpp"
#include "resource.h"

#ifdef _MSC_VER
#pragma comment(linker,"/manifestdependency:\"type='win32' \
    name='Microsoft.Windows.Common-Controls' \
    version='6.0.0.0' \
    processorArchitecture='*' \
    publicKeyToken='6595b64144ccf1df' \
    language='*'\"") 
#endif

// check the bits of processor and process
inline bool CheckBits(HANDLE hProcess)
{
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    printf("info.wProcessorArchitecture: %08X\n", info.wProcessorArchitecture);

    #ifdef _WIN64
        return (info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64);
    #else
        return (info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL);
    #endif
}

typedef BOOL (APIENTRY *INSTALL_PROC)(HWND hwndNotify, HWND hwndTarget);
typedef BOOL (APIENTRY *UNINSTALL_PROC)(void);

HINSTANCE g_hInstance = NULL;
HINSTANCE g_hinstDLL = NULL;
HWND g_hwndNotify = NULL;
HWND g_hwndTarget = NULL;
HWND g_hLst1 = NULL;
MResizable g_resizable;
BOOL g_db_loaded = FALSE;
HICON g_hIcon = NULL;
HICON g_hIconSmall = NULL;

INSTALL_PROC InstallSendProc = NULL;
INSTALL_PROC InstallSendRetProc = NULL;
INSTALL_PROC InstallSendPostProc = NULL;
UNINSTALL_PROC UninstallSendProc = NULL;
UNINSTALL_PROC UninstallSendRetProc = NULL;
UNINSTALL_PROC UninstallPostProc = NULL;

#ifdef UNICODE
    typedef std::wstring tstring;
    #define CF_GENERICTEXT CF_UNICODETEXT
#else
    typedef std::string tstring;
    #define CF_GENERICTEXT CF_TEXT
#endif

BOOL EnableProcessPriviledge(LPCTSTR pszSE_)
{
    BOOL f;
    HANDLE hProcess;
    HANDLE hToken;
    LUID luid;
    TOKEN_PRIVILEGES tp;
    
    f = FALSE;
    hProcess = GetCurrentProcess();
    if (OpenProcessToken(hProcess, TOKEN_ADJUST_PRIVILEGES, &hToken))
    {
        if (LookupPrivilegeValue(NULL, pszSE_, &luid))
        {
            tp.PrivilegeCount = 1;
            tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
            tp.Privileges[0].Luid = luid;
            f = AdjustTokenPrivileges(hToken, FALSE, &tp, 0, NULL, NULL);
        }
        CloseHandle(hToken);
    }
    return f;
}

BOOL DoUnhook(HWND hwnd)
{
    if (g_hinstDLL)
    {
        UninstallSendProc();
        UninstallSendRetProc();
        UninstallPostProc();
        FreeLibrary(g_hinstDLL);
        g_hinstDLL = NULL;
    }
    printf("Unhooked\n");
    return TRUE;
}

BOOL DoHook(HWND hwnd)
{
    TCHAR szPath[MAX_PATH];
    GetModuleFileName(NULL, szPath, ARRAYSIZE(szPath));
    LPWSTR pch = PathFindFileName(szPath);
    if (pch && *pch)
        *pch = 0;

#ifdef _WIN64
    PathAppend(szPath, TEXT("MsgGet64.dll"));
#else
    PathAppend(szPath, TEXT("MsgGet32.dll"));
#endif
    g_hinstDLL = LoadLibrary(szPath);
    if (!g_hinstDLL)
    {
        return FALSE;
    }

    InstallSendProc = (INSTALL_PROC)GetProcAddress(g_hinstDLL, "InstallSendProc");
    InstallSendRetProc = (INSTALL_PROC)GetProcAddress(g_hinstDLL, "InstallSendRetProc");
    InstallSendPostProc = (INSTALL_PROC)GetProcAddress(g_hinstDLL, "InstallPostProc");
    UninstallSendProc = (UNINSTALL_PROC)GetProcAddress(g_hinstDLL, "UninstallSendProc");
    UninstallSendRetProc = (UNINSTALL_PROC)GetProcAddress(g_hinstDLL, "UninstallSendRetProc");
    UninstallPostProc = (UNINSTALL_PROC)GetProcAddress(g_hinstDLL, "UninstallPostProc");

    if (!InstallSendProc ||
        !InstallSendRetProc ||
        !InstallSendPostProc ||
        !UninstallSendProc ||
        !UninstallSendRetProc ||
        !UninstallPostProc)
    {
        FreeLibrary(g_hinstDLL);
        g_hinstDLL = NULL;
        return FALSE;
    }

    if (!InstallSendProc(hwnd, g_hwndTarget) ||
        !InstallSendRetProc(hwnd, g_hwndTarget) ||
        !InstallSendPostProc(hwnd, g_hwndTarget))
    {
        UninstallSendProc();
        UninstallSendRetProc();
        UninstallPostProc();
        FreeLibrary(g_hinstDLL);
        g_hinstDLL = NULL;
        return FALSE;
    }

    printf("Hooked\n");
    return TRUE;
}

BOOL DoData(HWND hwnd, LPTSTR psz)
{
    INT iItem, ich;

    ich = lstrlen(psz);
    if (ich && psz[ich - 1] == '\n')
    {
        psz[ich - 1] = 0;
        if ((ich - 1) && psz[ich - 2] == '\r')
        {
            psz[ich - 2] = 0;
        }
    }

    HDC hDC = GetDC(g_hLst1);
    HFONT hFont = GetWindowFont(g_hLst1);
    INT nExtent = ListBox_GetHorizontalExtent(g_hLst1);
    SIZE siz;
    HGDIOBJ hFontOld = SelectObject(hDC, hFont);
    GetTextExtentPoint32(hDC, psz, lstrlen(psz), &siz);
    SelectObject(hDC, hFontOld);
    if (siz.cx + 32 > nExtent)
        nExtent = siz.cx + 32;
    ListBox_SetHorizontalExtent(g_hLst1, nExtent);
    ReleaseDC(g_hLst1, hDC);

    SendMessage(g_hLst1, LB_ADDSTRING, 0, (LPARAM)psz);

    iItem = (INT)SendMessage(g_hLst1, LB_GETCOUNT, 0, 0);
    if (iItem != LB_ERR)
    {
        SendMessage(g_hLst1, LB_SETCURSEL, iItem - 1, 0);
    }

    return TRUE;
}

BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    g_hwndNotify = hwnd;
    g_hLst1 = GetDlgItem(hwnd, lst1);

    g_hIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(1));
    g_hIconSmall = (HICON)LoadImage(g_hInstance, MAKEINTRESOURCE(1),
        IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON),
        0);

    SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)g_hIcon);
    SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)g_hIconSmall);

    WCHAR *pch, szPath[MAX_PATH];
    GetModuleFileName(NULL, szPath, ARRAYSIZE(szPath));
    pch = PathFindFileName(szPath);
    if (pch && *pch)
        *pch = 0;
    PathAppend(szPath, L"Constants.txt");

    g_db_loaded = TRUE;
    if (!g_db.LoadFromFile(szPath))
    {
        if (pch && *pch)
            *pch = 0;
        PathAppend(szPath, L"..\\Constants.txt");
        if (!g_db.LoadFromFile(szPath))
        {
            if (pch && *pch)
                *pch = 0;
            PathAppend(szPath, L"..\\..\\Constants.txt");
            if (!g_db.LoadFromFile(szPath))
            {
                printf("Unable to load Constants.txt file\n");
                g_db_loaded = FALSE;
            }
        }
    }

    INT argc;
    LPWSTR *wargv = CommandLineToArgvW(GetCommandLineW(), &argc);
    HWND hwndTarget = NULL;

    if (argc <= 1)
    {
        MessageBox(NULL, TEXT("Please specify window handle (HWND)."), NULL, MB_ICONERROR);
        EndDialog(hwnd, IDABORT);
        return FALSE;
    }

    for (INT i = 1; i < argc; ++i)
    {
        hwndTarget = (HWND)(ULONG_PTR)wcstoul(wargv[i], &pch, 16);
        if (!IsWindow(hwndTarget) || *pch != 0)
        {
            hwndTarget = (HWND)(ULONG_PTR)wcstoul(wargv[i], &pch, 0);
        }
    }
    if (!IsWindow(hwndTarget))
    {
        MessageBox(NULL, TEXT("HWND was not valid."), NULL, MB_ICONERROR);
        EndDialog(hwnd, IDABORT);
        return FALSE;
    }

    DWORD pid;
    GetWindowThreadProcessId(hwndTarget, &pid);
    HANDLE hProcess = OpenProcess(PROCESS_VM_WRITE | PROCESS_VM_OPERATION, FALSE, pid);
    if (!hProcess)
    {
        MessageBox(NULL, TEXT("Unable to open process."), NULL, MB_ICONERROR);
        EndDialog(hwnd, IDABORT);
        return FALSE;
    }

    if (!CheckBits(hProcess))
    {
        MessageBox(hwnd, TEXT("CheckBits failed"), NULL, MB_ICONERROR);
        CloseHandle(hProcess);
        return FALSE;
    }

    CloseHandle(hProcess);

    g_hwndTarget = hwndTarget;

    if (!DoHook(hwnd))
    {
        MessageBox(NULL, TEXT("Unable to hook."), NULL, MB_ICONERROR);
        EndDialog(hwnd, IDABORT);
        return TRUE;
    }

    TCHAR szText[256], szText2[64];
    LoadString(NULL, IDS_MSGFOR, szText2, ARRAYSIZE(szText2));
    StringCbPrintf(szText, sizeof(szText), szText2, hwndTarget);
    SetWindowText(hwnd, szText);

    DWORD osver = GetVersion();
    GetWindowsDirectory(szText2, ARRAYSIZE(szText2));
    StringCbPrintf(szText, sizeof(szText), TEXT("GetVersion():0x%08lX, WinDir:'%s'"), osver, szText2);
    DoData(hwnd, szText);

    GetClassName(hwndTarget, szText2, ARRAYSIZE(szText2));
    StringCbPrintf(szText, sizeof(szText), TEXT("hwndTarget:%p, ClassName:'%s'"), hwndTarget, szText2);
    DoData(hwnd, szText);

    GetWindowText(hwndTarget, szText2, ARRAYSIZE(szText2));
    StringCbPrintf(szText, sizeof(szText), TEXT("WindowText:'%s'"), szText2);
    DoData(hwnd, szText);

    RECT rcWnd;
    GetWindowRect(hwndTarget, &rcWnd);
    StringCbPrintf(szText, sizeof(szText), TEXT("WindowRect: (%ld, %ld) - (%ld, %ld)\n"),
                   rcWnd.left, rcWnd.top, rcWnd.right, rcWnd.bottom);
    DoData(hwnd, szText);

    RECT rcClient;
    GetClientRect(hwndTarget, &rcClient);
    StringCbPrintf(szText, sizeof(szText), TEXT("ClientRect: (%ld, %ld) - (%ld, %ld)\n"),
                   rcClient.left, rcClient.top, rcClient.right, rcClient.bottom);
    DoData(hwnd, szText);

    DWORD style = GetWindowStyle(hwndTarget);
    DWORD exstyle = GetWindowExStyle(hwndTarget);
    if (!g_db_loaded)
    {
        StringCbPrintf(szText, sizeof(szText), TEXT("style:0x%08lX, exstyle:0x%08lX"),
                       style, exstyle);
        DoData(hwnd, szText);
    }
    else
    {
        GetClassName(hwndTarget, szText2, ARRAYSIZE(szText2));
        std::wstring strStyle = mstr_hex(style);
        std::wstring bits = g_db.DumpBitField(szText2, L"STYLE", style);
        strStyle += L" (";
        strStyle += bits;
        strStyle += L")";
        StringCbPrintf(szText, sizeof(szText), TEXT("style:%s"), strStyle.c_str());
        DoData(hwnd, szText);

        std::wstring strExStyle = mstr_hex(exstyle);
        strExStyle += L" (";
        strExStyle += g_db.DumpBitField(L"EXSTYLE", exstyle);
        strExStyle += L")";
        StringCbPrintf(szText, sizeof(szText), TEXT("exstyle:%s"), strExStyle.c_str());
        DoData(hwnd, szText);
    }

    StringCbPrintf(szText, sizeof(szText), TEXT("Owner:%p, Parent:%p"),
                   GetWindow(hwndTarget, GW_OWNER), GetParent(hwndTarget));
    DoData(hwnd, szText);

    StringCbPrintf(szText, sizeof(szText), TEXT("FirstChild:%p, LastChild:%p"),
                   GetTopWindow(hwndTarget),
                   GetWindow(GetTopWindow(hwndTarget), GW_HWNDLAST));
    DoData(hwnd, szText);

    StringCbPrintf(szText, sizeof(szText), TEXT("---"));
    DoData(hwnd, szText);

    g_resizable.OnParentCreate(hwnd);
    g_resizable.SetLayoutAnchor(lst1, mzcLA_TOP_LEFT, mzcLA_BOTTOM_RIGHT);
    g_resizable.SetLayoutAnchor(psh1, mzcLA_BOTTOM_LEFT);
    g_resizable.SetLayoutAnchor(psh2, mzcLA_BOTTOM_LEFT);
    g_resizable.SetLayoutAnchor(psh3, mzcLA_BOTTOM_LEFT);
    g_resizable.SetLayoutAnchor(psh4, mzcLA_BOTTOM_LEFT);
    g_resizable.SetLayoutAnchor(psh5, mzcLA_BOTTOM_LEFT);

    return TRUE;
}

BOOL DoGetText(HWND hwnd, tstring& str)
{
    INT i, nCount;
    static TCHAR szText[1024];

    nCount = (INT)SendMessage(g_hLst1, LB_GETCOUNT, 0, 0);
    for (i = 0; i < nCount; ++i)
    {
        ListBox_GetText(g_hLst1, i, szText);
        str += szText;
        str += TEXT("\r\n");
    }

    return nCount != 0;
}

void OnCopy(HWND hwnd)
{
    static TCHAR szText[1024];
    INT i, nCount;
    std::vector<INT> items;
    tstring str;

    nCount = ListBox_GetSelCount(g_hLst1);
    if (nCount <= 0)
        return;

    items.resize(nCount);
    ListBox_GetSelItems(g_hLst1, nCount, &items[0]);

    for (i = 0; i < nCount; ++i)
    {
        ListBox_GetText(g_hLst1, items[i], szText);
        str += szText;
        str += TEXT("\r\n");
    }

    if (OpenClipboard(hwnd))
    {
        EmptyClipboard();

        SIZE_T size = (str.size() + 1) * sizeof(TCHAR);
        if (HGLOBAL hGlobal = GlobalAlloc(GHND | GMEM_SHARE, size))
        {
            if (LPTSTR psz = (LPTSTR)GlobalLock(hGlobal))
            {
                CopyMemory(psz, str.c_str(), size);
                GlobalUnlock(hGlobal);
            }
            SetClipboardData(CF_GENERICTEXT, hGlobal);
        }

        CloseClipboard();
    }
}

void OnCopyAll(HWND hwnd)
{
    tstring str;
    DoGetText(hwnd, str);

    if (OpenClipboard(hwnd))
    {
        EmptyClipboard();

        SIZE_T size = (str.size() + 1) * sizeof(TCHAR);
        if (HGLOBAL hGlobal = GlobalAlloc(GHND | GMEM_SHARE, size))
        {
            if (LPTSTR psz = (LPTSTR)GlobalLock(hGlobal))
            {
                CopyMemory(psz, str.c_str(), size);
                GlobalUnlock(hGlobal);
            }
            SetClipboardData(CF_GENERICTEXT, hGlobal);
        }

        CloseClipboard();
    }
}

void OnStop(HWND hwnd)
{
    DoUnhook(hwnd);
}

void OnRestart(HWND hwnd)
{
    DoUnhook(hwnd);
    DoHook(hwnd);
}

void OnSaveAs(HWND hwnd)
{
    OPENFILENAME ofn;
    TCHAR szFile[MAX_PATH] = TEXT("Messages.txt");
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = TEXT("Text Files (*.txt;*.log)\0*.txt;*.log\0All Files (*.*)\0*.*\0");
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = ARRAYSIZE(szFile);
    ofn.lpstrTitle = TEXT("Save As");
    ofn.Flags = OFN_EXPLORER | OFN_ENABLESIZING | OFN_HIDEREADONLY |
                OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    ofn.lpstrDefExt = TEXT("txt");
    if (GetSaveFileName(&ofn))
    {
        tstring str;
        DoGetText(hwnd, str);

        if (FILE *fp = _tfopen(szFile, TEXT("wb")))
        {
            SIZE_T size = str.size() * sizeof(TCHAR);
            fwrite(str.c_str(), size, 1, fp);
            fclose(fp);
        }
    }
}

void OnClear(HWND hwnd)
{
    ListBox_ResetContent(g_hLst1);
    ListBox_SetHorizontalExtent(g_hLst1, 0);
}

void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    switch (id)
    {
    case IDOK:
    case IDCANCEL:
        UninstallSendProc();
        UninstallSendRetProc();
        UninstallPostProc();
        DestroyIcon(g_hIcon);
        DestroyIcon(g_hIconSmall);
        FreeLibrary(g_hinstDLL);
        EndDialog(hwnd, id);
        break;
    case psh1:
    case ID_COPY_ALL:
        OnCopyAll(hwnd);
        break;
    case psh2:
        OnStop(hwnd);
        break;
    case psh3:
        OnRestart(hwnd);
        break;
    case psh4:
    case ID_SAVE_AS:
        OnSaveAs(hwnd);
        break;
    case psh5:
    case ID_CLEAR:
        OnClear(hwnd);
        break;
    case ID_COPY:
        OnCopy(hwnd);
        break;
    }
}

BOOL OnCopyData(HWND hwnd, HWND hwndFrom, PCOPYDATASTRUCT pcds)
{
    LPTSTR psz = (LPTSTR)pcds->lpData;

    return DoData(hwnd, psz);
}

void OnSize(HWND hwnd, UINT state, int cx, int cy)
{
    g_resizable.OnSize();
}

void OnContextMenu(HWND hwnd, HWND hwndContext, UINT xPos, UINT yPos)
{
    if (hwndContext != g_hLst1)
        return;

    INT nCount = ListBox_GetSelCount(g_hLst1);
    if (nCount == 0)
    {
        POINT pt;
        pt.x = xPos;
        pt.y = yPos;
        ScreenToClient(g_hLst1, &pt);

        SendMessage(g_hLst1, WM_LBUTTONDOWN, 0, MAKELPARAM(pt.x, pt.y));
    }

    SetForegroundWindow(hwnd);

    HMENU hMenu = LoadMenu(g_hInstance, MAKEINTRESOURCE(1));
    HMENU hSubMenu = GetSubMenu(hMenu, 0);

    INT nCmd = TrackPopupMenu(hSubMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD,
                              xPos, yPos, 0, hwnd, NULL);
    DestroyMenu(hMenu);

    PostMessage(hwnd, WM_COMMAND, nCmd, 0);
}

INT_PTR CALLBACK
DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
        HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
        HANDLE_MSG(hwnd, WM_COPYDATA, OnCopyData);
        HANDLE_MSG(hwnd, WM_SIZE, OnSize);
        HANDLE_MSG(hwnd, WM_CONTEXTMENU, OnContextMenu);
    }
    return 0;
}

INT WINAPI
WinMain(HINSTANCE   hInstance,
        HINSTANCE   hPrevInstance,
        LPSTR       lpCmdLine,
        INT         nCmdShow)
{
    g_hInstance = hInstance;
    EnableProcessPriviledge(SE_DEBUG_NAME);
    DialogBox(hInstance, MAKEINTRESOURCE(1), NULL, DialogProc);
    return 0;
}

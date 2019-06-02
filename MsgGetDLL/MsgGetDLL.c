// MsgGetDLL.c --- Win32 message tracing DLL
// Copyright (C) 2019 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// This file is public domain software.

#include "targetver.h"
#include <windows.h>
#include <strsafe.h>
#include <assert.h>
#include "common.h"

#undef SHARED
#if defined(__GNUC__) || defined(__clang__)
    #define SHARED __attribute__((section(".shared"), shared))
#else
    #define SHARED
#endif

#pragma data_seg(".shared")
HINSTANCE g_hinstDLL SHARED = NULL;
HWND g_hwndNotify SHARED = NULL;
HWND g_hwndTarget SHARED = NULL;
HHOOK g_hPostMsgHook SHARED = NULL;
HHOOK g_hSendMsgHook SHARED = NULL;
HHOOK g_hSendRetMsgHook SHARED = NULL;
LPCTSTR g_pszPrefix = TEXT("");
DWORD g_dwData;
#pragma data_seg()
#pragma comment(linker, "/section:.shared,RWS!D!K!P")

void MsgDumpPrintf(LPCTSTR fmt, ...)
{
    static TCHAR s_szText[1024];
    va_list va;
    COPYDATASTRUCT CopyData;
    DWORD_PTR dwResult;

    va_start(va, fmt);
    StringCbVPrintf(s_szText, sizeof(s_szText), fmt, va);

    CopyData.dwData = g_dwData;
    CopyData.cbData = sizeof(TCHAR) * (lstrlen(s_szText) + 1);
    CopyData.lpData = s_szText;
    SendMessageTimeout(g_hwndNotify, WM_COPYDATA, 0, (LPARAM)&CopyData,
                       SMTO_NORMAL | SMTO_ABORTIFHUNG,
                       COPYDATA_TIMEOUT, &dwResult);
    va_end(va);
}

#define MSGDUMP_TPRINTF MsgDumpPrintf
#define MSGDUMP_PREFIX g_pszPrefix
#include "msgdump.h"

BOOL APIENTRY InstallSendProc(HWND hwndNotify, HWND hwndTarget);
BOOL APIENTRY InstallSendRetProc(HWND hwndNotify, HWND hwndTarget);
BOOL APIENTRY InstallPostProc(HWND hwndNotify, HWND hwndTarget);
BOOL APIENTRY UninstallSendProc(void);
BOOL APIENTRY UninstallSendRetProc(void);
BOOL APIENTRY UninstallPostProc(void);

// for WH_CALLWNDPROC
static LRESULT CALLBACK
MsgSendProc(int code, WPARAM wParam, LPARAM lParam)
{
    PCWPSTRUCT pMsg;
    HWND hwnd;
    DWORD tid1, tid2;

    if (code == HC_ACTION)
    {
        if (!IsWindow(g_hwndNotify) || !IsWindow(g_hwndTarget))
        {
            assert(0);
            UninstallSendProc();
        }
        else
        {
            pMsg = (PCWPSTRUCT)lParam;
            hwnd = pMsg->hwnd;
            tid1 = GetWindowThreadProcessId(hwnd, NULL);
            tid2 = GetWindowThreadProcessId(g_hwndNotify, NULL);
            if (hwnd == g_hwndTarget && tid1 != tid2)
            {
                g_dwData = COPYDATA_SEND;
                g_pszPrefix = TEXT("S: ");
                MD_msgdump(pMsg->hwnd, pMsg->message, pMsg->wParam, pMsg->lParam);
            }
        }
    }

    return CallNextHookEx(g_hSendMsgHook, code, wParam, lParam);
}

// for WH_CALLWNDPROCRET
static LRESULT CALLBACK
MsgSendRetProc(INT code, WPARAM wParam, LPARAM lParam)
{
    PCWPRETSTRUCT pMsg;
    DWORD tid1, tid2;

    if (code == HC_ACTION)
    {
        if (!IsWindow(g_hwndNotify) || !IsWindow(g_hwndTarget))
        {
            assert(0);
            UninstallSendRetProc();
        }
        else
        {
            HWND hwnd;
            pMsg = (PCWPRETSTRUCT)lParam;
            hwnd = pMsg->hwnd;
            tid1 = GetWindowThreadProcessId(hwnd, NULL);
            tid2 = GetWindowThreadProcessId(g_hwndNotify, NULL);
            if (hwnd == g_hwndTarget && tid1 != tid2)
            {
                g_dwData = COPYDATA_SENDRET;
                g_pszPrefix = TEXT("R: ");
                MD_msgresult(pMsg->hwnd, pMsg->message, pMsg->wParam, pMsg->lParam, pMsg->lResult);
                if (pMsg->message == WM_NCDESTROY)
                {
                    UninstallSendRetProc();
                }
            }
        }
    }

    return CallNextHookEx(g_hSendRetMsgHook, code, wParam, lParam);
}

// for WH_GETMESSAGE
static LRESULT CALLBACK
MsgPostProc(INT nCode, WPARAM wParam, LPARAM lParam)
{
    LPMSG pMsg;
    HWND hwnd;
    DWORD tid1, tid2;

    if (nCode == HC_ACTION)
    {
        if (!IsWindow(g_hwndNotify) || !IsWindow(g_hwndTarget))
        {
            assert(0);
            UninstallPostProc();
        }
        else
        {
            pMsg = (LPMSG)lParam;
            hwnd = pMsg->hwnd;
            tid1 = GetWindowThreadProcessId(hwnd, NULL);
            tid2 = GetWindowThreadProcessId(g_hwndNotify, NULL);
            if (hwnd == g_hwndTarget && tid1 != tid2)
            {
                g_pszPrefix = TEXT("P: ");
                g_dwData = COPYDATA_POST;
                MD_msgdump(pMsg->hwnd, pMsg->message, pMsg->wParam, pMsg->lParam);
            }
        }
    }

    return CallNextHookEx(g_hPostMsgHook, nCode, wParam, lParam);
}

BOOL APIENTRY UninstallSendProc(void)
{
    if (g_hSendMsgHook)
    {
        UnhookWindowsHookEx(g_hSendMsgHook);
        g_hSendMsgHook = NULL;
    }
    return TRUE;
}

BOOL APIENTRY InstallSendProc(HWND hwndNotify, HWND hwndTarget)
{
    DWORD tid;
    UninstallSendProc();

    g_hwndNotify = hwndNotify;
    g_hwndTarget = hwndTarget;

    tid = GetWindowThreadProcessId(g_hwndTarget, NULL);
    g_hSendMsgHook = SetWindowsHookEx(WH_CALLWNDPROC, MsgSendProc, g_hinstDLL, tid);
    return g_hSendMsgHook != NULL;
}

BOOL APIENTRY UninstallSendRetProc(void)
{
    if (g_hSendRetMsgHook)
    {
        UnhookWindowsHookEx(g_hSendRetMsgHook);
        g_hSendRetMsgHook = NULL;
    }
    return TRUE;
}

BOOL APIENTRY InstallSendRetProc(HWND hwndNotify, HWND hwndTarget)
{
    DWORD tid;
    UninstallSendRetProc();

    g_hwndNotify = hwndNotify;
    g_hwndTarget = hwndTarget;

    tid = GetWindowThreadProcessId(g_hwndTarget, NULL);
    g_hSendRetMsgHook = SetWindowsHookEx(WH_CALLWNDPROCRET, MsgSendRetProc, g_hinstDLL, tid);
    return g_hSendRetMsgHook != NULL;
}

BOOL APIENTRY UninstallPostProc(void)
{
    if (g_hPostMsgHook)
    {
        UnhookWindowsHookEx(g_hPostMsgHook);
        g_hPostMsgHook = NULL;
    }
    return TRUE;
}

BOOL APIENTRY InstallPostProc(HWND hwndNotify, HWND hwndTarget)
{
    DWORD tid;
    UninstallPostProc();

    g_hwndNotify = hwndNotify;
    g_hwndTarget = hwndTarget;

    tid = GetWindowThreadProcessId(g_hwndTarget, NULL);
    g_hPostMsgHook = SetWindowsHookEx(WH_GETMESSAGE, MsgPostProc, g_hinstDLL, tid);
    return g_hPostMsgHook != NULL;
}

BOOL WINAPI
DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        g_hinstDLL = hinstDLL;
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

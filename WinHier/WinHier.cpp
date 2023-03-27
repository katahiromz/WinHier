// WinHier.cpp
// Copyright (C) 2018-2019 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>.
// This file is public domain software.

#include "targetver.h"
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NON_CONFORMING_WCSTOK
#define MWINDOWTREE_ADJUST_OWNER
#include <shlwapi.h>
#include "MWindowTreeView.hpp"
#include "MResizable.hpp"
#include "ConstantsDB.hpp"
#include "MSelectWnd.hpp"
#include "resource.h"

#ifdef _MSC_VER
#pragma comment(linker,"/manifestdependency:\"type='win32' \
    name='Microsoft.Windows.Common-Controls' \
    version='6.0.0.0' \
    processorArchitecture='*' \
    publicKeyToken='6595b64144ccf1df' \
    language='*'\"") 
#endif

BOOL NormalizeNTPath(WCHAR *pszDest, size_t cchDestMax, LPCWSTR pszSrc)
{
    StringCbCopyW(pszDest, cchDestMax, pszSrc);

    WCHAR szNTPath[MAX_PATH], szDrive[MAX_PATH] = L"A:";
    for (WCHAR cDrive = L'A'; cDrive <= L'Z'; ++cDrive)
    {
        szDrive[0] = cDrive;
        szNTPath[0] = 0;
        if (QueryDosDeviceW(szDrive, szNTPath, ARRAYSIZE(szNTPath)))
        {
            size_t cchLen = wcslen(szNTPath);
            if (memcmp(szNTPath, pszSrc, cchLen * sizeof(WCHAR)) == 0)
            {
                StringCchCopyW(pszDest, cchDestMax, szDrive);
                StringCchCatW(pszDest, cchDestMax, &pszSrc[cchLen]);
                return TRUE;
            }
        }
    }
    return FALSE;
}

// QueryFullProcessImageNameW
typedef BOOL (WINAPI *QUERYFULLPROCESSIMAGENAMEW)(HANDLE, DWORD, LPWSTR, PDWORD);
QUERYFULLPROCESSIMAGENAMEW g_pQueryFullProcessImageNameW =
    (QUERYFULLPROCESSIMAGENAMEW)GetProcAddress(GetModuleHandleA("kernel32"), "QueryFullProcessImageNameW");

inline BOOL GetFileNameFromProcess(LPWSTR pszPath, DWORD cchPath, HANDLE hProcess)
{
    if (g_pQueryFullProcessImageNameW)
    {
        DWORD cch = cchPath;
        return (*g_pQueryFullProcessImageNameW)(hProcess, 0, pszPath, &cch);
    }
    else
    {
        TCHAR szPath[MAX_PATH];
        BOOL ret = GetProcessImageFileName(hProcess, szPath, ARRAYSIZE(szPath));
        NormalizeNTPath(pszPath, cchPath, szPath);
        return ret;
    }
}

inline BOOL GetFileNameFromPID(LPWSTR pszPath, DWORD cchPath, DWORD pid)
{
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (!hProcess)
        return FALSE;

    BOOL ret = GetFileNameFromProcess(pszPath, cchPath, hProcess);
    CloseHandle(hProcess);
    return ret;
}

typedef BOOL (WINAPI *ISWOW64PROCESS)(HANDLE, PBOOL);

BOOL IsWow64(HANDLE hProcess)
{
    HINSTANCE hKernel32 = GetModuleHandleA("kernel32");
    ISWOW64PROCESS pIsWow64Process;
    pIsWow64Process = (ISWOW64PROCESS)GetProcAddress(hKernel32, "IsWow64Process");
    if (pIsWow64Process)
    {
        BOOL ret = FALSE;
        (*pIsWow64Process)(hProcess, &ret);
        return ret;
    }
    return FALSE;
}

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

BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    MWindowTreeView::node_type *node = (MWindowTreeView::node_type *)lParam;
    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)node);

    TCHAR szText[128];

    if (node->m_type == MWindowTreeNode::WINDOW)
    {
        StringCbPrintf(szText, sizeof(szText), TEXT("%p"), node->m_hwndTarget);
        SetDlgItemText(hwnd, edt1, szText);

        StringCbPrintf(szText, sizeof(szText), TEXT("%s"), node->m_szClass);
        SetDlgItemText(hwnd, edt2, szText);

        DWORD cls_style = node->m_cls_style;
        std::wstring strClsStyle = mstr_hex(node->m_cls_style);
        strClsStyle += L" (";
        strClsStyle += g_db.DumpBitField(L"CLASS.STYLE", cls_style);
        strClsStyle += L")";
        SetDlgItemText(hwnd, edt3, strClsStyle.c_str());

        StringCbPrintf(szText, sizeof(szText), TEXT("%s"), node->m_szText);
        SetDlgItemText(hwnd, edt4, szText);

        DWORD style = node->m_style;
        std::wstring strStyle = mstr_hex(node->m_style);
        strStyle += L" (";
        strStyle += g_db.DumpBitField(node->m_szClass, L"STYLE", style);
        strStyle += L")";
        SetDlgItemText(hwnd, edt5, strStyle.c_str());

        DWORD exstyle = node->m_exstyle;
        std::wstring strExStyle = mstr_hex(node->m_exstyle);
        strExStyle += L" (";
        strExStyle += g_db.DumpBitField(L"EXSTYLE", exstyle);
        strExStyle += L")";
        SetDlgItemText(hwnd, edt6, strExStyle.c_str());

        StringCbPrintf(szText, sizeof(szText), TEXT("%p"), node->m_hwndOwner);
        SetDlgItemText(hwnd, edt7, szText);

        StringCbPrintf(szText, sizeof(szText), TEXT("%p"), node->m_hwndParent);
        SetDlgItemText(hwnd, edt8, szText);

        StringCbPrintf(szText, sizeof(szText), TEXT("%p"), node->m_hwndFirstChild);
        SetDlgItemText(hwnd, edt9, szText);

        StringCbPrintf(szText, sizeof(szText), TEXT("%p"), node->m_hwndLastChild);
        SetDlgItemText(hwnd, edt10, szText);

        StringCbPrintf(szText, sizeof(szText), TEXT("0x%04X (%u)"), node->m_wClassAtom, node->m_wClassAtom);
        SetDlgItemText(hwnd, edt11, szText);

        StringCbPrintf(szText, sizeof(szText), TEXT("%p"), node->m_id);
        SetDlgItemText(hwnd, edt12, szText);

        StringCbPrintf(szText, sizeof(szText), TEXT("(%ld, %ld) - (%ld, %ld)"),
                       node->m_rcWnd.left,
                       node->m_rcWnd.top,
                       node->m_rcWnd.right,
                       node->m_rcWnd.bottom);
        SetDlgItemText(hwnd, edt13, szText);

        StringCbPrintf(szText, sizeof(szText), TEXT("(%ld, %ld) - (%ld, %ld)"),
                       node->m_rcClient.left,
                       node->m_rcClient.top,
                       node->m_rcClient.right,
                       node->m_rcClient.bottom);
        SetDlgItemText(hwnd, edt14, szText);

        SetDlgItemText(hwnd, edt15, node->m_szExeFile);
    }
    else if (node->m_type == MWindowTreeNode::PROCESS)
    {
        StringCbPrintf(szText, sizeof(szText), TEXT("0x%08X (%lu)"), (DWORD)node->m_id, (DWORD)node->m_id);
        SetDlgItemText(hwnd, edt12, szText);

        SetDlgItemText(hwnd, edt15, node->m_szExeFile);
    }
    else if (node->m_type == MWindowTreeNode::THREAD)
    {
        StringCbPrintf(szText, sizeof(szText), TEXT("0x%08X (%lu)"), (DWORD)node->m_id, (DWORD)node->m_id);
        SetDlgItemText(hwnd, edt12, szText);
    }

    return TRUE;
}

BOOL WriteLine(FILE *fp, LPCWSTR fmt, ...)
{
    WCHAR szTextW[MAX_PATH];
    CHAR szTextA[MAX_PATH];
    va_list va;
    va_start(va, fmt);
    StringCbVPrintfW(szTextW, sizeof(szTextW), fmt, va);
    WideCharToMultiByte(CP_UTF8, 0, szTextW, -1, szTextA, MAX_PATH, NULL, NULL);
    va_end(va);
    if (fputs(szTextA, fp) != 0)
        throw 1;
    return TRUE;
}

static void
OnSaveAs(HWND hwnd, LPCTSTR pszFile, MWindowTreeView::node_type *node)
{
    BOOL bOK = FALSE;
    if (FILE *fp = _wfopen(pszFile, L"wb"))
    {
        bOK = TRUE;
        try
        {
            fputs("\xEF\xBB\xBF", fp);  // BOM

            if (node->m_type == MWindowTreeNode::WINDOW)
            {
                WriteLine(fp, TEXT("HWND: %p\r\n"), node->m_hwndTarget);
                WriteLine(fp, TEXT("Class Name: '%s'\r\n"), node->m_szClass);

                DWORD cls_style = node->m_cls_style;
                std::wstring strClsStyle = mstr_hex(node->m_cls_style);
                strClsStyle += L" (";
                strClsStyle += g_db.DumpBitField(L"CLASS.STYLE", cls_style);
                strClsStyle += L")";
                WriteLine(fp, TEXT("Class Style: %s\r\n"), strClsStyle.c_str());

                WriteLine(fp, TEXT("Window Text: '%s'\r\n"), node->m_szText);

                DWORD style = node->m_style;
                std::wstring strStyle = mstr_hex(node->m_style);
                strStyle += L" (";
                strStyle += g_db.DumpBitField(node->m_szClass, L"STYLE", style);
                strStyle += L")";
                WriteLine(fp, L"Window Style: %s\r\n", strStyle.c_str());

                DWORD exstyle = node->m_exstyle;
                std::wstring strExStyle = mstr_hex(node->m_exstyle);
                strExStyle += L" (";
                strExStyle += g_db.DumpBitField(L"EXSTYLE", exstyle);
                strExStyle += L")";
                WriteLine(fp, L"Extended Style: %s\r\n", strExStyle.c_str());

                WriteLine(fp, TEXT("Owner: %p\r\n"), node->m_hwndOwner);

                WriteLine(fp, TEXT("Parent: %p\r\n"), node->m_hwndParent);

                WriteLine(fp, TEXT("First Child: %p\r\n"), node->m_hwndFirstChild);

                WriteLine(fp, TEXT("Last Child: %p\r\n"), node->m_hwndLastChild);

                WriteLine(fp, TEXT("Class Atom: 0x%04X (%u)\r\n"), node->m_wClassAtom, node->m_wClassAtom);

                WriteLine(fp, TEXT("ID: %p\r\n"), node->m_id);

                WriteLine(fp, TEXT("Window Rect: (%ld, %ld) - (%ld, %ld)\r\n"),
                               node->m_rcWnd.left,
                               node->m_rcWnd.top,
                               node->m_rcWnd.right,
                               node->m_rcWnd.bottom);

                WriteLine(fp, TEXT("Client Rect: (%ld, %ld) - (%ld, %ld)\r\n"),
                               node->m_rcClient.left,
                               node->m_rcClient.top,
                               node->m_rcClient.right,
                               node->m_rcClient.bottom);

                WriteLine(fp, L"EXE Filename: '%s'\r\n", node->m_szExeFile);
            }
            else if (node->m_type == MWindowTreeNode::PROCESS)
            {
                WriteLine(fp, TEXT("ID: 0x%08X (%lu)"), (DWORD)node->m_id, (DWORD)node->m_id);

                WriteLine(fp, L"EXE Filename: '%s'\r\n", node->m_szExeFile);
            }
            else if (node->m_type == MWindowTreeNode::THREAD)
            {
                WriteLine(fp, TEXT("ID: 0x%08X (%lu)\r\n"), (DWORD)node->m_id, (DWORD)node->m_id);
            }
        }
        catch (int)
        {
            bOK = FALSE;
        }

        fclose(fp);
    }

    if (!bOK)
    {
        DeleteFile(pszFile);
        MessageBoxW(hwnd, LoadStringDx(IDS_CANTSAVE), NULL, MB_ICONERROR);
    }
}

void OnPsh1(HWND hwnd)
{
    MWindowTreeView::node_type *node;
    node = (MWindowTreeView::node_type *)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    TCHAR szFile[MAX_PATH];
    StringCbCopy(szFile, sizeof(szFile), LoadStringDx(IDS_TXTNAME));

    OPENFILENAME ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = MakeFilterDx(LoadStringDx(IDS_FILTER));
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = ARRAYSIZE(szFile);
    ofn.lpstrTitle = LoadStringDx(IDS_SAVEAS);
    ofn.Flags = OFN_EXPLORER | OFN_ENABLESIZING | OFN_HIDEREADONLY |
                OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
    ofn.lpstrDefExt = TEXT("txt");
    if (GetSaveFileName(&ofn))
    {
        OnSaveAs(hwnd, szFile, node);
    }
}

void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    switch (id)
    {
    case IDOK:
    case IDCANCEL:
        EndDialog(hwnd, id);
        break;
    case psh1:
        OnPsh1(hwnd);
        break;
    }
}

INT_PTR CALLBACK
PropDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
        HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
    }
    return 0;
}

class WinHier : public MDialogBase
{
public:
    HINSTANCE   m_hInst;        // the instance handle
    HICON       m_hIcon;        // the main icon
    HICON       m_hIconSm;      // the small icon
    HWND                m_hwndSelected;
    MWindowTreeView     m_ctl1;
    MResizable          m_resizable;
    SELECTWNDDATA       m_selection;
    MSelectWndIconWnd   m_ico1;

    WinHier(HINSTANCE hInst)
        : MDialogBase(IDD_MAIN), m_hInst(hInst),
          m_hIcon(NULL), m_hIconSm(NULL), m_hwndSelected(NULL),
          m_ico1(m_selection)
    {
        m_hIcon = LoadIconDx(IDI_MAIN);
        m_hIconSm = LoadSmallIconDx(IDI_MAIN);
    }

    virtual ~WinHier()
    {
        DestroyIcon(m_hIcon);
        DestroyIcon(m_hIconSm);
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        EnableProcessPriviledge(SE_DEBUG_NAME);

        SubclassChildDx(m_ctl1, ctl1);

        WCHAR *pch, szPath[MAX_PATH];
        GetModuleFileName(NULL, szPath, ARRAYSIZE(szPath));
        pch = PathFindFileName(szPath);
        if (pch && *pch)
            *pch = 0;
        PathAppend(szPath, L"Constants.txt");

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
                    MessageBoxW(NULL, L"Unable to load Constants.txt file", NULL,
                                MB_ICONERROR);
                    DestroyWindow(hwnd);
                    return TRUE;
                }
            }
        }

        if (HINSTANCE hinstUXTheme = LoadLibrary(TEXT("UXTHEME")))
        {
            typedef HRESULT (WINAPI *SETWINDOWTHEME)(HWND, LPCWSTR, LPCWSTR);
            SETWINDOWTHEME pSetWindowTheme =
                (SETWINDOWTHEME)GetProcAddress(hinstUXTheme, "SetWindowTheme");

            if (pSetWindowTheme)
            {
                // apply Explorer's visual style
                (*pSetWindowTheme)(m_ctl1, L"Explorer", NULL);
            }

            FreeLibrary(hinstUXTheme);
        }

        SendDlgItemMessage(hwnd, cmb1, CB_ADDSTRING, 0, (LPARAM)TEXT("Desktop-Tree"));
        SendDlgItemMessage(hwnd, cmb1, CB_ADDSTRING, 0, (LPARAM)TEXT("Process-Window"));
        SendDlgItemMessage(hwnd, cmb1, CB_ADDSTRING, 0, (LPARAM)TEXT("Process-Thread"));
        SendDlgItemMessage(hwnd, cmb1, CB_SETCURSEL, 0, 0);

        SendMessageDx(WM_SETICON, ICON_BIG, LPARAM(m_hIcon));
        SendMessageDx(WM_SETICON, ICON_SMALL, LPARAM(m_hIconSm));

        SubclassChildDx(m_ico1, ico1);
        m_ico1.SetTargetIcon(m_ico1);

        // adjust size
        RECT rc;
        SetRect(&rc, 0, 0, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON));
        AdjustWindowRectEx(&rc, GetWindowLong(m_ico1, GWL_STYLE),
            FALSE, GetWindowLong(m_ico1, GWL_EXSTYLE));
        SetWindowPos(m_ico1, NULL, 0, 0,
            rc.right - rc.left, rc.bottom - rc.top,
            SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOOWNERZORDER);

        m_resizable.OnParentCreate(hwnd);

        m_resizable.SetLayoutAnchor(ctl1, mzcLA_TOP_LEFT, mzcLA_BOTTOM_RIGHT);
        m_resizable.SetLayoutAnchor(psh1, mzcLA_BOTTOM_LEFT);
        m_resizable.SetLayoutAnchor(edt1, mzcLA_BOTTOM_LEFT);
        m_resizable.SetLayoutAnchor(stc1, mzcLA_BOTTOM_LEFT);
        m_resizable.SetLayoutAnchor(ico1, mzcLA_BOTTOM_LEFT);
        m_resizable.SetLayoutAnchor(stc2, mzcLA_BOTTOM_RIGHT);
        m_resizable.SetLayoutAnchor(stc3, mzcLA_BOTTOM_RIGHT);
        m_resizable.SetLayoutAnchor(edt2, mzcLA_BOTTOM_RIGHT);
        m_resizable.SetLayoutAnchor(psh2, mzcLA_BOTTOM_RIGHT);
        m_resizable.SetLayoutAnchor(psh3, mzcLA_BOTTOM_RIGHT);
        m_resizable.SetLayoutAnchor(cmb1, mzcLA_BOTTOM_RIGHT);
        m_resizable.SetLayoutAnchor(IDOK, mzcLA_BOTTOM_RIGHT);
        m_resizable.SetLayoutAnchor(IDCANCEL, mzcLA_BOTTOM_RIGHT);

        m_ctl1.set_style(MWTVS_DESKTOPTREE);
        m_ctl1.refresh();

        SetFocus(GetDlgItem(hwnd, psh1));
        return TRUE;
    }

    void OnSize(HWND hwnd, UINT state, int cx, int cy)
    {
        m_resizable.OnSize();
    }

    void OnTimer(HWND hwnd, UINT id)
    {
        KillTimer(hwnd, id);
        switch (id)
        {
        case 888:
            InvalidateRect(GetDesktopWindow(), NULL, TRUE);
            break;
        case 999:
            m_ctl1.refresh();
            break;
        }
    }

    void OnPsh1(HWND hwnd)
    {
        INT i = (INT)SendDlgItemMessage(hwnd, cmb1, CB_GETCURSEL, 0, 0);
        if (i == CB_ERR)
            return;

        switch (i)
        {
        case 0:
            m_ctl1.set_style(MWTVS_DESKTOPTREE);
            break;
        case 1:
            m_ctl1.set_style(MWTVS_PROCESSWINDOW);
            break;
        case 2:
            m_ctl1.set_style(MWTVS_PROCESSTHREAD);
            break;
        }

        UINT uSeconds = GetDlgItemInt(hwnd, edt1, NULL, FALSE);
        SetTimer(hwnd, 999, 1000 * uSeconds, NULL);
    }

    void OnPsh2(HWND hwnd)
    {
        WCHAR szText[128];
        GetDlgItemTextW(hwnd, edt2, szText, ARRAYSIZE(szText));

        mstr_trim(szText, L" \t");

        if (szText[0] == 0)
            return;

        std::vector<HTREEITEM> found;
        m_ctl1.find_text(found, szText);
        if (found.empty())
            return;

        HTREEITEM hItem = m_ctl1.GetSelectedItem();
        HTREEITEM hThisOne = NULL;
        for (size_t i = 1; i < found.size(); ++i)
        {
            if (found[i] == NULL || found[i] == hItem)
            {
                hThisOne = found[i - 1];
                break;
            }
        }

        if (hThisOne == NULL)
            hThisOne = found[found.size() - 1];

        if (hThisOne == TVI_ROOT)
            hThisOne = m_ctl1.GetRootItem();

        m_ctl1.SelectItem(hThisOne);
        SetFocus(m_ctl1);
    }

    void OnPsh3(HWND hwnd)
    {
        WCHAR szText[128];
        GetDlgItemTextW(hwnd, edt2, szText, ARRAYSIZE(szText));

        mstr_trim(szText, L" \t");

        if (szText[0] == 0)
            return;

        std::vector<HTREEITEM> found;
        m_ctl1.find_text(found, szText);
        if (found.empty())
            return;

        HTREEITEM hItem = m_ctl1.GetSelectedItem();
        HTREEITEM hThisOne = NULL;
        for (size_t i = 0; i < found.size() - 1; ++i)
        {
            if (found[i] == NULL || found[i] == hItem)
            {
                hThisOne = found[i + 1];
                break;
            }
        }

        if (hThisOne == NULL)
            hThisOne = found[0];
        
        if (hThisOne == TVI_ROOT)
            hThisOne = m_ctl1.GetRootItem();

        m_ctl1.SelectItem(hThisOne);
        SetFocus(m_ctl1);
    }

    void OnCmb1(HWND hwnd)
    {
        INT i = (INT)SendDlgItemMessage(hwnd, cmb1, CB_GETCURSEL, 0, 0);
        if (i == CB_ERR)
            return;

        switch (i)
        {
        case 0:
            m_ctl1.set_style(MWTVS_DESKTOPTREE);
            break;
        case 1:
            m_ctl1.set_style(MWTVS_PROCESSWINDOW);
            break;
        case 2:
            m_ctl1.set_style(MWTVS_PROCESSTHREAD);
            break;
        }
        m_ctl1.refresh();
    }

    void OnProp(HWND hwnd)
    {
        HTREEITEM hItem = TreeView_GetSelection(m_ctl1);
        MWindowTreeView::node_type *node = m_ctl1.NodeFromItem(hItem);
        if (node)
        {
            DialogBoxParam(m_hInst, MAKEINTRESOURCE(IDD_PROPERTIES), hwnd,
                           PropDialogProc, (LPARAM)node);
        }
    }

    BOOL CopyTextToClipboard(HWND hwnd, const MStringW& text)
    {
        if (::OpenClipboard(hwnd))
        {
            ::EmptyClipboard();

            size_t cb = (text.size() + 1) * sizeof(WCHAR);
            if (HGLOBAL hGlobal = GlobalAlloc(GHND | GMEM_SHARE, cb))
            {
                if (LPWSTR psz = (LPWSTR)GlobalLock(hGlobal))
                {
                    CopyMemory(psz, text.c_str(), cb);
                    GlobalUnlock(hGlobal);
                }

                ::SetClipboardData(CF_UNICODETEXT, hGlobal);
            }

            ::CloseClipboard();
        }
        return TRUE;
    }

    void OnCopyHWND(HWND hwnd)
    {
        HTREEITEM hItem = TreeView_GetSelection(m_ctl1);
        MWindowTreeView::node_type *node = m_ctl1.NodeFromItem(hItem);
        if (node)
        {
            WCHAR szText[16];
            StringCbPrintfW(szText, sizeof(szText), L"%08X",
                (DWORD)(DWORD_PTR)node->m_hwndTarget);
            CopyTextToClipboard(hwnd, szText);
        }
    }

    void OnCopyText(HWND hwnd)
    {
        HTREEITEM hItem = TreeView_GetSelection(m_ctl1);
        MWindowTreeView::node_type *node = m_ctl1.NodeFromItem(hItem);
        if (node)
        {
            MString text = node->m_szText;
            CopyTextToClipboard(hwnd, text);
        }
    }

    void OnCopyClassName(HWND hwnd)
    {
        HTREEITEM hItem = TreeView_GetSelection(m_ctl1);
        MWindowTreeView::node_type *node = m_ctl1.NodeFromItem(hItem);
        if (node)
        {
            MString text = node->m_szClass;
            CopyTextToClipboard(hwnd, text);
        }
    }

    void OnCopyAsText(HWND hwnd)
    {
        HTREEITEM hItem = TreeView_GetSelection(m_ctl1);
        MWindowTreeView::node_type *node = m_ctl1.NodeFromItem(hItem);
        if (node)
        {
            MString text = m_ctl1.text_from_node(node);
            CopyTextToClipboard(hwnd, text);
        }
    }

    void OnMessages(HWND hwnd)
    {
        HTREEITEM hItem = TreeView_GetSelection(m_ctl1);
        HWND hwndTarget = m_ctl1.WindowFromItem(hItem);
        if (IsWindow(hwndTarget))
        {
            DWORD pid = 0;
            GetWindowThreadProcessId(hwndTarget, &pid);

            TCHAR szTargetPath[MAX_PATH];
            GetFileNameFromPID(szTargetPath, ARRAYSIZE(szTargetPath), pid);

            BOOL bIsWow64 = FALSE;
            if (HANDLE hProcess = OpenProcess(GENERIC_READ, TRUE, pid))
            {
                if (IsWow64(hProcess))
                {
                    bIsWow64 = TRUE;
                }
                CloseHandle(hProcess);
            }

            TCHAR szPath[MAX_PATH];
            GetModuleFileName(NULL, szPath, ARRAYSIZE(szPath));
            LPTSTR pch = PathFindFileName(szPath);
            *pch = 0;

            DWORD dwBinType;
            if (bIsWow64)
            {
                PathAppend(szPath, TEXT("MsgGetter32.exe"));
            }
            else if (GetBinaryType(szTargetPath, &dwBinType) && dwBinType == SCS_32BIT_BINARY)
            {
                PathAppend(szPath, TEXT("MsgGetter32.exe"));
            }
            else
            {
                PathAppend(szPath, TEXT("MsgGetter64.exe"));
            }

            TCHAR szText[64];
            StringCbPrintf(szText, sizeof(szText), TEXT("0x%p"), hwndTarget);

            //MessageBox(hwnd, szPath, szText, MB_ICONINFORMATION);

            ShellExecute(hwnd, NULL, szPath, szText, NULL, SW_SHOWNORMAL);
        }
    }

    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
    {
        switch (id)
        {
        case IDCANCEL:
            KillTimer(hwnd, 999);
            EndDialog(IDCANCEL);
            break;
        case psh1:
            OnPsh1(hwnd);
            break;
        case psh2:
            OnPsh2(hwnd);
            break;
        case psh3:
            OnPsh3(hwnd);
            break;
        case cmb1:
            switch (codeNotify)
            {
            case CBN_SELENDOK:
                OnCmb1(hwnd);
                break;
            }
            break;
        case ID_PROP:
            OnProp(hwnd);
            break;
        case ID_MESSAGES:
            OnMessages(hwnd);
            break;
        case ID_WINDOW_CHOOSED:
            m_ctl1.refresh();
            m_ctl1.select_hwnd(m_ico1.GetSelectedWindow());
            break;
        case ID_COPYASTEXT:
            OnCopyAsText(hwnd);
            break;
        case ID_COPYHWND:
            OnCopyHWND(hwnd);
            break;
        case ID_COPYTEXT:
            OnCopyText(hwnd);
            break;
        case ID_COPYCLASSNAME:
            OnCopyClassName(hwnd);
            break;
        case ID_SHOW:
            OnShow(hwnd);
            break;
        case ID_HIDE:
            OnHide(hwnd);
            break;
        case ID_DESTROY:
            OnDestroy(hwnd);
            break;
        case ID_BLINK:
            OnBlink(hwnd);
            break;
        case ID_RESTORE:
            OnRestore(hwnd);
            break;
        case ID_BRINGTOTOP:
            OnBringToTop(hwnd);
            break;
        }
    }

    void OnContextMenu(HWND hwnd, HWND hwndContext, UINT xPos, UINT yPos)
    {
        if (hwndContext != m_ctl1)
            return;

        if (xPos == 0xFFFF && yPos == 0xFFFF)
        {
            RECT rc;
            GetWindowRect(m_ctl1, &rc);
            xPos = rc.left;
            yPos = rc.top;
        }
        else
        {
            TV_HITTESTINFO hittest;
            hittest.pt.x = xPos;
            hittest.pt.y = yPos;
            ScreenToClient(m_ctl1, &hittest.pt);
            TreeView_HitTest(m_ctl1, &hittest);
            TreeView_SelectItem(m_ctl1, hittest.hItem);
        }

        HMENU hMenu = LoadMenu(m_hInst, MAKEINTRESOURCE(1));
        HMENU hSubMenu = GetSubMenu(hMenu, 0);

        SetForegroundWindow(hwnd);
        INT nCmd = TrackPopupMenu(hSubMenu, TPM_RIGHTBUTTON | TPM_LEFTALIGN | TPM_RETURNCMD,
            xPos, yPos, 0, hwnd, NULL);
        DestroyMenu(hMenu);
        PostMessage(hwnd, WM_COMMAND, nCmd, 0);
    }

    void OnShow(HWND hwnd)
    {
        HTREEITEM hItem = TreeView_GetSelection(m_ctl1);
        HWND hwndTarget = m_ctl1.WindowFromItem(hItem);
        ShowWindow(hwndTarget, SW_SHOWNOACTIVATE);
    }

    void OnHide(HWND hwnd)
    {
        HTREEITEM hItem = TreeView_GetSelection(m_ctl1);
        HWND hwndTarget = m_ctl1.WindowFromItem(hItem);
        ShowWindow(hwndTarget, SW_HIDE);
    }

    void OnDestroy(HWND hwnd)
    {
        HTREEITEM hItem = TreeView_GetSelection(m_ctl1);
        HWND hwndTarget = m_ctl1.WindowFromItem(hItem);
        PostMessage(hwndTarget, WM_CLOSE, 0, 0);
    }

    void OnBlink(HWND hwnd)
    {
        HTREEITEM hItem = TreeView_GetSelection(m_ctl1);
        HWND hwndTarget = m_ctl1.WindowFromItem(hItem);

        for (int i = 0; i < 4; ++i)
        {
            if (HDC hdc = CreateDC(TEXT("DISPLAY"), NULL, NULL, NULL))
            {
                const INT PEN_WIDTH = 5;
                if (HPEN hPen = CreatePen(PS_SOLID, PEN_WIDTH, RGB(0, 255, 255)))
                {
                    SetROP2(hdc, R2_XORPEN);
                    SelectObject(hdc, hPen);
                    SelectObject(hdc, GetStockObject(NULL_BRUSH));

                    RECT rc;
                    GetWindowRect(hwndTarget, &rc);
                    InflateRect(&rc, -PEN_WIDTH, -PEN_WIDTH);
                    Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);

                    DeleteObject(hPen);
                }
                DeleteDC(hdc);
            }
            Sleep(200);
        }
        SetTimer(hwnd, 888, 500, NULL);
    }

    void OnRestore(HWND hwnd)
    {
        HTREEITEM hItem = TreeView_GetSelection(m_ctl1);
        HWND hwndTarget = m_ctl1.WindowFromItem(hItem);
        ShowWindow(hwndTarget, SW_RESTORE);
    }

    void OnBringToTop(HWND hwnd)
    {
        HTREEITEM hItem = TreeView_GetSelection(m_ctl1);
        HWND hwndTarget = m_ctl1.WindowFromItem(hItem);
        BringWindowToTop(hwndTarget);
    }

    void OnInitMenuPopup(HWND hwnd, HMENU hMenu, UINT item, BOOL fSystemMenu)
    {
        HTREEITEM hItem = TreeView_GetSelection(m_ctl1);
        HWND hwndTarget = m_ctl1.WindowFromItem(hItem);
        if (IsWindowVisible(hwndTarget))
        {
            EnableMenuItem(hMenu, ID_SHOW, MF_BYCOMMAND | MF_GRAYED);
            EnableMenuItem(hMenu, ID_HIDE, MF_BYCOMMAND | MF_ENABLED);
        }
        else
        {
            EnableMenuItem(hMenu, ID_SHOW, MF_BYCOMMAND | MF_ENABLED);
            EnableMenuItem(hMenu, ID_HIDE, MF_BYCOMMAND | MF_GRAYED);
        }
    }

    virtual INT_PTR CALLBACK
    DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
        HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
        HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
        HANDLE_MSG(hwnd, WM_SIZE, OnSize);
        HANDLE_MSG(hwnd, WM_TIMER, OnTimer);
        HANDLE_MSG(hwnd, WM_CONTEXTMENU, OnContextMenu);
        HANDLE_MSG(hwnd, WM_INITMENUPOPUP, OnInitMenuPopup);
        default:
            return DefaultProcDx();
        }
    }

    BOOL StartDx(INT nCmdShow)
    {
        return TRUE;
    }

    INT RunDx()
    {
        DialogBoxDx(NULL);
        return 0;
    }
};

//////////////////////////////////////////////////////////////////////////////
// Win32 main function

extern "C"
INT APIENTRY WinMain(
    HINSTANCE   hInstance,
    HINSTANCE   hPrevInstance,
    LPSTR       lpCmdLine,
    INT         nCmdShow)
{
    int ret = -1;

    {
        WinHier app(hInstance);

        ::InitCommonControls();

        if (app.StartDx(nCmdShow))
        {
            ret = app.RunDx();
        }
    }

#if (WINVER >= 0x0500)
    HANDLE hProcess = GetCurrentProcess();
    DebugPrintDx(TEXT("Count of GDI objects: %ld\n"),
                 GetGuiResources(hProcess, GR_GDIOBJECTS));
    DebugPrintDx(TEXT("Count of USER objects: %ld\n"),
                 GetGuiResources(hProcess, GR_USEROBJECTS));
#endif

#if defined(_MSC_VER) && !defined(NDEBUG)
    // for detecting memory leak (MSVC only)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    return ret;
}

//////////////////////////////////////////////////////////////////////////////

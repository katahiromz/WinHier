// MSelectWnd.hpp
// Copyright (C) 2018 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// This file is public domain software.
//////////////////////////////////////////////////////////////////////////////

#ifndef MSELECTWND_HPP_
#define MSELECTWND_HPP_ 3

#include "MWindowBase.hpp"
#include <strsafe.h>
#include "resource.h"

struct SELECTWNDDATA;
class MSelectWndIconWnd;
class MSelectWndDlg;

//////////////////////////////////////////////////////////////////////////////

// common data for selecting window
struct SELECTWNDDATA
{
    HICON m_hIconNull;
    HCURSOR m_hTargetCursor;
    BOOL m_bSelectTopLevelOnly;

    SELECTWNDDATA()
    {
        m_hIconNull = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_NULL));
        m_hTargetCursor = LoadCursor(GetModuleHandle(NULL), MAKEINTRESOURCE(IDC_TARGET));
        m_bSelectTopLevelOnly = FALSE;
        assert(m_hIconNull);
        assert(m_hTargetCursor);
    }
    ~SELECTWNDDATA()
    {
        DestroyIcon(m_hIconNull);
        DestroyCursor(m_hTargetCursor);
    }
};

//////////////////////////////////////////////////////////////////////////////

class MSelectWndIconWnd : public MWindowBase
{
protected:
    SELECTWNDDATA& m_data;
    HWND m_hParentWnd;
    HPEN m_hPen;
    DWORD m_nDelayCount;
    HWND m_hwndSelected;
    HWND m_hwndOver;
    HWND m_hwndOverOld;
    RECT m_rcSave;
    RECT m_rcSave2;

    enum
    {
        PEN_WIDTH = 2,
        HIDE_DELAY = 8,
        REDRAW_DELAY = 12
    };

public:
    HWND GetSelectedWindow() const
    {
        return m_hwndSelected;
    }

    void SetNullIcon(HWND hwnd)
    {
        SendMessage(hwnd, STM_SETICON, (WPARAM)m_data.m_hIconNull, 0);
    }

    void SetTargetIcon(HWND hwnd)
    {
        SendMessage(hwnd, STM_SETIMAGE, IMAGE_CURSOR, (LPARAM)m_data.m_hTargetCursor);
    }

    MSelectWndIconWnd(SELECTWNDDATA& data) :
        m_data(data),
        m_hParentWnd(NULL),
        m_hPen(NULL),
        m_nDelayCount(0),
        m_hwndSelected(NULL),
        m_hwndOver(NULL),
        m_hwndOverOld(NULL)
    {
    }

    virtual void PostSubclassDx(HWND hwnd)
    {
        m_hParentWnd = GetParent(hwnd);
        m_hPen = CreatePen(PS_SOLID, PEN_WIDTH, RGB(255, 255, 255));
        m_hwndSelected = NULL;
        m_hwndOver = NULL;
        m_hwndOverOld = NULL;
    }

    void PreUnsubclassDx()
    {
        DeleteObject(m_hPen);
        m_hPen = NULL;
    }

    virtual LRESULT CALLBACK
    WindowProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
        HANDLE_MSG(hwnd, WM_LBUTTONDOWN, OnLButtonDown);
        HANDLE_MSG(hwnd, WM_LBUTTONUP, OnLButtonUp);
        HANDLE_MSG(hwnd, WM_MOUSEMOVE, OnMouseMove);
        HANDLE_MESSAGE(hwnd, WM_CAPTURECHANGED, OnCaptureChanged);
        default:
            return DefaultProcDx();
        }
    }

    void OnLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
    {
        SetCapture(hwnd);   // dragging starts

        SetNullIcon(hwnd);

        m_hwndOver = NULL;
        m_hwndOverOld = NULL;
        m_nDelayCount = 0;
        SetCursor(m_data.m_hTargetCursor);
    }

    MString CreateInfoText() const
    {
        return CreateInfoText(m_hwndSelected);
    }

    MString CreateInfoText(HWND hwndSelected) const
    {
        TCHAR szText[64];
        StringCbPrintf(szText, sizeof(szText), TEXT("%08lX"), (ULONG)(ULONG_PTR)hwndSelected);
        return szText;
    }

    void OnLButtonUp(HWND hwnd, int x, int y, UINT keyFlags)
    {
        if (GetCapture() != hwnd)
            return;

        ReleaseCapture();   // dragging ends

        if (m_hwndOver != NULL)
        {
            m_hwndSelected = m_hwndOver;
        }

        SetTargetIcon(hwnd);
        PostMessage(m_hParentWnd, WM_COMMAND, ID_WINDOW_CHOOSED, 0);

        ShowWindow(m_hParentWnd, SW_SHOWNORMAL);  // activate
    }

    void DrawBox(HDC hdc, HWND hwndDraw)
    {
        RECT rc;
        GetWindowRect(hwndDraw, &rc);
        InflateRect(&rc, -PEN_WIDTH, -PEN_WIDTH);
        Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
    }

    void OnMouseMove(HWND hwnd, int x, int y, UINT keyFlags)
    {
        if (GetCapture() != hwnd)
            return;

        if (m_nDelayCount < HIDE_DELAY)    // delay hiding window
        {
            m_nDelayCount++;
            if (m_nDelayCount == HIDE_DELAY)
            {
                // save the position
                GetWindowRect(m_hParentWnd, &m_rcSave);

                // move window to invisible area
                MoveWindow(m_hParentWnd, -32768, -32768, 0, 0, TRUE);

                if (HWND hwndOwner = GetWindow(m_hParentWnd, GW_OWNER))
                {
                    GetWindowRect(hwndOwner, &m_rcSave2);
                    MoveWindow(hwndOwner, -32768, -32768, 0, 0, TRUE);
                }
            }
            SetCursor(m_data.m_hTargetCursor);
            return;
        }

        if (m_nDelayCount < REDRAW_DELAY)   // delay for redraw
        {
            m_nDelayCount++;
            SetCursor(m_data.m_hTargetCursor);
            return;
        }

        POINT pt;
        GetCursorPos(&pt);
        m_hwndOverOld = m_hwndOver;
        m_hwndOver = WindowFromPoint(pt);

        if (m_data.m_bSelectTopLevelOnly)
        {
            // get top level window
            while (HWND hwndParent = GetParent(m_hwndOver))
            {
                m_hwndOver = hwndParent;
            }
        }

        if (m_hwndOverOld != m_hwndOver)
        {
            HWND hwndDesktop = GetDesktopWindow();
            if (HDC hdc = GetWindowDC(hwndDesktop))
            {
                SetROP2(hdc, R2_XORPEN);
                SelectObject(hdc, m_hPen);
                SelectObject(hdc, GetStockObject(NULL_BRUSH));

                // erase previous box
                if (m_hwndOverOld != NULL)
                {
                    DrawBox(hdc, m_hwndOverOld);
                }

                // draw box
                if (m_hwndOver != NULL)
                {
                    DrawBox(hdc, m_hwndOver);
                }

                ReleaseDC(hwndDesktop, hdc);
            }
        }

        SetCursor(m_data.m_hTargetCursor);
    }

    LRESULT OnCaptureChanged(HWND hwnd, WPARAM wParam, LPARAM lParam)
    {
        if (m_hwndOverOld)
        {
            // erase previous box
            HWND hwndDesktop = GetDesktopWindow();
            if (HDC hdc = GetWindowDC(hwndDesktop))
            {
                SetROP2(hdc, R2_XORPEN);
                SelectObject(hdc, m_hPen);
                SelectObject(hdc, GetStockObject(NULL_BRUSH));

                DrawBox(hdc, m_hwndOverOld);

                ReleaseDC(hwndDesktop, hdc);
            }
        }

        // restore pos & size
        MoveWindow(m_hParentWnd,
            m_rcSave.left, m_rcSave.top,
            m_rcSave.right - m_rcSave.left,
            m_rcSave.bottom - m_rcSave.top, TRUE);

        if (HWND hwndOwner = GetWindow(m_hParentWnd, GW_OWNER))
        {
            MoveWindow(hwndOwner,
                m_rcSave2.left, m_rcSave2.top,
                m_rcSave2.right - m_rcSave2.left,
                m_rcSave2.bottom - m_rcSave2.top, TRUE);
        }

        SetTargetIcon(hwnd);
        return 0;
    }
};

//////////////////////////////////////////////////////////////////////////////

#endif  // ndef MSELECTWND_HPP_

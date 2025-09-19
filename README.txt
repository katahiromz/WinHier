# WinHier by katahiromz

## What's this?

WinHier is a useful utility to investigate window hierarchy, properties and messages.
Freeware.

Download: https://katahiromz.web.fc2.com/winhier/en

Platforms: Windows 2000/XP/Vista/7/10/11

## How to install/uninstall

Follow the instructions of the installer to install.
You can uninstall it in the standard way.

This software does not use the registry, so if it was without the installer,
please delete the files.

## Usage

When you launched, the window hierarchy is displayed in the tree view.

If you want to check the properties and messages,
please right-click the treeview item and choose the appropriate menu item.

## History

- 2019-05-12 ver.1.4
    - First release.
- 2019-05-29 ver.1.5
    - Able to save properties as a text file.
- 2019-05-30 ver.1.6
    - Improved saving.
    - Don't search empty string.
    - Added version info to MsgGetter.
    - Improved installer.
- 2019-06-17 ver.1.7
    - Support of registered window messages.
    - Able to copy a node as text.
    - Supported ListView messages.
    - Supported TreeView messages.
- 2019-08-14 ver.1.8
    - Correctly treat DIALOG STYLE.
    - Power to WM_NOTIFY.
    - Power to RichEdit.
    - Fixed "Copy Lines".
- 2019-10-13 ver.1.9
    - Add "Copy HWND", "Copy window &text" and "Copy window class &name" items to the context menu.
    - Add "Forcibly show", "Forcibly hide" and "Forcibly &destroy" items to the context menu.
    - Add "Blink" item to the context menu.
- 2019-11-22 ver.2.0
    - Add "Restore maximized/minimized" item to the context menu.
    - Add "Bring to top" item to the context menu.
    - Support WM_SETTINGCHANGE message.
- 2020-03-13 ver.2.1
    - XP support.
- 2021-03-18 ver.2.2
    - RegisterWindowMessage support.
- 2022-04-23 ver.2.3
    - Fixed parent handle against WS_POPUP and WS_CHILD.
- 2023-03-27 ver.2.4
    - Fixed a bug that caused abnormal termination when searching for a specific string.
- 2025-02-12 ver.2.5
    - Finding text is now case-insensitive.
- 2025-06-25 ver.2.6
    - Officially supported Windows 11.
    - Able to get message outputs from the other process.
    - Fixed message processing partially.
- 2025-07-01 ver.2.7
    - Handling WoW64 Filesystem redirection.
    - Officially supported Windows 2000.
    - Added digital signature.
    - Updated main icon.
    - Added WM_CANCELMODE handler. Redrawed when capture is canceled.
- 2025-09-19 ver.2.8
    - Updated main icon.

## Contact Us

Katayama Hirofumi MZ (katahiromz) katayama.hirofumi.mz@gmail.com

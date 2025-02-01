#include <windows.h>
#include <stdio.h>
#include <commctrl.h>
#include <psapi.h>
#include <time.h>
#include <Uxtheme.h>
#pragma comment(lib, "UxTheme.lib")
#pragma comment(linker, "/subsystem:windows /ENTRY:mainCRTStartup")

#define ID_CHECK_BUTTON      1001
#define ID_UNLOCK_BUTTON     1002
#define ID_KILL_BUTTON       1003
#define ID_STATUS_TEXT       1004
#define ID_REFRESH_TIMER     1005
#define ID_AUTO_REFRESH      1006
#define ID_COPY_PID          1007
#define ID_PROCESS_LIST      1008
#define ID_CLEAR_CLIPBOARD   1009
#define ID_PREVIEW_TEXT      1010
#define ID_FORMAT_COMBO      1011
#define MAX_PREVIEW_SIZE     32768
#define FORMAT_NAME_BUFFER   512

// Global handles for controls and background brush.
HWND statusText, checkButton, unlockButton, killButton, autoRefreshCheck;
HWND copyPidButton, clearClipboardButton, processList, previewText, formatCombo;
HWND groupActions, groupStatus, groupProcess, groupPreview;
HBRUSH hBrushBackground = NULL; // Custom background brush
BOOL autoRefreshEnabled = FALSE;

// Function prototypes.
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void CreateControls(HWND hwnd);
void UpdateClipboardStatus(HWND hwnd);
BOOL AttemptClipboardUnlock(HWND hwnd);
BOOL KillClipboardOwner(HWND hwnd);
void GetProcessInfo(DWORD processId, wchar_t* buffer, size_t bufferSize);
void CopyProcessIdToClipboard(DWORD processId);
void ClearClipboard(void);
void EnableAutoRefresh(HWND hwnd, BOOL enable);
void UpdatePreviewArea(UINT format);
const wchar_t* GetFormatName(UINT format);
void RepositionControls(HWND hwnd);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Initialize common controls.
    INITCOMMONCONTROLSEX icex = { sizeof(INITCOMMONCONTROLSEX), ICC_WIN95_CLASSES | ICC_LISTVIEW_CLASSES };
    InitCommonControlsEx(&icex);

    // Register window class.
    WNDCLASSW wc = {0};
    wc.lpfnWndProc   = WindowProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = L"ClipboardManagerClass";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.hIcon         = LoadIconW(hInstance, MAKEINTRESOURCE(1));
    if (!RegisterClassW(&wc)) {
        MessageBoxW(NULL, L"Window Registration Failed!", L"Error", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    // Create a fixed-size (non-resizable) window.
    HWND hwnd = CreateWindowExW(
        0,
        L"ClipboardManagerClass",
        L"Advanced Clipboard Manager",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        1000, 600,
        NULL,
        NULL,
        hInstance,
        NULL
    );
    if (!hwnd) return 0;

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // Message loop.
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch(uMsg) {
        case WM_CREATE:
            // Create a light background brush.
            hBrushBackground = CreateSolidBrush(RGB(245, 245, 245));
            CreateControls(hwnd);
            break;

        case WM_ERASEBKGND: {
            RECT rect;
            GetClientRect(hwnd, &rect);
            FillRect((HDC)wParam, &rect, hBrushBackground);
            return 1;
        }

        case WM_SIZE:
            RepositionControls(hwnd);
            break;

        case WM_COMMAND:
            switch(LOWORD(wParam)) {
                case ID_CHECK_BUTTON:
                    UpdateClipboardStatus(hwnd);
                    break;
                case ID_UNLOCK_BUTTON:
                    AttemptClipboardUnlock(hwnd);
                    UpdateClipboardStatus(hwnd);
                    break;
                case ID_KILL_BUTTON:
                    if (MessageBoxW(hwnd, L"Are you sure you want to terminate the clipboard owner process?",
                        L"Confirm Process Termination", MB_YESNO | MB_ICONWARNING) == IDYES) {
                        if (KillClipboardOwner(hwnd)) {
                            SetWindowTextW(statusText, L"Process terminated successfully");
                        } else {
                            SetWindowTextW(statusText, L"Failed to terminate process");
                        }
                        UpdateClipboardStatus(hwnd);
                    }
                    break;
                case ID_AUTO_REFRESH:
                    EnableAutoRefresh(hwnd, SendMessage(autoRefreshCheck, BM_GETCHECK, 0, 0) == BST_CHECKED);
                    break;
                case ID_COPY_PID: {
                    HWND clipboardOwner = GetClipboardOwner();
                    if (clipboardOwner) {
                        DWORD processId;
                        GetWindowThreadProcessId(clipboardOwner, &processId);
                        CopyProcessIdToClipboard(processId);
                    }
                    break;
                }
                case ID_CLEAR_CLIPBOARD:
                    ClearClipboard();
                    UpdateClipboardStatus(hwnd);
                    break;
                case ID_FORMAT_COMBO:
                    if (HIWORD(wParam) == CBN_SELCHANGE) {
                        int sel = (int)SendMessage(formatCombo, CB_GETCURSEL, 0, 0);
                        if (sel != CB_ERR) {
                            UINT format = (UINT)SendMessage(formatCombo, CB_GETITEMDATA, sel, 0);
                            UpdatePreviewArea(format);
                        }
                    }
                    break;
            }
            break;

        case WM_TIMER:
            if (wParam == ID_REFRESH_TIMER)
                UpdateClipboardStatus(hwnd);
            break;

        case WM_DESTROY:
            if (autoRefreshEnabled)
                KillTimer(hwnd, ID_REFRESH_TIMER);
            if (hBrushBackground)
                DeleteObject(hBrushBackground);
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

void CreateControls(HWND hwnd) {
    // Use a modern system font (Segoe UI) for most controls.
    HFONT hSegoe = CreateFontW(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

    // --- Group Box: Clipboard Actions ---
    groupActions = CreateWindowW(
        L"BUTTON", L"Clipboard Actions",
        WS_VISIBLE | WS_CHILD | BS_GROUPBOX,
        20, 10, 460, 120,
        hwnd, NULL, GetModuleHandle(NULL), NULL
    );
    SendMessage(groupActions, WM_SETFONT, (WPARAM)hSegoe, TRUE);

    checkButton = CreateWindowW(
        L"BUTTON", L"Check Clipboard",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        30, 35, 130, 30,
        hwnd, (HMENU)ID_CHECK_BUTTON,
        NULL, NULL
    );
    SendMessage(checkButton, WM_SETFONT, (WPARAM)hSegoe, TRUE);

    unlockButton = CreateWindowW(
        L"BUTTON", L"Force Unlock",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        180, 35, 130, 30,
        hwnd, (HMENU)ID_UNLOCK_BUTTON,
        NULL, NULL
    );
    SendMessage(unlockButton, WM_SETFONT, (WPARAM)hSegoe, TRUE);

    killButton = CreateWindowW(
        L"BUTTON", L"Kill Owner Process",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        330, 35, 130, 30,
        hwnd, (HMENU)ID_KILL_BUTTON,
        NULL, NULL
    );
    SendMessage(killButton, WM_SETFONT, (WPARAM)hSegoe, TRUE);

    copyPidButton = CreateWindowW(
        L"BUTTON", L"Copy PID",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        30, 75, 130, 30,
        hwnd, (HMENU)ID_COPY_PID,
        NULL, NULL
    );
    SendMessage(copyPidButton, WM_SETFONT, (WPARAM)hSegoe, TRUE);

    clearClipboardButton = CreateWindowW(
        L"BUTTON", L"Clear Clipboard",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        180, 75, 130, 30,
        hwnd, (HMENU)ID_CLEAR_CLIPBOARD,
        NULL, NULL
    );
    SendMessage(clearClipboardButton, WM_SETFONT, (WPARAM)hSegoe, TRUE);

    autoRefreshCheck = CreateWindowW(
        L"BUTTON", L"Auto Refresh (1s)",
        WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
        330, 75, 130, 30,
        hwnd, (HMENU)ID_AUTO_REFRESH,
        NULL, NULL
    );
    SendMessage(autoRefreshCheck, WM_SETFONT, (WPARAM)hSegoe, TRUE);

    // --- Group Box: Clipboard Status ---
    groupStatus = CreateWindowW(
        L"BUTTON", L"Clipboard Status",
        WS_VISIBLE | WS_CHILD | BS_GROUPBOX,
        20, 140, 460, 440,
        hwnd, NULL, GetModuleHandle(NULL), NULL
    );
    SendMessage(groupStatus, WM_SETFONT, (WPARAM)hSegoe, TRUE);

    statusText = CreateWindowW(
        L"EDIT", L"Click 'Check Clipboard' to begin...",
        WS_VISIBLE | WS_CHILD | ES_MULTILINE | ES_READONLY | WS_VSCROLL,
        30, 160, 440, 410,
        hwnd, (HMENU)ID_STATUS_TEXT,
        NULL, NULL
    );
    SendMessage(statusText, WM_SETFONT, (WPARAM)hSegoe, TRUE);

    // --- Group Box: Process Information ---
    groupProcess = CreateWindowW(
        L"BUTTON", L"Process Information",
        WS_VISIBLE | WS_CHILD | BS_GROUPBOX,
        520, 10, 460, 200,
        hwnd, NULL, GetModuleHandle(NULL), NULL
    );
    SendMessage(groupProcess, WM_SETFONT, (WPARAM)hSegoe, TRUE);

    processList = CreateWindowExW(
        0, WC_LISTVIEWW, L"",
        WS_VISIBLE | WS_CHILD | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
        530, 30, 420, 160,
        hwnd, (HMENU)ID_PROCESS_LIST,
        GetModuleHandle(NULL), NULL
    );
    SendMessage(processList, WM_SETFONT, (WPARAM)hSegoe, TRUE);
    ListView_SetExtendedListViewStyle(processList, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    SetWindowTheme(processList, L"Explorer", NULL);

    LVCOLUMNW lvc = {0};
    lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
    lvc.iSubItem = 0;
    lvc.cx = 100;
    lvc.pszText = L"PID";
    ListView_InsertColumn(processList, 0, &lvc);

    lvc.iSubItem = 1;
    lvc.cx = 200;
    lvc.pszText = L"Process Name";
    ListView_InsertColumn(processList, 1, &lvc);

    lvc.iSubItem = 2;
    lvc.cx = 420;
    lvc.pszText = L"Window Title";
    ListView_InsertColumn(processList, 2, &lvc);

    // --- Group Box: Clipboard Preview ---
    groupPreview = CreateWindowW(
        L"BUTTON", L"Clipboard Preview",
        WS_VISIBLE | WS_CHILD | BS_GROUPBOX,
        520, 220, 460, 360,
        hwnd, NULL, GetModuleHandle(NULL), NULL
    );
    SendMessage(groupPreview, WM_SETFONT, (WPARAM)hSegoe, TRUE);

    // Combo box for selecting clipboard format.
    formatCombo = CreateWindowW(
        L"COMBOBOX", L"",
        WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL,
        530, 240, 440, 25,
        hwnd, (HMENU)ID_FORMAT_COMBO,
        NULL, NULL
    );
    SendMessage(formatCombo, WM_SETFONT, (WPARAM)hSegoe, TRUE);

    // Preview area with a fixed-width font (Consolas).
    previewText = CreateWindowW(
        L"EDIT", L"",
        WS_VISIBLE | WS_CHILD | ES_MULTILINE | ES_READONLY | WS_VSCROLL | WS_HSCROLL | ES_NOHIDESEL,
        530, 270, 440, 260,
        hwnd, (HMENU)ID_PREVIEW_TEXT,
        NULL, NULL
    );
    HFONT hConsolas = CreateFontW(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, L"Consolas");
    SendMessage(previewText, WM_SETFONT, (WPARAM)hConsolas, TRUE);
}

void GetProcessInfo(DWORD processId, wchar_t* buffer, size_t bufferSize) {
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
    if (hProcess) {
        wchar_t processName[MAX_PATH];
        if (GetModuleBaseNameW(hProcess, NULL, processName, MAX_PATH))
            _snwprintf_s(buffer, bufferSize, _TRUNCATE, L"Process: %s (PID: %lu)", processName, processId);
        else
            _snwprintf_s(buffer, bufferSize, _TRUNCATE, L"Process ID: %lu (Name unavailable)", processId);
        CloseHandle(hProcess);
    } else {
        _snwprintf_s(buffer, bufferSize, _TRUNCATE, L"Process ID: %lu (Access denied)", processId);
    }
}

const wchar_t* GetFormatName(UINT format) {
    static wchar_t formatName[FORMAT_NAME_BUFFER];
    switch (format) {
        case CF_TEXT:
            return L"CF_TEXT (1)";
        case CF_BITMAP:
            return L"CF_BITMAP (2)";
        case CF_METAFILEPICT:
            return L"CF_METAFILEPICT (3)";
        case CF_SYLK:
            return L"CF_SYLK (4)";
        case CF_DIF:
            return L"CF_DIF (5)";
        case CF_TIFF:
            return L"CF_TIFF (6)";
        case CF_OEMTEXT:
            return L"CF_OEMTEXT (7)";
        case CF_DIB:
            return L"CF_DIB (8)";
        case CF_PALETTE:
            return L"CF_PALETTE (9)";
        case CF_PENDATA:
            return L"CF_PENDATA (10)";
        case CF_RIFF:
            return L"CF_RIFF (11)";
        case CF_WAVE:
            return L"CF_WAVE (12)";
        case CF_UNICODETEXT:
            return L"CF_UNICODETEXT (13)";
        case CF_ENHMETAFILE:
            return L"CF_ENHMETAFILE (14)";
        case CF_HDROP:
            return L"CF_HDROP (15)";
        case CF_LOCALE:
            return L"CF_LOCALE (16)";
        case CF_DIBV5:
            return L"CF_DIBV5 (17)";
        default: {
            wchar_t customFormatName[256] = L"";
            if (GetClipboardFormatNameW(format, customFormatName, _countof(customFormatName)) > 0)
                _snwprintf_s(formatName, FORMAT_NAME_BUFFER, _TRUNCATE, L"%s (%u)", customFormatName, format);
            else
                _snwprintf_s(formatName, FORMAT_NAME_BUFFER, _TRUNCATE, L"Unknown Format (%u)", format);
            return formatName;
        }
    }
}

void UpdateClipboardStatus(HWND hwnd) {
    HWND clipboardOwner = GetClipboardOwner();
    DWORD processId = 0;
    wchar_t statusBuffer[4096] = {0};
    wchar_t timeStr[64] = {0};
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_s(&timeinfo, &now);
    wcsftime(timeStr, sizeof(timeStr) / sizeof(wchar_t), L"%Y-%m-%d %H:%M:%S", &timeinfo);

    _snwprintf_s(statusBuffer, _countof(statusBuffer), _TRUNCATE,
        L"Clipboard Status Check - %s\r\n----------------------------------------\r\n", timeStr);

    if (!OpenClipboard(NULL)) {
        wcscat_s(statusBuffer, _countof(statusBuffer), L"Clipboard is locked!\r\n");
        if (clipboardOwner != NULL) {
            GetWindowThreadProcessId(clipboardOwner, &processId);
            wchar_t processInfo[256];
            GetProcessInfo(processId, processInfo, _countof(processInfo));
            wcscat_s(statusBuffer, _countof(statusBuffer), processInfo);
            wcscat_s(statusBuffer, _countof(statusBuffer), L"\r\n");
        }
    } else {
        wcscat_s(statusBuffer, _countof(statusBuffer), L"Clipboard is available\r\n\r\nAvailable formats:\r\n");
        UINT format = 0;
        while ((format = EnumClipboardFormats(format)) != 0) {
            wchar_t formatInfo[512];
            _snwprintf_s(formatInfo, _countof(formatInfo), _TRUNCATE, L"  - %s\r\n", GetFormatName(format));
            wcscat_s(statusBuffer, _countof(statusBuffer), formatInfo);
        }
        // Update the combo box with available formats.
        SendMessage(formatCombo, CB_RESETCONTENT, 0, 0);
        format = 0;
        while ((format = EnumClipboardFormats(format)) != 0) {
            int index = (int)SendMessage(formatCombo, CB_ADDSTRING, 0, (LPARAM)GetFormatName(format));
            SendMessage(formatCombo, CB_SETITEMDATA, index, format);
        }
        if (SendMessage(formatCombo, CB_GETCOUNT, 0, 0) > 0) {
            SendMessage(formatCombo, CB_SETCURSEL, 0, 0);
            UINT selectedFormat = (UINT)SendMessage(formatCombo, CB_GETITEMDATA, 0, 0);
            UpdatePreviewArea(selectedFormat);
        } else {
            SetWindowTextW(previewText, L"No clipboard data available");
        }
        CloseClipboard();
    }
    SetWindowTextW(statusText, statusBuffer);

    // Update the ListView with process info.
    ListView_DeleteAllItems(processList);
    if (processId != 0) {
        LVITEMW lvi = {0};
        wchar_t pidStr[32];
        lvi.mask = LVIF_TEXT;
        lvi.iItem = 0;
        _snwprintf_s(pidStr, _countof(pidStr), _TRUNCATE, L"%lu", processId);
        lvi.iSubItem = 0;
        lvi.pszText = pidStr;
        ListView_InsertItem(processList, &lvi);

        wchar_t processName[MAX_PATH] = L"";
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
        if (hProcess) {
            if (GetModuleBaseNameW(hProcess, NULL, processName, MAX_PATH)) {
                lvi.iSubItem = 1;
                lvi.pszText = processName;
                ListView_SetItem(processList, &lvi);
            }
            CloseHandle(hProcess);
        }
        wchar_t windowTitle[256] = L"";
        if (GetWindowTextW(clipboardOwner, windowTitle, _countof(windowTitle)) > 0) {
            lvi.iSubItem = 2;
            lvi.pszText = windowTitle;
            ListView_SetItem(processList, &lvi);
        }
    }
}

void CopyProcessIdToClipboard(DWORD processId) {
    if (OpenClipboard(NULL)) {
        EmptyClipboard();
        wchar_t pidStr[32];
        _snwprintf_s(pidStr, _countof(pidStr), _TRUNCATE, L"%lu", processId);
        size_t len = (wcslen(pidStr) + 1) * sizeof(wchar_t);
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, len);
        if (hMem) {
            memcpy(GlobalLock(hMem), pidStr, len);
            GlobalUnlock(hMem);
            SetClipboardData(CF_UNICODETEXT, hMem);
        }
        CloseClipboard();
    }
}

void ClearClipboard(void) {
    if (OpenClipboard(NULL)) {
        EmptyClipboard();
        CloseClipboard();
    }
}

void EnableAutoRefresh(HWND hwnd, BOOL enable) {
    if (enable)
        SetTimer(hwnd, ID_REFRESH_TIMER, 1000, NULL);  // 1-second interval.
    else
        KillTimer(hwnd, ID_REFRESH_TIMER);
    autoRefreshEnabled = enable;
}

BOOL AttemptClipboardUnlock(HWND hwnd) {
    // Attempt to force-close and reopen the clipboard.
    CloseClipboard();
    if (OpenClipboard(NULL)) {
        CloseClipboard();
        return TRUE;
    }
    return FALSE;
}

BOOL KillClipboardOwner(HWND hwnd) {
    HWND clipboardOwner = GetClipboardOwner();
    if (clipboardOwner == NULL)
        return FALSE;
    DWORD processId;
    GetWindowThreadProcessId(clipboardOwner, &processId);
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, processId);
    if (hProcess == NULL)
        return FALSE;
    BOOL result = TerminateProcess(hProcess, 1);
    CloseHandle(hProcess);
    return result;
}

void UpdatePreviewArea(UINT format) {
    if (!OpenClipboard(NULL)) {
        SetWindowTextW(previewText, L"Cannot access clipboard");
        return;
    }
    HANDLE hData = GetClipboardData(format);
    if (hData == NULL) {
        SetWindowTextW(previewText, L"No data available in this format");
        CloseClipboard();
        return;
    }
    wchar_t previewBuffer[MAX_PREVIEW_SIZE] = {0};
    switch (format) {
        case CF_TEXT:
        case CF_OEMTEXT: {
            char* text = (char*)GlobalLock(hData);
            if (text) {
                MultiByteToWideChar(CP_ACP, 0, text, -1, previewBuffer, MAX_PREVIEW_SIZE);
                GlobalUnlock(hData);
            }
            break;
        }
        case CF_UNICODETEXT: {
            wchar_t* text = (wchar_t*)GlobalLock(hData);
            if (text) {
                wcsncpy_s(previewBuffer, MAX_PREVIEW_SIZE, text, _TRUNCATE);
                GlobalUnlock(hData);
            }
            break;
        }
        case CF_BITMAP:
            wcscpy_s(previewBuffer, MAX_PREVIEW_SIZE, L"[Bitmap data present - preview not supported]");
            break;
        case CF_HDROP: {
            HDROP hDrop = (HDROP)GlobalLock(hData);
            if (hDrop) {
                UINT fileCount = DragQueryFileW(hDrop, 0xFFFFFFFF, NULL, 0);
                int offset = 0;
                offset += swprintf_s(previewBuffer + offset, MAX_PREVIEW_SIZE - offset, L"Files in clipboard: %u\r\n", fileCount);
                for (UINT i = 0; i < fileCount && i < 100; i++) {
                    wchar_t filePath[MAX_PATH];
                    if (DragQueryFileW(hDrop, i, filePath, MAX_PATH)) {
                        offset += swprintf_s(previewBuffer + offset, MAX_PREVIEW_SIZE - offset, L"%2u: %s\r\n", i + 1, filePath);
                    }
                }
                GlobalUnlock(hData);
            }
            break;
        }
        default: {
            wchar_t formatName[256] = L"";
            if (GetClipboardFormatNameW(format, formatName, _countof(formatName)) > 0 &&
                wcscmp(formatName, L"HTML Format") == 0) {
                char* html = (char*)GlobalLock(hData);
                if (html) {
                    char* htmlStart = strstr(html, "<html");
                    if (!htmlStart) htmlStart = strstr(html, "<HTML");
                    if (!htmlStart) htmlStart = html;
                    MultiByteToWideChar(CP_UTF8, 0, htmlStart, -1, previewBuffer, MAX_PREVIEW_SIZE);
                    GlobalUnlock(hData);
                }
            } else {
                swprintf_s(previewBuffer, MAX_PREVIEW_SIZE, L"[Data present in %s]", GetFormatName(format));
            }
            break;
        }
    }
    SetWindowTextW(previewText, previewBuffer);
    CloseClipboard();
}

void RepositionControls(HWND hwnd) {
    RECT rc;
    GetClientRect(hwnd, &rc);
    int margin = 10;
    int clientWidth = rc.right - rc.left;
    int clientHeight = rc.bottom - rc.top;
    int leftWidth = (clientWidth - 3 * margin) / 2;
    int rightWidth = leftWidth;

    // Define group box sizes and positions
    // Group Actions: top left, fixed height = 140
    int actions_x = margin;
    int actions_y = margin;
    int actions_w = leftWidth;
    int actions_h = 140;

    // Clipboard Status: bottom left
    int status_x = margin;
    int status_y = actions_y + actions_h + margin;
    int status_w = leftWidth;
    int status_h = clientHeight - (actions_h + 3 * margin);

    // Process Information: top right, fixed height = 200
    int process_x = margin * 2 + leftWidth;
    int process_y = margin;
    int process_w = rightWidth;
    int process_h = 200;

    // Clipboard Preview: bottom right
    int preview_x = process_x;
    int preview_y = process_y + process_h + margin;
    int preview_w = rightWidth;
    int preview_h = clientHeight - (process_h + 3 * margin);

    // Reposition group boxes
    MoveWindow(groupActions, actions_x, actions_y, actions_w, actions_h, TRUE);
    MoveWindow(groupStatus, status_x, status_y, status_w, status_h, TRUE);
    MoveWindow(groupProcess, process_x, process_y, process_w, process_h, TRUE);
    MoveWindow(groupPreview, preview_x, preview_y, preview_w, preview_h, TRUE);

    int innerMargin = 10;

    // Reposition controls inside Group Actions
    // Row 1
    MoveWindow(checkButton, actions_x + innerMargin, actions_y + 20, 130, 30, TRUE);
    MoveWindow(unlockButton, actions_x + actions_w/2, actions_y + 20, 130, 30, TRUE);
    // Row 2
    MoveWindow(killButton, actions_x + innerMargin, actions_y + 60, 130, 30, TRUE);
    MoveWindow(copyPidButton, actions_x + actions_w/2, actions_y + 60, 130, 30, TRUE);
    // Row 3
    MoveWindow(clearClipboardButton, actions_x + innerMargin, actions_y + 100, 130, 30, TRUE);
    MoveWindow(autoRefreshCheck, actions_x + actions_w/2, actions_y + 100, 130, 30, TRUE);

    // Reposition statusText inside Clipboard Status group
    MoveWindow(statusText, status_x + innerMargin, status_y + 20, status_w - 2*innerMargin, status_h - 30, TRUE);

    // Reposition processList inside Process Information group
    MoveWindow(processList, process_x + innerMargin, process_y + 20, process_w - 2*innerMargin, process_h - 30, TRUE);

    // Reposition formatCombo and previewText inside Clipboard Preview group
    MoveWindow(formatCombo, preview_x + innerMargin, preview_y + 20, preview_w - 2*innerMargin, 25, TRUE);
    MoveWindow(previewText, preview_x + innerMargin, preview_y + 55, preview_w - 2*innerMargin, preview_h - 75, TRUE);

    {
        int pl_width = process_w - 2 * innerMargin;
        // Set column widths: 20% for PID, 30% for Process Name, 50% for Window Title
        ListView_SetColumnWidth(processList, 0, (int)(pl_width * 0.2));
        ListView_SetColumnWidth(processList, 1, (int)(pl_width * 0.3));
        ListView_SetColumnWidth(processList, 2, pl_width - (int)(pl_width * 0.2) - (int)(pl_width * 0.3));
    }
}

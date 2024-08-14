#ifndef UNICODE
#define UNICODE
#endif
#define STRICT

#include <windows.h>
#include <strsafe.h>
#include <assert.h>
#include <commctrl.h>
#include <process.h>

// https://learn.microsoft.com/en-us/windows/win32/controls/cookbook-overview
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#define ID_BUTTON_YES   ((HMENU)1)
#define ID_BUTTON_NO    ((HMENU)2)
#define ID_EDIT         ((HMENU)3)

#define ID_EDIT_SUBCLASS 0

enum CmdKind {
    CmdKindNone,
    CmdKindYes,
    CmdKindNo,
    CmdKindText,
} cmd;
HANDLE ghWriteEvent;
#define TEXT_LEN 32
wchar_t text[TEXT_LEN];

// https://learn.microsoft.com/en-us/windows/win32/debug/retrieving-the-last-error-code
void ErrorExit(void) {
    wchar_t msg[1024];
    int msgLen = sizeof msg / sizeof(wchar_t);
    DWORD lastError = GetLastError();

    HRESULT strRes = StringCchPrintf(msg, msgLen,
        L"Failed with error %d: ", lastError);
    assert(strRes == S_OK);

    // If the message length excedes the size of the buffer we truncate it,
    // because it would have been to long anyway.
    int partialStrLen = lstrlen(msg);
    DWORD fmtMsgSize = FormatMessage(
        FORMAT_MESSAGE_FROM_SYSTEM
        | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        lastError,
        LANG_USER_DEFAULT,
        msg + partialStrLen,
        msgLen - partialStrLen,
        NULL);
    if (!fmtMsgSize) {
        DWORD fmtMsglastError = GetLastError();
        assert(fmtMsglastError == ERROR_MORE_DATA);
    }
    msg[msgLen - 1] = 0;

    MessageBox(NULL, msg, TEXT("Error"), MB_OK | MB_ICONERROR);
    ExitProcess(lastError);
}

BOOL SetWriterEvent(enum CmdKind inputCmd) {
    cmd = inputCmd;
    return SetEvent(ghWriteEvent);
}

// https://stackoverflow.com/a/29041414
// https://learn.microsoft.com/en-us/windows/win32/controls/subclassing-overview
LRESULT CALLBACK EditSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    if (uMsg == WM_CHAR && wParam == VK_RETURN) {
        if (cmd != CmdKindNone) {
            MessageBeep(MB_ICONSTOP);
            return 0;
        }
        (void) GetWindowText(hWnd, text, TEXT_LEN);
        assert(GetLastError() == ERROR_SUCCESS);
        SetWriterEvent(CmdKindText);
        BOOL res = SetWindowText(hWnd, L"");
        assert(res);
        return 0;
    }
    LRESULT lRes = DefSubclassProc(hWnd, uMsg, wParam, lParam);
    if (uMsg == WM_DESTROY)
        RemoveWindowSubclass(hWnd, EditSubclassProc, ID_EDIT_SUBCLASS);
    return lRes;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE:
        HWND hwndButtonYes = CreateWindow(
            L"BUTTON", L"Yes",
            WS_VISIBLE | WS_CHILD, 
            10, 100,         // x,y 
            40, 20,         // width, height
            hwnd,
            ID_BUTTON_YES,
            (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
            NULL
        );
        HWND hwndButtonNo = CreateWindow(
            L"BUTTON", L"No",
            WS_VISIBLE | WS_CHILD,
            300, 100,         // x,y 
            40, 20,         // width, height
            hwnd,
            ID_BUTTON_NO,
            (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
            NULL
        );
        HWND hwndEdit = CreateWindow(
            L"EDIT", L"",
            WS_BORDER | WS_CHILD | WS_VISIBLE,
            10, 10, // x,y
            350, 20, // width,height
            hwnd,
            ID_EDIT,
            (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
            NULL
        );
        SendMessageW((hwndEdit), EM_LIMITTEXT, TEXT_LEN - 1, 0);
        SetWindowSubclass(hwndEdit, EditSubclassProc, ID_EDIT_SUBCLASS, 0);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_BUTTON_YES:
            if (cmd != CmdKindNone) {
                MessageBeep(MB_ICONSTOP);
                break;
            }
            SetWriterEvent(CmdKindYes);
            break;
        case ID_BUTTON_NO:
            if (cmd != CmdKindNone) {
                MessageBeep(MB_ICONSTOP);
                break;
            }
            SetWriterEvent(CmdKindNo);
            break;
        }
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void Writer(void *arg) {
    // The driver makes the serial communication work only if the USB is
    // connected to a particular port and assigns it to COM3.
    HANDLE hPort = CreateFileW(L"\\\\.\\COM3",
        GENERIC_READ | GENERIC_WRITE,
        0,                                  // the devide isn't shared.
        NULL,                               // the object gets a default security.
        OPEN_EXISTING,                      // Specify which action to take on file. 
        0,                                  // default.
        NULL);                              // default.
    if (hPort == INVALID_HANDLE_VALUE) {
        ErrorExit();
    }

    DCB config;
    if (!GetCommState(hPort, &config)) {
        ErrorExit();
    }
    config.BaudRate = 115200;
    config.Parity = 0;   // none
    config.ByteSize = 8;
    config.StopBits = 0; // one
    if (!SetCommState(hPort, &config)) {
        ErrorExit();
    }

    // https://www.lookrs232.com/com_port_programming/api_commtimeouts.htm
    COMMTIMEOUTS comTimeOut = {
        .ReadIntervalTimeout = 10,
        .ReadTotalTimeoutMultiplier = 3,
        .ReadTotalTimeoutConstant = 2,
        .WriteTotalTimeoutMultiplier = 3,
        .WriteTotalTimeoutConstant = 2,
    };
    if (!SetCommTimeouts(hPort, &comTimeOut)) {
        ErrorExit();
    }

    /* This sequence soft resets the device, put it at the default home, moves
     * it at the desired home and set the current position as the new home.
     */
    // CHAR initGcode[] = "\x18G0 X0 Y0\nG0 Y-70\nG92 X0 Y0\n";

    CHAR initGcode[] = "\x18G92 X0 Y0\n";
    CHAR yesGcode[] = "G0 X-40\nG4 P1\nG0 X0\n";
    CHAR noGcode[] = "G0 X50\nG4 P1\nG0 X0\n";
    CHAR molochGcode[] =
        "G0 X130 Y-40\nG4 P1"  // M
        "G0 X-100 Y-60\nG4 P1" // O
        "G0 X100 Y-40\nG4 P1"  // L
        "G0 X-100 Y-60\nG4 P1" // O
        "G0 X-80 Y-40\nG4 P1"  // C
        "G0 X30 Y-40\nG4 P1"   // H
        "G0 X0 Y0\n";
    // The allocate the necessary space to send a Gcode for each
    // letter plus returning back to home at the end.
    CHAR bufGcode[(sizeof "Gnn Xsnnn Ysnnn\nG4 Pn.n\n" - 1) * (TEXT_LEN - 1 + 1)] = {0};

    DWORD numberOfBytesWritten = 0;

    if (!WriteFile(hPort, initGcode, sizeof initGcode, &numberOfBytesWritten, NULL)) {
        ErrorExit();
    }
    Sleep(2 * 1000);

    // Ouija board
    // +-------------------------+
    // |       yes  *  no        |
    // |                         |
    // |a b c d e f g h i j k l m|
    // |n o p q r s t u v w x y z|
    // +-------------------------+
    int xAlphabet = -120, yAlphabet = -40; // A's location
    int xOffset = 20, yOffset = -20;

    while (1) {
        DWORD dwWaitResult = WaitForSingleObject(ghWriteEvent, INFINITE);

        switch (cmd) {
        case CmdKindYes:
            if (!WriteFile(hPort, yesGcode, sizeof yesGcode, &numberOfBytesWritten, NULL)) {
                ErrorExit();
            }
            Sleep(2 * 1000);
            break;
        case CmdKindNo:
            if (!WriteFile(hPort, noGcode, sizeof noGcode, &numberOfBytesWritten, NULL)) {
                ErrorExit();
            }
            Sleep(2 * 1000);
            break;
        case CmdKindText:
            WCHAR *ptr = text;
            WCHAR ch = 0;
            CHAR *destEnd = bufGcode;
            size_t remaining = sizeof bufGcode;
            int count = 1;

            while ((ch = *ptr++)) {
                // From https://en.wikipedia.org/wiki/UTF-16#U+0000_to_U+D7FF_and_U+E000_to_U+FFFF
                // The ranges for the high surrogates (0xD800–0xDBFF), low surrogates (0xDC00–0xDFFF),
                // and valid BMP characters (0x0000–0xD7FF, 0xE000–0xFFFF) are disjoint.
                if (ch >= 128 // 😃
                    || !(L'A' <= ch && ch <= L'Z'
                        || L'a' <= ch && ch <= L'z'
                        || ch == L' ')) {
                    continue;
                }
                count++;
                if (ch == L' ') {
                    HRESULT res = StringCchPrintfExA(destEnd, remaining, &destEnd, &remaining, STRSAFE_FILL_BEHIND_NULL,
                        "G0 X0 Y0\nG4 P0.5\n");
                    assert(res == S_OK);
                    continue;
                }
                // To lower
                if (ch <= L'Z') ch += L'a' - L'A';
                assert(L'a' <= ch && ch <= L'z');
                ch -= L'a';
                // An absolutelly unnecessary way to avoid division.
                int row = ch >= 13;           // == ch / 13
                int col = row ? ch - 13 : ch; // == ch % 13
                HRESULT res = StringCchPrintfExA(destEnd, remaining, &destEnd, &remaining, STRSAFE_FILL_BEHIND_NULL,
                    "G0 X%d Y%d\nG4 P1\n", xAlphabet + col * xOffset, yAlphabet + row * yOffset);
                assert(res == S_OK);
            }
            HRESULT res = StringCchPrintfExA(destEnd, remaining, &destEnd, &remaining, STRSAFE_FILL_BEHIND_NULL,
                "G0 X0 Y0\n");
            assert(res == S_OK);
            if (!WriteFile(hPort, bufGcode, (DWORD)(sizeof bufGcode - remaining), &numberOfBytesWritten, NULL)) {
                ErrorExit();
            }
            Sleep(count * 1000);
            break;
        default:
            assert(0);
            break;
        }

        cmd = CmdKindNone;
    }
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
    ghWriteEvent = CreateEvent(
        NULL,               // default security attributes
        FALSE,              // non manual-reset event
        FALSE,              // initial state is nonsignaled
        TEXT("WriteEvent")  // object name
    );
    if (!ghWriteEvent) {
        ErrorExit();
    }
    _beginthread(Writer, 0, NULL);

    WNDCLASS wc = {
       .lpfnWndProc = WindowProc,
       .hInstance = hInstance,
       .lpszClassName = L"Sample Window Class",
    };

    ATOM wcAtom = RegisterClass(&wc);
    if (!wcAtom) {
        ErrorExit();
    }

    HWND hwnd = CreateWindowEx(
        0,                     // Optional window styles.
        MAKEINTATOM(wcAtom),   // Window class
        L"Ouija Controller",    // Window text
        (WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME) ^ WS_MAXIMIZEBOX | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, // x,y
        400, 200,   // w,h
        NULL,       // Parent window    
        NULL,       // Menu
        hInstance,  // Instance handle
        NULL        // Additional application data
    );
    if (!hwnd) {
        ErrorExit();
    }

    MSG msg = {0};
    BOOL res = 0;
    while ((res = (GetMessage(&msg, NULL, 0, 0)) != 0))
    {
        if (res == -1) {
            // WTF do I do now?
        }
        (void) TranslateMessage(&msg);
        (void) DispatchMessage(&msg);
    }

    return 0;
}

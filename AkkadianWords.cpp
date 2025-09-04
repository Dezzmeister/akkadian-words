#include "common.h"
#include <time.h>
#include "dict.h"
#include "handlers.h"
#include "errors.h"
#include "resource.h"

#define MAX_LOADSTRING 100
#define DICT_FILENAME _T("dict.dat")

HINSTANCE h_inst;                                // current instance
WCHAR sz_title[MAX_LOADSTRING];                  // The title bar text
WCHAR sz_window_class[MAX_LOADSTRING];           // the main window class name
HBITMAP bg;
BITMAP bg_bmp;
HBRUSH bg_brush;

ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(
    _In_ HINSTANCE h_instance,
    _In_opt_ HINSTANCE h_prev_instance,
	_In_ LPWSTR lp_cmd_line,
	_In_ int n_cmd_show
) {
    UNREFERENCED_PARAMETER(h_prev_instance);
    UNREFERENCED_PARAMETER(lp_cmd_line);

    time_t t;
    srand((unsigned int)time(&t));

    try {
        Akk::dict = Dictionary(DICT_FILENAME);
    } catch (DictParseError err) {
        std::wstring msg = err.message();

        if (err.err_type == ParseErrorType::FileNotFound) {
            MessageBoxW(nullptr,
                L"File not found! Expected dict.dat in working directory",
                L"Fatal Error",
                MB_ICONERROR | MB_OK);
            return -1;
        }

        MessageBoxW(nullptr,
            msg.c_str(),
            L"Fatal Error",
            MB_ICONERROR | MB_OK);
        return -1;
    }

    LoadStringW(h_instance, IDS_APP_TITLE, sz_title, MAX_LOADSTRING);
    LoadStringW(h_instance, IDC_AKKADIAN_WORDS, sz_window_class, MAX_LOADSTRING);
    MyRegisterClass(h_instance);

    bg = LoadBitmap(h_instance, MAKEINTRESOURCE(IDB_KING_LOGO));
    int success = GetObject(bg, sizeof(BITMAP), &bg_bmp);

    if (bg == NULL || !success) {
        MessageBoxW(nullptr,
            L"Failed to load logo",
            L"Error",
            MB_ICONERROR | MB_OK
        );
    }

    if (!InitInstance (h_instance, n_cmd_show)) {
        return FALSE;
    }

    HACCEL h_accel_table = LoadAccelerators(h_instance, MAKEINTRESOURCE(IDC_AKKADIANPRACTICE));

    MSG msg;

    while (GetMessage(&msg, nullptr, 0, 0)) {
        if (!TranslateAccelerator(msg.hwnd, h_accel_table, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance) {
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_AKKADIAN_WORDS));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_AKKADIAN_WORDS);
    wcex.lpszClassName  = sz_window_class;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE h_instance, int n_cmd_show) {
   h_inst = h_instance;

   HWND hWnd = CreateWindowW(sz_window_class, sz_title, WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME,
      CW_USEDEFAULT, 0, 300, 300, nullptr, nullptr, h_instance, nullptr);

   if (!hWnd) {
      return FALSE;
   }

   ShowWindow(hWnd, n_cmd_show);
   UpdateWindow(hWnd);

   return TRUE;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM w_param, LPARAM l_param) {
    switch (message) {
		case WM_CREATE:
			if (bg == NULL) {
				break;
			}

			bg_brush = CreatePatternBrush(bg);

			break;
        case WM_COMMAND: {
            int wmId = LOWORD(w_param);
            // Parse the menu selections:
            switch (wmId) {
                case IDM_ABOUT:
                    DialogBox(h_inst, MAKEINTRESOURCE(IDD_ABOUTBOX), hwnd, About);
                    break;
                case IDM_EXIT:
                    DestroyWindow(hwnd);
                    break;
                case ID_PRACTICE_ENGLISH:
                    DialogBoxW(h_inst, MAKEINTRESOURCEW(IDD_PRACTICE), hwnd, PracticeEnglish);
                    break;
                case ID_PRACTICE_AKKADIAN:
                    DialogBoxW(h_inst, MAKEINTRESOURCEW(IDD_PRACTICE), hwnd, PracticeAkkadian);
                    break;
                case ID_LOOKUP_ENGLISH:
                    DialogBoxW(h_inst, MAKEINTRESOURCEW(IDD_LOOKUP), hwnd, LookupEnglish);
                    break;
                case ID_LOOKUP_AKKADIAN:
                    DialogBoxW(h_inst, MAKEINTRESOURCEW(IDD_LOOKUP), hwnd, LookupAkkadian);
                    break;
                default:
                    return DefWindowProc(hwnd, message, w_param, l_param);
            }
            break;
        }
        case WM_ERASEBKGND: {
            HDC hdc = (HDC)w_param;
            HDC hdcBitmap = CreateCompatibleDC(hdc);
            SelectObject(hdcBitmap, bg);

            RECT r;
            GetClientRect(hwnd, &r);
            StretchBlt(hdc, 0, 0, r.right - r.left, r.bottom - r.top, hdcBitmap, 0, 0, bg_bmp.bmWidth, bg_bmp.bmHeight, SRCCOPY);
            DeleteDC(hdcBitmap);

            return TRUE;
        }
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		default:
			return DefWindowProc(hwnd, message, w_param, l_param);
    }

    return 0;
}

INT_PTR CALLBACK About(HWND h_dlg, UINT message, WPARAM w_param, LPARAM l_param) {
    UNREFERENCED_PARAMETER(l_param);

    switch (message) {
		case WM_INITDIALOG:
			return (INT_PTR)TRUE;
		case WM_COMMAND:
			if (LOWORD(w_param) == IDOK || LOWORD(w_param) == IDCANCEL) {
				EndDialog(h_dlg, LOWORD(w_param));
				return (INT_PTR)TRUE;
			}
			break;
    }

    return (INT_PTR)FALSE;
}

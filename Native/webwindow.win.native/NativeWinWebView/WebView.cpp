#include <string>
#include <windows.h>
#include <wrl.h>
#include <wil/com.h>
#include <WebView2.h>
#include "WebView.h"
using namespace Microsoft::WRL;
using namespace std;

static wil::com_ptr<IWebView2WebView5> webviewWindow;
bool window_settings_enabled{ false };
bool is_fullscreen{ false };
wstring organization;
wstring application;
wstring reg_key;
const wchar_t* X = L"x";
const wchar_t* Y = L"y";
const wchar_t* IS_MAXIMIZED = L"IsMaximized";
const wchar_t* IS_MINIMIZED = L"isMinimized";
const wchar_t* WIDTH = L"width";
const wchar_t* HEIGHT = L"height";
callback_ptr callback{ nullptr };

struct Window_settings {
    int x{ CW_USEDEFAULT };
    int y{ CW_USEDEFAULT };
    int width{ CW_USEDEFAULT };
    int height{ CW_USEDEFAULT };
    bool is_maximized{ false };
    bool isMinimized{ false };
};

void save_window_settings(HWND hWnd) {
    if (window_settings_enabled && !is_fullscreen) {
        HKEY key;
        DWORD disposition{ 0 };
        RegCreateKeyEx(HKEY_CURRENT_USER, reg_key.c_str(), 0, nullptr, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, nullptr, &key, &disposition);
        if (!IsZoomed(hWnd) && !IsIconic(hWnd)) {
            RECT rect{ 0 };
            GetWindowRect(hWnd, &rect);
            RegSetValueEx(key, X, 0, REG_DWORD, (BYTE*)&rect.left, 4);
            RegSetValueEx(key, Y, 0, REG_DWORD, (BYTE*)&rect.top, 4);
            int width = rect.right - rect.left;
            int height = rect.bottom - rect.top;
            RegSetValueEx(key, WIDTH, 0, REG_DWORD, (BYTE*)&width, 4);
            RegSetValueEx(key, HEIGHT, 0, REG_DWORD, (BYTE*)&height, 4);
            DWORD v{ 0 };
            RegSetValueEx(key, IS_MAXIMIZED, 0, REG_DWORD, (BYTE*)&v, 4);
            RegSetValueEx(key, IS_MINIMIZED, 0, REG_DWORD, (BYTE*)&v, 4);
        }
        if (IsZoomed(hWnd)) {
            DWORD v{ 1 };
            RegSetValueEx(key, IS_MAXIMIZED, 0, REG_DWORD, (BYTE*)&v, 4);
            v = 0;
            RegSetValueEx(key, IS_MINIMIZED, 0, REG_DWORD, (BYTE*)&v, 4);
        }
        if (IsIconic(hWnd)) {
            DWORD v{ 1 };
            RegSetValueEx(key, IS_MINIMIZED, 0, REG_DWORD, (BYTE*)&v, 4);
            v = 0;
            RegSetValueEx(key, IS_MAXIMIZED, 0, REG_DWORD, (BYTE*)&v, 4);
        }

        RegCloseKey(key);
    }
}

Window_settings get_window_settings() {
    Window_settings ws;
    if (window_settings_enabled) {
        HKEY key;
        DWORD disposition{ 0 };
        RegCreateKeyEx(HKEY_CURRENT_USER, reg_key.c_str(), 0, nullptr, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, nullptr, &key, &disposition);
        DWORD s{ 4 };
        DWORD t{ REG_DWORD };
        auto ret = RegQueryValueEx(key, X, 0, &t, (BYTE*)&ws.x, &s);
        if (ret == 0) {
            RegQueryValueEx(key, Y, 0, &t, (BYTE*)&ws.y, &s);
            RegQueryValueEx(key, WIDTH, 0, &t,(BYTE*)&ws.width, &s);
            RegQueryValueEx(key, HEIGHT, 0, &t, (BYTE*)&ws.height, &s);
            DWORD v;
            RegQueryValueEx(key, IS_MAXIMIZED, 0, &t, (BYTE*)&v, &s);
            ws.is_maximized = v == 1;
            RegQueryValueEx(key, IS_MINIMIZED, 0, &t, (BYTE*)&v, &s);
            ws.isMinimized = v == 1;
        }
        RegCloseKey(key);
    }
    return ws;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_SIZE:
        if (webviewWindow != nullptr) {
            RECT bounds;
            GetClientRect(hWnd, &bounds);
            webviewWindow->put_Bounds(bounds);
        };
        break;
    case WM_SYSCOMMAND:
        switch (wParam)
        {
        case SC_MINIMIZE:
            save_window_settings(hWnd);
            break;
        case SC_MAXIMIZE:
            save_window_settings(hWnd);
            break;
        }        
        return DefWindowProc(hWnd, message, wParam, lParam);
    case WM_DESTROY:
        save_window_settings(hWnd);
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

auto load_icon(const wchar_t* icon_path) {
    return (HICON)LoadImage( // returns a HANDLE so we have to cast to HICON
        nullptr,             
        icon_path,   
        IMAGE_ICON,  
        0,           
        0,                
        LR_LOADFROMFILE | 
        LR_DEFAULTSIZE |    
        LR_SHARED         // let the system release the handle when it's no longer used
    );
}

HMENU menu{ nullptr };
WINDOWPLACEMENT previous_placement{ 0 };

void enter_fullscreen(HWND hWnd) {
    is_fullscreen = true;
    DWORD style = GetWindowLong(hWnd, GWL_STYLE);
    MONITORINFO monitor_info = { sizeof(monitor_info) };
    menu = GetMenu(hWnd);
    SetMenu(hWnd, nullptr);
    if (GetWindowPlacement(hWnd, &previous_placement) &&
        GetMonitorInfo(MonitorFromWindow(hWnd, MONITOR_DEFAULTTOPRIMARY), &monitor_info))
    {
        SetWindowLong(hWnd, GWL_STYLE, style & ~WS_OVERLAPPEDWINDOW);
        SetWindowPos(hWnd, HWND_TOP, monitor_info.rcMonitor.left, monitor_info.rcMonitor.top,
            monitor_info.rcMonitor.right - monitor_info.rcMonitor.left,
            monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top,
            SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
}

void exit_fullscreen(HWND hWnd) {
    is_fullscreen = false;
    DWORD style = GetWindowLong(hWnd, GWL_STYLE);
    SetMenu(hWnd, menu);
    SetWindowLong(hWnd, GWL_STYLE, style | WS_OVERLAPPEDWINDOW);
    SetWindowPlacement(hWnd, &previous_placement);
    SetWindowPos(hWnd, NULL, 0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
}

void create_window(Configuration configuration) {
    window_settings_enabled = configuration.save_window_settings;
    organization = window_settings_enabled ? configuration.organization : L""s; 
    application = window_settings_enabled ? configuration.application : L""s;
    callback = configuration.callback;
    reg_key = LR"(Software\)"s + organization + LR"(\)"s + application;
    auto settings = get_window_settings();
    auto instance = LoadLibrary(L"NativeWinWebView");
    auto window_class = L"NativeWebViewClass";
    WNDCLASSEXW wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = instance;
    wcex.hIcon = load_icon(configuration.icon_path);
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = window_class;
    wcex.hIconSm = nullptr;
    auto atom = RegisterClassExW(&wcex);

    auto hWnd = CreateWindowW(window_class, configuration.title, WS_OVERLAPPEDWINDOW,
        settings.x, settings.y, settings.width, settings.height, nullptr, nullptr, instance, nullptr);

    if (!hWnd)
        return;

    ShowWindow(hWnd, 
        settings.is_maximized 
        ? SW_SHOWMAXIMIZED 
        : settings.isMinimized
            ? SW_SHOWMINIMIZED
            : SW_SHOWDEFAULT);
    UpdateWindow(hWnd);
    auto url = wstring(configuration.url);
    auto dev_tools_enabled = configuration.debugging_enabled;
    auto fullscreen_enabled = configuration.full_screen_enabled;

    CreateWebView2EnvironmentWithDetails(nullptr, nullptr, nullptr, 
        Callback<IWebView2CreateWebView2EnvironmentCompletedHandler>([hWnd, url, dev_tools_enabled, fullscreen_enabled](HRESULT result, IWebView2Environment* env) -> HRESULT {
            env->CreateWebView(hWnd, Callback<IWebView2CreateWebViewCompletedHandler>(
                    [hWnd, url, dev_tools_enabled, fullscreen_enabled](HRESULT result, IWebView2WebView* webview) -> HRESULT {
                if (webview != nullptr)
                    webview->QueryInterface(IID_PPV_ARGS(&webviewWindow));

                IWebView2Settings* Settings;
                webviewWindow->get_Settings(&Settings);
                Settings->put_IsScriptEnabled(TRUE);
                Settings->put_AreDefaultScriptDialogsEnabled(TRUE);
                Settings->put_IsWebMessageEnabled(TRUE);
                Settings->put_AreDevToolsEnabled(dev_tools_enabled);
                // Resize WebView to fit the bounds of the parent window
                RECT bounds;
                GetClientRect(hWnd, &bounds);
                webviewWindow->put_Bounds(bounds);

                // Schedule an async task to navigate to Bing
                webviewWindow->Navigate(url.c_str());

                if (fullscreen_enabled) {
                    (webviewWindow->add_ContainsFullScreenElementChanged(Callback<IWebView2ContainsFullScreenElementChangedEventHandler>(
                        [hWnd](IWebView2WebView5* sender, IUnknown* args) -> HRESULT {
                            BOOL contains_fullscreen{ FALSE };
                            sender->get_ContainsFullScreenElement(&contains_fullscreen);
                            if (contains_fullscreen)
                                enter_fullscreen(hWnd);
                            else
                                exit_fullscreen(hWnd);
                            return S_OK;
                        })
                    .Get(),
                    nullptr));
                }

                // Step 6 - Communication between host and web content
                // Set an event handler for the host to return received message back to the web content
                if (callback) {
                    EventRegistrationToken token;
                    webviewWindow->add_WebMessageReceived(Callback<IWebView2WebMessageReceivedEventHandler>(
                        [](auto webview, auto args) -> HRESULT {
                            PWSTR message;
                            args->get_WebMessageAsString(&message);
                            callback(message);
                            CoTaskMemFree(message);
                            return S_OK;
                        }).Get(), &token);
                }

                //// Schedule an async task to add initialization script that
                //// 1) Add an listener to print message from the host
                //// 2) Post document URL to the host
                webviewWindow->AddScriptToExecuteOnDocumentCreated(
                	L"window.chrome.webview.addEventListener(\'message\', event => alert(event.data));" \
                        L"var webWindowNetCore = window.chrome.webview;", nullptr);
                    return S_OK;
                }).Get());
            return S_OK;
        }).Get());
}

void send_to_browser(const wchar_t* text) {
    webviewWindow->PostWebMessageAsString(text);
}
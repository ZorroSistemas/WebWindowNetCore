#include <windows.h>
#include "WebView.h"

void initialize_window(Configuration configuration) {
    create_window(configuration);
}

int execute() {
    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}




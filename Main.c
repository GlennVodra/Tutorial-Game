#include <stdio.h>
#include <Windows.h>
#include <stdint.h>

#include "Main.h"


BOOL gMainGameIsRunning;
HWND  gGameWindow;
MONITORINFO gMonitorInfo = {sizeof(MONITORINFO)};

GAMEBITMAP gBackBuffer;

int WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, PSTR CmdLine, INT CmdShow){



    if(GameIsAlreadyRunning() == TRUE){
        MessageBoxA(NULL, "Application is already running!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        goto Exit;
    }

    if(CreateMainGameWindow() != ERROR_SUCCESS){
        goto Exit;
    }

    gBackBuffer.BitmapInfo.bmiHeader.biSize = sizeof(gBackBuffer.BitmapInfo.bmiHeader);
    gBackBuffer.BitmapInfo.bmiHeader.biWidth = GAME_RES_HEIGHT;
    gBackBuffer.BitmapInfo.bmiHeader.biHeight = GAME_RES_WIDTH;
    gBackBuffer.BitmapInfo.bmiHeader.biBitCount = GAME_BPP;
    gBackBuffer.BitmapInfo.bmiHeader.biCompression = BI_RGB;
    gBackBuffer.BitmapInfo.bmiHeader.biPlanes = 1;
    if((gBackBuffer.Memory = VirtualAlloc(NULL, GAME_DRAWING_AREA_MEMORY_SIZE, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE)) == NULL){
        MessageBoxA(NULL, "Memory Allocation for Drawing Surface Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        goto Exit;
    }
    memset(gBackBuffer.Memory, 0xFF, GAME_DRAWING_AREA_MEMORY_SIZE);

    MSG Message = {0};
    gMainGameIsRunning = TRUE;

    while(gMainGameIsRunning){
        while (PeekMessageA(&Message, gGameWindow, 0, 0, PM_REMOVE)){
            DispatchMessageA(&Message);
        }
        ProcessPlayerInput();
        RenderFrameGraphics();
        Sleep(1);
    }

Exit:
    return 0;
}

LRESULT CALLBACK MainWindowProc(
        _In_ HWND WindowHandle,        // handle to window
        _In_ UINT Message,        // message identifier
        _In_ WPARAM wParam,    // first message parameter
        _In_ LPARAM lParam)    // second message parameter
{
    LRESULT Result = 0;

    switch (Message)
    {
        case WM_CLOSE:{
            gMainGameIsRunning = FALSE;
            PostQuitMessage(0);
            break;
        }
        default:
            Result = DefWindowProcA(WindowHandle, Message, wParam, lParam);
    }
    return Result;
}

DWORD CreateMainGameWindow(void){
    DWORD Result = ERROR_SUCCESS;
    WNDCLASSEXA WindowClass = { 0 };
    WindowClass.cbSize = sizeof(WNDCLASSEXA);
    WindowClass.style = 0;
    WindowClass.lpfnWndProc = MainWindowProc;
    WindowClass.cbClsExtra = 0;
    WindowClass.cbWndExtra = 0;
    WindowClass.hInstance = GetModuleHandleA(NULL);
    WindowClass.hIcon = LoadIconA(NULL, IDI_APPLICATION);
    WindowClass.hCursor = LoadCursorA(NULL, IDC_ARROW);
    WindowClass.hbrBackground = CreateSolidBrush(RGB(255,0,255));
    WindowClass.lpszClassName = GAME_NAME "_WindowClass";
    WindowClass.lpszMenuName = NULL;
    WindowClass.hIconSm = LoadIconA(NULL, IDI_APPLICATION);

    if(!RegisterClassExA(&WindowClass)){
        Result = GetLastError();
        MessageBoxA(NULL, "Window Registration Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        goto Exit;
    }

    gGameWindow = CreateWindowExA(
            WS_EX_CLIENTEDGE,
            WindowClass.lpszClassName,
            "Window title",
            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            680,
            480,
            NULL,
            NULL,
            GetModuleHandleA(NULL),
            NULL
    );

    if(gGameWindow == NULL){
        Result = GetLastError();
        MessageBoxA(NULL, "Window Registration Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        goto Exit;
    }

    if(GetMonitorInfoA(MonitorFromWindow(gGameWindow, MONITOR_DEFAULTTOPRIMARY), &gMonitorInfo) == 0){
        Result = 0x80261001;
        goto Exit;
    }

    int MonitorWidth = gMonitorInfo.rcMonitor.right - gMonitorInfo.rcWork.left;
    int MonitorHeight = gMonitorInfo.rcMonitor.bottom - gMonitorInfo.rcMonitor.top;
    uint16_t windowDPI = GetDpiForWindow(GetModuleHandleA(NULL));
    printf("%i", MonitorWidth);
    printf("\n");
    printf("%i", MonitorHeight);



    Exit:
    return (Result);
}

BOOL GameIsAlreadyRunning(){

    CreateMutexA(NULL, FALSE, GAME_NAME "_Mutex");

    if(GetLastError() == ERROR_ALREADY_EXISTS){
        return(TRUE);
    }
    else{
        return(FALSE);
    }
}

void ProcessPlayerInput(){
    int16_t EscapeKeyIsDown = GetAsyncKeyState(VK_ESCAPE);
    if(EscapeKeyIsDown){
        SendMessageA(gGameWindow, WM_CLOSE, 0, 0);
    }
}

void RenderFrameGraphics(){
    HDC DeviceContext = GetDC(gGameWindow);

    StretchDIBits(DeviceContext, 0, 0, 100, 100, 0, 0, 100, 100, gBackBuffer.Memory, &gBackBuffer.BitmapInfo, DIB_RGB_COLORS, SRCCOPY);

    ReleaseDC(gGameWindow, DeviceContext);
}
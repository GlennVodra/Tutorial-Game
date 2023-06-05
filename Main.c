#include <stdio.h>
#pragma warning( push )
#pragma warning( disable : 4668 )
#include <Windows.h>
#include <emmintrin.h>
#include <psapi.h>
#pragma comment(lib, "Winmm.lib")
#pragma warning( pop )
#include <stdint.h>
#include "Main.h"

BOOL gGameIsRunning;
HWND gGameWindow;

GAMEBITMAP gBackBuffer;
GAMEPERFDATA gPerformanceData;

PLAYER gPlayer;

BOOL gWindowHasFocus;

int WINAPI WinMain(_In_ HINSTANCE Instance, _In_opt_ HINSTANCE PreviousInstance, _In_ PSTR CommandLine, _In_ int CommandShow){
	
    UNREFERENCED_PARAMETER(Instance);
	UNREFERENCED_PARAMETER(PreviousInstance);
	UNREFERENCED_PARAMETER(CommandLine);
	UNREFERENCED_PARAMETER(CommandShow);

    MSG Message = { 0 };

    int64_t FrameStart = 0;
    int64_t FrameEnd = 0;
    int64_t ElapsedMicroseconds = 0;
    int64_t ElapsedMicrosecondsAccumulatorRaw = 0;
    int64_t ElapsedMicrosecondsAccumulatorCooked = 0;

    HMODULE NtDllModuleHandle = NULL;

    FILETIME ProcessCreationTime = { 0 };
    FILETIME ProcessExitTime = { 0 };

    int64_t CurrentUserCPUTime = 0;
    int64_t CurrentKernelCPUTime = 0;
    int64_t PreviousUserCPUTime = 0;
    int64_t PreviousKernelCPUTime = 0;

    HANDLE ProcessHandle = GetCurrentProcess();

    if ((NtDllModuleHandle = GetModuleHandleA("ntdll.dll")) == NULL){
        MessageBoxA(NULL, "Couldn't load ntdll.dll!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        goto Exit;
    }

    #pragma warning( disable : 4191 ) //Function cast unsafe
    if ((NtQueryTimerResolution = (_NtQueryTimerResolution)GetProcAddress(NtDllModuleHandle, "NtQueryTimerResolution")) == NULL){
        MessageBoxA(NULL, "Couldn't find the NtQueryTimerResolution function in ntdll.dll!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        goto Exit;
    }
    #pragma warning( default : 4191 )

    NtQueryTimerResolution(&gPerformanceData.MinimumTimerResolution, &gPerformanceData.MaximumTimerResolution, &gPerformanceData.CurrentTimerResolution);

    GetSystemInfo(&gPerformanceData.SystemInfo);
    GetSystemTimeAsFileTime((FILETIME*)&gPerformanceData.PreviousSystemTime);

    if (GameIsAlreadyRunning() == TRUE){
        MessageBoxA(NULL, "Another instance of this game is already running!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        goto Exit;
    }

    if (timeBeginPeriod(1) == TIMERR_NOCANDO){
        MessageBoxA(NULL, "Failed to set global timer resolution!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        goto Exit;
    }

    if (SetPriorityClass(ProcessHandle, HIGH_PRIORITY_CLASS) == 0){
        MessageBoxA(NULL, "Failed to set processor priority", "Error!", MB_ICONEXCLAMATION | MB_OK);
        goto Exit;
    }

    if (SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST) == 0){
        MessageBoxA(NULL, "Failed to set thread priority", "Error!", MB_ICONEXCLAMATION | MB_OK);
        goto Exit;
    }
  
    if (CreateMainGameWindow() != ERROR_SUCCESS){
        goto Exit;
    }

    QueryPerformanceFrequency((LARGE_INTEGER*) &gPerformanceData.PerfFrequency);

    gBackBuffer.BitmapInfo.bmiHeader.biSize = sizeof(gBackBuffer.BitmapInfo.bmiHeader);
    gBackBuffer.BitmapInfo.bmiHeader.biWidth = GAME_RES_WIDTH;
    gBackBuffer.BitmapInfo.bmiHeader.biHeight = GAME_RES_HEIGHT;
    gBackBuffer.BitmapInfo.bmiHeader.biBitCount = GAME_BPP;
    gBackBuffer.BitmapInfo.bmiHeader.biCompression = BI_RGB;
    gBackBuffer.BitmapInfo.bmiHeader.biPlanes = 1;

    gBackBuffer.Memory = VirtualAlloc(NULL, GAME_DRAWING_AREA_MEMORY_SIZE, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

    if (gBackBuffer.Memory == NULL) {
        MessageBoxA(NULL, "Failed to allocate memory for drawing durface!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        goto Exit;
    }

    memset(gBackBuffer.Memory, 0x7F, GAME_DRAWING_AREA_MEMORY_SIZE);

    gPlayer.ScreenPosX = 25;
    gPlayer.ScreenPosY = 25;

    //--Main Game Loop--
    gGameIsRunning = TRUE;

    while (gGameIsRunning == TRUE){
        QueryPerformanceCounter((LARGE_INTEGER*) &FrameStart);
        while (PeekMessageA(&Message, gGameWindow, 0, 0, PM_REMOVE)){
            DispatchMessageA(&Message);
        }
        
        ProcessPlayerInput();
        RenderFrameGraphics();

        QueryPerformanceCounter((LARGE_INTEGER*) &FrameEnd);
        ElapsedMicroseconds = FrameEnd - FrameStart;
        ElapsedMicroseconds *= 1000000;
        ElapsedMicroseconds /= gPerformanceData.PerfFrequency;

        gPerformanceData.TotalFramesRendered++;
        ElapsedMicrosecondsAccumulatorRaw += ElapsedMicroseconds;

        while (ElapsedMicroseconds < TARGET_MICROSECONDS_PER_FRAME){
        
            ElapsedMicroseconds = FrameEnd - FrameStart;
            ElapsedMicroseconds *= 1000000;
            ElapsedMicroseconds /= gPerformanceData.PerfFrequency;
        
            QueryPerformanceCounter((LARGE_INTEGER*)&FrameEnd);
        
            if (ElapsedMicroseconds < (TARGET_MICROSECONDS_PER_FRAME - ((gPerformanceData.CurrentTimerResolution * 0.1f)) * 3.6)){
                Sleep(1);
            }
        }
        ElapsedMicrosecondsAccumulatorCooked += ElapsedMicroseconds;

        if (gPerformanceData.TotalFramesRendered % CALCULATE_AVERGAE_FPS_EVERY_X_FRAMES == 0) {
            
            GetSystemTimeAsFileTime((FILETIME*)&gPerformanceData.CurrentSystemTime);
            GetProcessTimes(ProcessHandle, &ProcessCreationTime, &ProcessExitTime, (FILETIME*)&CurrentKernelCPUTime, (FILETIME*)&CurrentUserCPUTime);
            
            gPerformanceData.CPUPercent = (CurrentKernelCPUTime - PreviousKernelCPUTime) + \
                (CurrentUserCPUTime - PreviousUserCPUTime);
            gPerformanceData.CPUPercent /= (gPerformanceData.CurrentSystemTime - gPerformanceData.PreviousSystemTime);
            gPerformanceData.CPUPercent /= (gPerformanceData.SystemInfo.dwNumberOfProcessors);
            gPerformanceData.CPUPercent *= 100;

            GetProcessHandleCount(ProcessHandle, &gPerformanceData.HandleCount);

            K32GetProcessMemoryInfo(ProcessHandle, (PROCESS_MEMORY_COUNTERS*) & gPerformanceData.MemInfo, sizeof(gPerformanceData.MemInfo));

            gPerformanceData.RawFPSAverage = 1.0f / ((ElapsedMicrosecondsAccumulatorRaw / CALCULATE_AVERGAE_FPS_EVERY_X_FRAMES) * 0.000001f);
            gPerformanceData.CookedFPSAverage = 1.0f / ((ElapsedMicrosecondsAccumulatorCooked / CALCULATE_AVERGAE_FPS_EVERY_X_FRAMES) * 0.000001f);

            ElapsedMicrosecondsAccumulatorRaw = 0;
            ElapsedMicrosecondsAccumulatorCooked = 0;

            PreviousKernelCPUTime = CurrentKernelCPUTime;
            PreviousUserCPUTime   = CurrentUserCPUTime;
            gPerformanceData.PreviousSystemTime    = gPerformanceData.CurrentSystemTime;
        }

    }

Exit:
	return 0;
}

LRESULT CALLBACK MainWindowProc(_In_ HWND WindowHandle, _In_ UINT Message, _In_ WPARAM wParam, _In_ LPARAM lParam){

    LRESULT Result = 0;

    switch (Message){
        case WM_CLOSE:{
            gGameIsRunning = FALSE;
            PostQuitMessage(0);
            break;
        }
        case WM_ACTIVATE: {
            if (wParam == 0) {
                gWindowHasFocus = FALSE;
            }
            else {
                gWindowHasFocus = TRUE;
                ShowCursor(FALSE);
            }
            
        }
        default:
            Result = DefWindowProcA(WindowHandle, Message, wParam, lParam);            
        }
    return Result;
}


DWORD CreateMainGameWindow(void) {
    DWORD Result = ERROR_SUCCESS;

    WNDCLASSEXA WindowClass = { 0 };

    WindowClass.cbSize = sizeof(WNDCLASSEXA);
    WindowClass.style = 0;
    WindowClass.lpfnWndProc = MainWindowProc;
    WindowClass.cbClsExtra = 0;
    WindowClass.cbWndExtra = 0;
    WindowClass.hInstance = GetModuleHandleA(NULL);
    WindowClass.hIcon = LoadIconA(NULL, IDI_APPLICATION);
    WindowClass.hIconSm = LoadIconA(NULL, IDI_APPLICATION);
    WindowClass.hCursor = LoadCursorA(NULL, IDC_ARROW);
    WindowClass.hbrBackground = CreateSolidBrush(RGB(255, 0, 255));
    WindowClass.lpszClassName = GAME_NAME "_WINDOWCLASS";
    WindowClass.lpszMenuName = NULL;



    if (RegisterClassExA(&WindowClass) == 0) {
        Result = GetLastError();
        MessageBoxA(NULL, "Window Registration Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        goto Exit;
    }

    gGameWindow = CreateWindowExA(
        0,
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

    if (gGameWindow == NULL) {
        Result = GetLastError();
        MessageBoxA(NULL, "Window Creation Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        goto Exit;
    }

    gPerformanceData.MonitorInfo.cbSize = sizeof(MONITORINFO);

    if (GetMonitorInfoA(MonitorFromWindow(gGameWindow, MONITOR_DEFAULTTOPRIMARY), &gPerformanceData.MonitorInfo) == FALSE) {
        Result = ERROR_MONITOR_NO_DESCRIPTOR;
        goto Exit;
    }

    gPerformanceData.MonitorWidth = gPerformanceData.MonitorInfo.rcMonitor.right - gPerformanceData.MonitorInfo.rcMonitor.left;
    gPerformanceData.MonitorHeight = gPerformanceData.MonitorInfo.rcMonitor.bottom - gPerformanceData.MonitorInfo.rcMonitor.top;

    if (SetWindowLongPtrA(gGameWindow, GWL_STYLE, (WS_OVERLAPPED | WS_VISIBLE) & ~WS_OVERLAPPEDWINDOW) == 0) {
        Result = ERROR_MONITOR_NO_DESCRIPTOR;
        goto Exit;
    }
    if (SetWindowPos(gGameWindow, HWND_TOP, gPerformanceData.MonitorInfo.rcMonitor.left, 
        gPerformanceData.MonitorInfo.rcMonitor.top, gPerformanceData.MonitorWidth, gPerformanceData.MonitorHeight, 
        SWP_NOOWNERZORDER | SWP_FRAMECHANGED) == 0) {

        Result = ERROR_MONITOR_NO_DESCRIPTOR;
        goto Exit;
    }

Exit:
    return Result;

}

BOOL GameIsAlreadyRunning(void) {

    HANDLE Mutex = NULL;
    Mutex = CreateMutexA(NULL, FALSE, GAME_NAME "_GameMutex");

    if (GetLastError() == ERROR_ALREADY_EXISTS){
        return(TRUE);
    }
    else{
        return(FALSE);
    }
}

void ProcessPlayerInput(void){
    //Game
    if (gWindowHasFocus == FALSE) {
        return;
    }

    int16_t EscapeKeyIsDown = GetAsyncKeyState(VK_ESCAPE);
    int16_t DebugKeyIsDown = GetAsyncKeyState(VK_F3);
    static int16_t DebugKeyWasDown;

    if (EscapeKeyIsDown) {
        SendMessageA(gGameWindow, WM_CLOSE, 0, 0);
    }

    if (DebugKeyIsDown && !DebugKeyWasDown) {
        gPerformanceData.DisplayDebugInfo = !gPerformanceData.DisplayDebugInfo;
    }
    
    DebugKeyWasDown = DebugKeyIsDown;


    //Movement
    int16_t LeftKeyIsDown = GetAsyncKeyState('A') | GetAsyncKeyState(VK_LEFT);
    static int16_t LeftKeyWasDown;
    int16_t RightKeyIsDown = GetAsyncKeyState('D') | GetAsyncKeyState(VK_RIGHT);
    static int16_t RightKeyWasDown;
    int16_t UpKeyIsDown = GetAsyncKeyState('W') | GetAsyncKeyState(VK_UP);
    static int16_t UpKeyWasDown;
    int16_t DownKeyIsDown = GetAsyncKeyState('S') | GetAsyncKeyState(VK_DOWN);
    static int16_t DownKeyWasDown;
    
    if (LeftKeyIsDown) {
        if (gPlayer.ScreenPosX > 0) {
            gPlayer.ScreenPosX--;
        }
    }
    if (RightKeyIsDown) {
        if (gPlayer.ScreenPosX < GAME_RES_WIDTH - 16) {
            gPlayer.ScreenPosX++;
        }
    }
    if (UpKeyIsDown) {
        if (gPlayer.ScreenPosY > 0) {
            gPlayer.ScreenPosY--;
        }
    }
    if (DownKeyIsDown) {
        if (gPlayer.ScreenPosY < GAME_RES_HEIGHT - 16) {
            gPlayer.ScreenPosY++;
        }
    }

    LeftKeyWasDown = LeftKeyIsDown;
    RightKeyWasDown = RightKeyIsDown;
    UpKeyWasDown = UpKeyIsDown;
    DownKeyWasDown = DownKeyIsDown;
}

void RenderFrameGraphics(void) {

    __m128i QuadPixel = {0x7f, 0x00, 0x00, 0xff, 0x7f, 0x00, 0x00, 0xff, 0x7f, 0x00, 0x00, 0xff, 0x7f, 0x00, 0x00, 0xff};
#ifdef SIMD
    ClearScreen(&QuadPixel);
#else
    PIXEL32 Pixel = { 0x7f, 0x00, 0x00, 0xff };
    ClearScreen(&Pixel);
#endif

    int32_t ScreenX = gPlayer.ScreenPosX;
    int32_t ScreenY = gPlayer.ScreenPosY;
    int32_t StartingScreenPixel = ((GAME_RES_WIDTH * GAME_RES_HEIGHT) - GAME_RES_WIDTH) - \
        (GAME_RES_WIDTH * ScreenY) + ScreenX;

    for (int32_t y = 0; y < 16; y++) {
        for (int32_t x = 0; x < 16; x++) {
            memset((PIXEL32*)gBackBuffer.Memory + StartingScreenPixel + x - (GAME_RES_WIDTH * y), 0xFF, sizeof(PIXEL32));
        }
    }

    HDC DeviceContext = GetDC(gGameWindow);
    
    SelectObject(DeviceContext, (HFONT)GetStockObject(ANSI_FIXED_FONT));

    StretchDIBits(DeviceContext, 0, 0, gPerformanceData.MonitorWidth, gPerformanceData.MonitorHeight, 0, 0, GAME_RES_WIDTH, GAME_RES_HEIGHT, gBackBuffer.Memory, &gBackBuffer.BitmapInfo, DIB_RGB_COLORS, SRCCOPY);
    
    if (gPerformanceData.DisplayDebugInfo == TRUE) {
        char DebugTextBuffer[64] = { 0 };
        sprintf_s(DebugTextBuffer, _countof(DebugTextBuffer), "FPS Raw:    %.01f", gPerformanceData.RawFPSAverage);
            TextOutA(DeviceContext, 0, 0, DebugTextBuffer, (int)strlen(DebugTextBuffer));
        sprintf_s(DebugTextBuffer, _countof(DebugTextBuffer), "FPS Cooked: %.01f", gPerformanceData.CookedFPSAverage);
            TextOutA(DeviceContext, 0, 13, DebugTextBuffer, (int)strlen(DebugTextBuffer));
        sprintf_s(DebugTextBuffer, _countof(DebugTextBuffer), "Min. Timer Res: %.02f", gPerformanceData.MinimumTimerResolution/ 10000.0f);
            TextOutA(DeviceContext, 0, 26, DebugTextBuffer, (int)strlen(DebugTextBuffer));
        sprintf_s(DebugTextBuffer, _countof(DebugTextBuffer), "Max  Timer Res: %.02f", gPerformanceData.MaximumTimerResolution/ 10000.0f);
            TextOutA(DeviceContext, 0, 39, DebugTextBuffer, (int)strlen(DebugTextBuffer));
        sprintf_s(DebugTextBuffer, _countof(DebugTextBuffer), "Cur Timer Res: %.02f", gPerformanceData.CurrentTimerResolution/ 10000.0f);
            TextOutA(DeviceContext, 0, 52, DebugTextBuffer, (int)strlen(DebugTextBuffer));
        sprintf_s(DebugTextBuffer, _countof(DebugTextBuffer), "Handles  : %lu", gPerformanceData.HandleCount);
            TextOutA(DeviceContext, 0, 65, DebugTextBuffer, (int)strlen(DebugTextBuffer));
        sprintf_s(DebugTextBuffer, _countof(DebugTextBuffer), "Memory   : %llu KB", gPerformanceData.MemInfo.PrivateUsage / 1024);
            TextOutA(DeviceContext, 0, 78, DebugTextBuffer, (int)strlen(DebugTextBuffer));
        sprintf_s(DebugTextBuffer, _countof(DebugTextBuffer), "CPU Usage: %.02f %%", gPerformanceData.CPUPercent);
            TextOutA(DeviceContext, 0, 91, DebugTextBuffer, (int)strlen(DebugTextBuffer));
    }
    

    ReleaseDC(gGameWindow, DeviceContext);
}

#ifdef SIMD
__forceinline void ClearScreen(_In_ __m128i* Color) {
    for (int x = 0; x < GAME_RES_WIDTH * GAME_RES_HEIGHT; x += 4) {
        _mm_store_si128((PIXEL32*)gBackBuffer.Memory + x, *Color);
    }    
}
#else
__forceinline void ClearScreen(_In_ PIXEL32* Pixel){
    for (int x = 0; x < GAME_RES_WIDTH * GAME_RES_HEIGHT; x++){
        memcpy((PIXEL32*)gBackBuffer.Memory + x, Pixel, sizeof(PIXEL32));
    }
}
#endif

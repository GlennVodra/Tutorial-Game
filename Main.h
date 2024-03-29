#pragma once

#define GAME_NAME "CGame"

#define GAME_RES_WIDTH  384
#define GAME_RES_HEIGHT 240
#define GAME_BPP        32
#define GAME_DRAWING_AREA_MEMORY_SIZE (GAME_RES_WIDTH * GAME_RES_HEIGHT * (GAME_BPP / 8))

#define CALCULATE_AVERGAE_FPS_EVERY_X_FRAMES 128
#define TARGET_MICROSECONDS_PER_FRAME 16667ULL

#define SIMD

#define SUIT_0 0
#define SUIT_1 1
#define SUIT_2 1

#define FACING_DOWN_0 0
#define FACING_DOWN_1 1
#define FACING_DOWN_2 2

#define FACING_LEFT_0 3
#define FACING_LEFT_1 4
#define FACING_LEFT_2 5

#define FACING_RIGHT_0 6
#define FACING_RIGHT_1 7
#define FACING_RIGHT_2 8

#define FACING_UPWARD_0 9
#define FACING_UPWARD_1 10
#define FACING_UPWARD_2 11

#pragma warning(disable : 4820) //Memory padding
#pragma warning(disable : 5045) //Spectre/Meltdown Vulnerability

typedef struct GAMEBITMAP {
	BITMAPINFO BitmapInfo; 
	void* Memory;          
} GAMEBITMAP;

typedef struct PIXEL32{
	uint8_t Blue;
	uint8_t Green;
	uint8_t Red;
	uint8_t Alpha;
} PIXEL32;

typedef struct GAMEPERFDATA {
	uint64_t TotalFramesRendered;
	float RawFPSAverage;
	float CookedFPSAverage;
	int64_t PerfFrequency;
	MONITORINFO MonitorInfo;
	int32_t MonitorWidth;
	int32_t MonitorHeight;
	BOOL DisplayDebugInfo;
	DWORD HandleCount;
	PROCESS_MEMORY_COUNTERS_EX MemInfo;
	SYSTEM_INFO SystemInfo;
	int64_t CurrentSystemTime;
	int64_t PreviousSystemTime;
	double CPUPercent;
} GAMEPERFDATA;

typedef struct HERO {
	char Name[12];
	GAMEBITMAP Sprite[3][12];
	int32_t ScreenPosX;
	int32_t ScreenPosY;
	int32_t HP;
	int32_t Strength;
	int32_t MP;
} HERO;

LRESULT CALLBACK MainWindowProc(_In_ HWND WindowHandle, _In_ UINT Message, _In_ WPARAM wParam, _In_ LPARAM lParam);

DWORD CreateMainGameWindow(void);

BOOL GameIsAlreadyRunning(void);

void ProcessPlayerInput(void);

DWORD Load32BppBitmapFromFile(_In_ char* FileName, _Inout_ GAMEBITMAP* GameBitmap);

DWORD InitializeHero(void);

void RenderFrameGraphics(void);

void Blit32BppBitmapToBuffer(_In_ GAMEBITMAP* GameBitmap, _In_ uint16_t x, _In_ uint16_t y);

#ifdef SIMD
void ClearScreen(_In_ __m128i* Color);
#else 
void ClearScreen(_In_ PIXEL32* Color);
#endif

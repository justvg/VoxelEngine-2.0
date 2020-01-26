#define VOXEL_ENGINE_INTERNAL 1

// TODO(georgy): Replace glew lib with my code
#include <gl\glew.h>
#include <gl\wglew.h>

// TOOD(georgy): Get rid of this
#include <stdio.h>

#include "voxel_engine_platform.h"
#include "voxel_engine.hpp"

#include <Windows.h>
#include <timeapi.h>

global_variable bool8 GlobalRunning;
global_variable bool8 GlobalGamePause;
global_variable bool8 GlobalCursorShouldBeClipped;
global_variable LARGE_INTEGER GlobalPerformanceFrequency;
global_variable WINDOWPLACEMENT GlobalWindowPosition = { sizeof(GlobalWindowPosition) };

global_variable bool8 GlobalDEBUGCursor;

ALLOCATE_MEMORY(WinAllocateMemory)
{
	void *Result = VirtualAlloc(0, Size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
	return(Result);
}

FREE_MEMORY(WinFreeMemory)
{
	if(Memory)
	{
		VirtualFree(Memory, 0, MEM_RELEASE);
	}
}

OUTPUT_DEBUG_STRING(WinOutputDebugString)
{
	OutputDebugString(String);
}

READ_ENTIRE_FILE(WinReadEntireFile)
{
	read_entire_file_result Result = {};

	HANDLE FileHandle = CreateFile(Filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	if (FileHandle != INVALID_HANDLE_VALUE)
	{
		LARGE_INTEGER FileSize;
		if (GetFileSizeEx(FileHandle, &FileSize))
		{
			u32 FileSize32 = CheckTruncationUInt64(FileSize.QuadPart);
			Result.Memory = VirtualAlloc(0, FileSize32, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
			if (Result.Memory)
			{
				DWORD BytesRead;
				if ((ReadFile(FileHandle, Result.Memory, FileSize32, &BytesRead, 0)) && (FileSize32 == BytesRead))
				{
					Result.Size = FileSize32;
				}
				else
				{
					WinFreeMemory(Result.Memory);
					Result.Memory = 0;
				}
			}
		}

		CloseHandle(FileHandle);
	}

	return(Result);
}

internal void
WinWriteEntireFile(HANDLE FileHandle, void *Memory, u64 MemorySize)
{
	if(FileHandle != INVALID_HANDLE_VALUE)
	{
		DWORD BytesWritten;
		if(WriteFile(FileHandle, Memory, (DWORD)MemorySize, &BytesWritten, 0))
		{
			Assert(MemorySize == BytesWritten);
		}
		else
		{
			// TODO(georgy): Logging
		}
	}
}


global_variable bool32 GlobalIsFontRenderTargetInitialized;
global_variable HDC GlobalFontDeviceContext;
global_variable HBITMAP GlobalFontBitmap;
global_variable void *GlobalFontBits;
global_variable HFONT GlobalFont;
global_variable TEXTMETRIC GlobalTextMetric;
#define MAX_GLYPH_WIDTH 300
#define MAX_GLYPH_HEIGHT 300
PLATFORM_BEGIN_FONT(WinBeginFont)
{
	if(!GlobalIsFontRenderTargetInitialized)
	{
		GlobalFontDeviceContext = CreateCompatibleDC(GetDC(0));

		BITMAPINFO Info = {};
		Info.bmiHeader.biSize = sizeof(Info.bmiHeader);
		Info.bmiHeader.biWidth = MAX_GLYPH_WIDTH;
		Info.bmiHeader.biHeight = MAX_GLYPH_HEIGHT;
		Info.bmiHeader.biPlanes = 1;
		Info.bmiHeader.biBitCount = 32;
		Info.bmiHeader.biCompression = BI_RGB;
		GlobalFontBitmap = CreateDIBSection(GlobalFontDeviceContext, &Info, DIB_RGB_COLORS, &GlobalFontBits, 0 , 0);
		SelectObject(GlobalFontDeviceContext, GlobalFontBitmap);
		SetBkColor(GlobalFontDeviceContext, RGB(0, 0, 0));

		GlobalIsFontRenderTargetInitialized = true;
	}
	
	AddFontResourceEx(Filename, FR_PRIVATE, 0);
	int PixelHeight = 128;
	GlobalFont = CreateFontA(PixelHeight, 0, 0, 0,
							 FW_NORMAL,
							 FALSE, // NOTE(georgy): Italic
							 FALSE, // NOTE(georgy): Underline,
							 FALSE, // NOTE(georgy): StrikeOut,
							 DEFAULT_CHARSET,
							 OUT_DEFAULT_PRECIS,
							 CLIP_DEFAULT_PRECIS,
							 ANTIALIASED_QUALITY,
							 DEFAULT_PITCH|FF_DONTCARE,
							 FontName);

	SelectObject(GlobalFontDeviceContext, GlobalFont);
	GetTextMetrics(GlobalFontDeviceContext, &GlobalTextMetric);

	Font->HorizontalAdvances = (r32 *)WinAllocateMemory(Font->GlyphsCount*Font->GlyphsCount*sizeof(r32));
	Font->LineAdvance = GlobalTextMetric.tmAscent + GlobalTextMetric.tmDescent + GlobalTextMetric.tmExternalLeading;
	Font->AscenderHeight = GlobalTextMetric.tmAscent;
}

PLATFORM_END_FONT(WinEndFont)
{
	DWORD KerningPairsCount = GetKerningPairsW(GlobalFontDeviceContext, 0, 0);
	KERNINGPAIR *KerningPairs = (KERNINGPAIR *)WinAllocateMemory(KerningPairsCount*sizeof(KERNINGPAIR));
	GetKerningPairsW(GlobalFontDeviceContext, KerningPairsCount, KerningPairs);
	for(u32 KerningPairIndex = 0;
		KerningPairIndex < KerningPairsCount;
		KerningPairIndex++)
	{
		KERNINGPAIR *Pair = KerningPairs + KerningPairIndex;
		if((Pair->wFirst >= Font->FirstCodepoint) && (Pair->wFirst <= Font->LastCodepoint) &&
		   (Pair->wSecond >= Font->FirstCodepoint) && (Pair->wSecond <= Font->LastCodepoint))
		{
			u32 FirstGlyphIndex = Pair->wFirst - Font->FirstCodepoint;
			u32 SecondGlyphIndex = Pair->wSecond - Font->FirstCodepoint;
			Font->HorizontalAdvances[FirstGlyphIndex*Font->GlyphsCount + SecondGlyphIndex] += (r32)Pair->iKernAmount;
		}
	}
	WinFreeMemory(KerningPairs);

	DeleteObject(GlobalFont);
	GlobalFont = 0;
	GlobalTextMetric = {};
}

PLATFORM_LOAD_CODEPOINT_BITMAP(WinLoadCodepointBitmap)
{
	SelectObject(GlobalFontDeviceContext, GlobalFont);

	wchar_t WPoint = (wchar_t) Codepoint;

	SIZE Size;
	GetTextExtentPoint32W(GlobalFontDeviceContext, &WPoint, 1, &Size);

	PatBlt(GlobalFontDeviceContext, 0, 0, MAX_GLYPH_WIDTH, MAX_GLYPH_HEIGHT, BLACKNESS);
	SetTextColor(GlobalFontDeviceContext, RGB(255, 255, 255));
	int PreStepX = 128; 
	TextOutW(GlobalFontDeviceContext, PreStepX, 0, &WPoint, 1);

	i32 MinX = 10000;
	i32 MinY = 10000;
	i32 MaxX = -10000;
	i32 MaxY = -10000;
	u32 *Row = (u32 *)GlobalFontBits;
	for(i32 Y = 0;
		Y < MAX_GLYPH_HEIGHT;
		Y++)
	{
		u32 *Pixel = Row;
		for(i32 X = 0;
			X < MAX_GLYPH_WIDTH;
			X++)
		{
			if (*Pixel != 0)
			{
				if(MinX > X)
				{
					MinX = X;
				}
				if(MinY > Y)
				{
					MinY = Y;
				}
				if(MaxX < X)
				{
					MaxX = X;
				}
				if(MaxY < Y)
				{
					MaxY = Y;
				}
			}

			Pixel++;
		}

		Row += MAX_GLYPH_WIDTH;
	}

	u8* Result = 0;
	r32 KerningChange = 0.0f;
	if(MinX <= MaxX)
	{
		*Width = (MaxX + 1) - MinX;
		*Height = (MaxY + 1) - MinY;

		Result = (u8 *)WinAllocateMemory(*Width * *Height * 1);

		u8 *DestRow = Result;
		u32 *SourceRow = (u32 *)GlobalFontBits + MinY*MAX_GLYPH_WIDTH;
		for(i32 Y = MinY;
			Y <= MaxY;
			Y++)
		{
			u8 *Dest = (u8 *)DestRow;
			u32 *Source = SourceRow + MinX;
			for(i32 X = MinX;
				X <= MaxX;
				X++)
			{
				*Dest = *(u8 *)Source;
				Dest++;
				Source++;
			}
			
			DestRow += *Width*1;
			SourceRow += MAX_GLYPH_WIDTH;
		}

		*AlignPercentageY = ((MAX_GLYPH_HEIGHT - MinY) - (Size.cy - GlobalTextMetric.tmDescent)) / (r32)*Height;

		KerningChange = (r32)(MinX - PreStepX);
	}

	INT ThisCharWidth;
	GetCharWidth32W(GlobalFontDeviceContext, Codepoint, Codepoint, &ThisCharWidth);
	
	u32 ThisGlyphIndex = Codepoint - Font->FirstCodepoint;
	for(u32 OtherGlyphIndex = 0;
		OtherGlyphIndex < Font->GlyphsCount;
		OtherGlyphIndex++)
	{
		Font->HorizontalAdvances[ThisGlyphIndex*Font->GlyphsCount + OtherGlyphIndex] += ThisCharWidth - KerningChange;
		Font->HorizontalAdvances[OtherGlyphIndex*Font->GlyphsCount + ThisGlyphIndex] += KerningChange;
	}

	return(Result);
}

inline LARGE_INTEGER
WinGetPerformanceCounter()
{
	LARGE_INTEGER Result;
	QueryPerformanceCounter(&Result);
	return(Result);
}

inline r32
WinGetSecondsElapsed(LARGE_INTEGER Start, LARGE_INTEGER End)
{
	r32 Result = (End.QuadPart - Start.QuadPart) / (r32)GlobalPerformanceFrequency.QuadPart;
	return(Result);
}

internal void
WinInitOpenGL(HWND Window, HINSTANCE Instance, LPCSTR WindowClassName)
{
	HWND FakeWindow = CreateWindowEx(0, WindowClassName,
									 "FakeWindow",
									 WS_OVERLAPPEDWINDOW,
									 CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
									 0, 0, Instance, 0);

	HDC FakeWindowDC = GetDC(FakeWindow);

	PIXELFORMATDESCRIPTOR FakeDesiredPixelFormat = {};
	FakeDesiredPixelFormat.nSize = sizeof(FakeDesiredPixelFormat);
	FakeDesiredPixelFormat.nVersion = 1;
	FakeDesiredPixelFormat.iPixelType = PFD_TYPE_RGBA;
	FakeDesiredPixelFormat.dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER;
	FakeDesiredPixelFormat.cColorBits = 32;
	FakeDesiredPixelFormat.cAlphaBits = 8;
	FakeDesiredPixelFormat.cDepthBits = 24;
	FakeDesiredPixelFormat.cStencilBits = 8;
	FakeDesiredPixelFormat.iLayerType = PFD_MAIN_PLANE;

	int FakeSuggestedPixelFormatIndex = ChoosePixelFormat(FakeWindowDC, &FakeDesiredPixelFormat);
	PIXELFORMATDESCRIPTOR FakeSuggestedPixelFormat;
	DescribePixelFormat(FakeWindowDC, FakeSuggestedPixelFormatIndex, sizeof(FakeSuggestedPixelFormat), &FakeSuggestedPixelFormat);
	SetPixelFormat(FakeWindowDC, FakeSuggestedPixelFormatIndex, &FakeSuggestedPixelFormat);

	HGLRC FakeOpenGLRC = wglCreateContext(FakeWindowDC);
	if(wglMakeCurrent(FakeWindowDC, FakeOpenGLRC))
	{
		if (glewInit() == GLEW_OK)
		{
			wglMakeCurrent(0, 0);
			wglDeleteContext(FakeOpenGLRC);
			ReleaseDC(FakeWindow, FakeWindowDC);
			DestroyWindow(FakeWindow);

			if(WGLEW_ARB_create_context && WGLEW_ARB_pixel_format)
			{
				HDC WindowDC = GetDC(Window);

				int PixelFormatAttributes[] = 
				{
					WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
					WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
					WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
					WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
					WGL_COLOR_BITS_ARB, 32,
					WGL_DEPTH_BITS_ARB, 24,
					WGL_STENCIL_BITS_ARB, 8,
					WGL_SAMPLE_BUFFERS_ARB, 1,
					WGL_SAMPLES_ARB, 4,
					0
				};
				int ContextAttributes[] =
				{
					WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
					WGL_CONTEXT_MINOR_VERSION_ARB, 3,
					0
				};
				int PixelFormatIndex, PixelFormatCount;
				wglChoosePixelFormatARB(WindowDC, PixelFormatAttributes, 0, 1, &PixelFormatIndex, (UINT *)&PixelFormatCount);
				PIXELFORMATDESCRIPTOR SuggestedPixelFormat;
				DescribePixelFormat(WindowDC, PixelFormatIndex, sizeof(SuggestedPixelFormat), &SuggestedPixelFormat);
				SetPixelFormat(WindowDC, PixelFormatIndex, &SuggestedPixelFormat);

				HGLRC OpenGLRC = wglCreateContextAttribsARB(WindowDC, 0, ContextAttributes);
				if(wglMakeCurrent(WindowDC, OpenGLRC))
				{
					// NOTE(georgy): Success!
					wglSwapIntervalEXT(1);
					glEnable(GL_DEPTH_TEST);
					glEnable(GL_CULL_FACE);
					glEnable(GL_MULTISAMPLE);
					glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

					// glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
				}
				else
				{
					InvalidCodePath;
					// TODO(georgy): Diagnostic
				}

				ReleaseDC(Window, WindowDC);
			}
			else
			{
				InvalidCodePath;
				// TODO(georgy): Diagnostic
			}
		}
		else
		{
			InvalidCodePath;
			// TODO(georgy): Diagnostic
		}
	}
	else
	{
		InvalidCodePath;
		// TODO(georgy): Diagnostic
	}
}

struct window_dimension
{
	int Width;
	int Height;
};

internal window_dimension
WinGetWindowDimension(HWND Window)
{
	window_dimension Result;

	RECT Rect;
	GetClientRect(Window, &Rect);
	Result.Width = Rect.right - Rect.left;
	Result.Height = Rect.bottom - Rect.top;

	return(Result);
}

internal void
WinUpdateWindow(HDC DeviceContext, int WindowWidth, int WindowHeight)
{
	glClearColor(0.0f, 0.175f, 0.375f, 1.0f);
	
	SwapBuffers(DeviceContext);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

internal void 
ToggleFullscreen(HWND Window)
{
    // NOTE(george): This follows Raymond Chen's prescription
    // for fullscreen toggling, see:
    // https://blogs.msdn.microsoft.com/oldnewthing/20100412-00/?p=14353

    DWORD Style = GetWindowLong(Window, GWL_STYLE);
    if (Style & WS_OVERLAPPEDWINDOW) 
    {
        MONITORINFO MonitorInfo = { sizeof(MonitorInfo) };
        if (GetWindowPlacement(Window, &GlobalWindowPosition) &&
            GetMonitorInfo(MonitorFromWindow(Window, MONITOR_DEFAULTTOPRIMARY), &MonitorInfo)) 
        {
            SetWindowLong(Window, GWL_STYLE, Style & ~WS_OVERLAPPEDWINDOW);
            SetWindowPos(Window, HWND_TOP,
                        MonitorInfo.rcMonitor.left, MonitorInfo.rcMonitor.top,
                        MonitorInfo.rcMonitor.right - MonitorInfo.rcMonitor.left,
                        MonitorInfo.rcMonitor.bottom - MonitorInfo.rcMonitor.top,
                        SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        }
    }
    else 
    {
        SetWindowLong(Window, GWL_STYLE, Style | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(Window, &GlobalWindowPosition);
        SetWindowPos(Window, NULL, 0, 0, 0, 0,
                    SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                    SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
}

LRESULT CALLBACK 
WinWindowCallback(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam)
{
	LRESULT Result = 0;

	switch (Message)
	{
		case WM_ACTIVATEAPP:
		{
			if (WParam)
			{
				ShowCursor(FALSE);
				GlobalCursorShouldBeClipped = true && !GlobalDEBUGCursor;
			}
			else
			{
				ShowCursor(TRUE);
				GlobalCursorShouldBeClipped = false;
			}
		} break;

		case WM_CLOSE:
		{
			GlobalRunning = false;
		} break;
		
		case WM_DESTROY:
		{
			GlobalRunning = false;
		} break;

		case WM_PAINT:
		{
			PAINTSTRUCT Paint;
			HDC DeviceContext = BeginPaint(Window, &Paint);

			window_dimension Dimension = WinGetWindowDimension(Window);

			//WinUpdateWindow(DeviceContext, Dimension.Width, Dimension.Height);
			EndPaint(Window, &Paint);
		} break;

		default:
		{
			Result = DefWindowProc(Window, Message, WParam, LParam);
		} break;
	}

	return(Result);
}



struct platform_job_system_entry
{
	platform_job_system_callback *Callback;
	void *Data;	
};
struct platform_job_system_queue
{
	u32 volatile JobsToCompleteCount;
	u32 volatile JobsToCompleteGoal;

	u32 volatile EntryToRead;
	u32 volatile EntryToWrite;
	HANDLE Semaphore;
	
	platform_job_system_entry Entries[128];
};

PLATFORM_ADD_ENTRY(WinAddEntry)
{
	u32 NewEntryToWrite = (JobSystem->EntryToWrite + 1) % ArrayCount(JobSystem->Entries);
	Assert(NewEntryToWrite != JobSystem->EntryToRead);

	platform_job_system_entry *Entry = JobSystem->Entries + JobSystem->EntryToWrite;
	Entry->Callback = Callback;
	Entry->Data = Data;

	 JobSystem->JobsToCompleteGoal++;
	_WriteBarrier();

	JobSystem->EntryToWrite = NewEntryToWrite;
	ReleaseSemaphore(JobSystem->Semaphore, 1, 0);
}

internal bool32
WinDoNextJobQueueEntry(platform_job_system_queue *JobSystem)
{
	bool32 Sleep = false;

	u32 OriginalEntryToRead = JobSystem->EntryToRead;
	u32 NewEntryToRead = (OriginalEntryToRead + 1) % ArrayCount(JobSystem->Entries);
	if(JobSystem->EntryToRead != JobSystem->EntryToWrite)
	{
		u32 Index = InterlockedCompareExchange((LONG volatile *)&JobSystem->EntryToRead, NewEntryToRead, OriginalEntryToRead);

		if(Index == OriginalEntryToRead)
		{
			platform_job_system_entry *Entry = JobSystem->Entries + Index;
			Entry->Callback(JobSystem, Entry->Data);
			InterlockedIncrement((LONG volatile *)&JobSystem->JobsToCompleteCount);
		}
	}
	else
	{
		Sleep = true;
	}

	return(Sleep);
}

PLATFORM_COMPLETE_ALL_WORK(WinCompleteAllWork)
{
	while(JobSystem->JobsToCompleteCount != JobSystem->JobsToCompleteGoal) 
    { 
        WinDoNextJobQueueEntry(JobSystem);
    }

	JobSystem->JobsToCompleteCount = 0;
	JobSystem->JobsToCompleteGoal = 0;
}

DWORD WINAPI
ThreadProc(LPVOID lpParameter)
{
	platform_job_system_queue *JobSystem = (platform_job_system_queue *)lpParameter;

	while(1)
	{
		if(WinDoNextJobQueueEntry(JobSystem))
		{
			WaitForSingleObjectEx(JobSystem->Semaphore, INFINITE, false);
		}
	}

	return(0);
}

internal void 
WinInitializeJobSystem(platform_job_system_queue *JobSystem, u32 ThreadCount)
{
	JobSystem->JobsToCompleteCount = 0;
    JobSystem->JobsToCompleteGoal = 0;

	JobSystem->EntryToRead = 0;
	JobSystem->EntryToWrite = 0;

	u32 InitialCount = 0;
	JobSystem->Semaphore = CreateSemaphoreEx(0, InitialCount, INT_MAX, 0, 0, SEMAPHORE_ALL_ACCESS);

	for(u32 ThreadIndex = 0;
		ThreadIndex < ThreadCount;
		ThreadIndex++)
	{
		HANDLE ThreadHandle = CreateThread(0, 0, ThreadProc, JobSystem, 0, 0);
		CloseHandle(ThreadHandle);
	}
}

inline void
WinProcessKey(button *Button, bool IsDown)
{
	if(Button->EndedDown != (bool32)IsDown)
	{
		Button->HalfTransitionCount++;
	}

	Button->EndedDown = IsDown;
}

struct win_state
{
	u64 GameMemorySize;
	void *GameMemory;
	
	HANDLE RecordStateFile;
	HANDLE RecordStateMemoryMap;
	void *RecordStateMemory;

	HANDLE RecordInputFile;	
	bool32 InputRecording;

	HANDLE PlaybackInputFile;
	bool32 InputPlayback;
};

#if VOXEL_ENGINE_INTERNAL 
internal void DEBUGEndDebugFrameAndRender(game_memory* Memory, game_input* Input, r32 BufferWidth, r32 BufferHeight);

internal void
WinBeginRecordingInput(win_state *WinState)
{
	Assert(!WinState->InputRecording);

	WinState->InputRecording = true;
	WinState->RecordInputFile = 
		CreateFileA("C:/Users/georg/source/repos/VoxelEngine 2.0/build/playback_input.vep", GENERIC_WRITE,
					0, 0, CREATE_ALWAYS, 0, 0);

	CopyMemory(WinState->RecordStateMemory, WinState->GameMemory, WinState->GameMemorySize);
	
	DEBUGGlobalPlaybackInfo.RecordPhaseStarted = true;
	DEBUGGlobalPlaybackInfo.RecordPhase = true;
	DEBUGGlobalPlaybackInfo.ChunksModifiedDuringRecordPhaseCount = 0;
	DEBUGGlobalPlaybackInfo.ChunksUnloadedDuringRecordPhaseCount = 0;
}

internal void
WinRecordInput(win_state *WinState, game_input *GameInput)
{
	DWORD BytesWritten;
	WriteFile(WinState->RecordInputFile, GameInput, sizeof(*GameInput), &BytesWritten, 0);
}

internal void
WinEndRecordingInput(win_state *WinState)
{
	Assert(WinState->InputRecording);

	CloseHandle(WinState->RecordInputFile);
	WinState->RecordInputFile = 0;
	WinState->InputRecording = false;

	DEBUGGlobalPlaybackInfo.RecordPhase = false;
}

internal void
WinBeginInputPlayback(win_state *WinState)
{
	Assert(!WinState->InputPlayback);

	WinState->InputPlayback = true;
	WinState->PlaybackInputFile = 
		CreateFileA("C:/Users/georg/source/repos/VoxelEngine 2.0/build/playback_input.vep", GENERIC_READ,
					0, 0, OPEN_EXISTING, 0, 0);

	CopyMemory(WinState->GameMemory, WinState->RecordStateMemory, WinState->GameMemorySize);
	DEBUGGlobalPlaybackInfo.RefreshNow = true;
}

internal void
WinEndInputPlayback(win_state *WinState)
{
	Assert(WinState->InputPlayback);

	CloseHandle(WinState->PlaybackInputFile);
	WinState->PlaybackInputFile = 0;
	WinState->InputPlayback = false;
}

internal void
WinPlaybackInput(win_state *WinState, game_input *GameInput)
{
	DWORD BytesRead = 0;
    if(ReadFile(WinState->PlaybackInputFile, GameInput, sizeof(*GameInput), &BytesRead, 0))
	{
		if(BytesRead == 0)
		{
			WinEndInputPlayback(WinState);
			WinBeginInputPlayback(WinState);
			ReadFile(WinState->PlaybackInputFile, GameInput, sizeof(*GameInput), &BytesRead, 0);
		}
	}
}
#endif

int CALLBACK
WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CommandLine, int ShowCode)
{	
	win_state WinState = {};

	platform_job_system_queue JobSystem;
	WinInitializeJobSystem(&JobSystem, 3);

	DWORD ThreadID = GetCurrentThreadId();

	QueryPerformanceFrequency(&GlobalPerformanceFrequency);

	UINT DesiredSchedulerMS = 1;
	bool32 SleepIsGranular = (timeBeginPeriod(DesiredSchedulerMS) == TIMERR_NOERROR);

	WNDCLASS WindowClass = {};
	WindowClass.style = CS_OWNDC | CS_VREDRAW | CS_HREDRAW;
	WindowClass.lpfnWndProc = WinWindowCallback;
	WindowClass.hInstance = Instance;
	WindowClass.hbrBackground = CreateSolidBrush(RGB(0, (u8)(0.175f*255.0f), (u8)(0.375f*255.0f)));
	WindowClass.lpszClassName = "VoxelEngineWindowClass";

	if(RegisterClass(&WindowClass))
	{
		HWND Window = CreateWindowEx(0, WindowClass.lpszClassName,
									 "Voxel Engine 2.0", 
									 WS_OVERLAPPEDWINDOW,
									 CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
									 0, 0, Instance, 0);
		if(Window)
		{
			WinInitOpenGL(Window, Instance, WindowClass.lpszClassName);
			ToggleFullscreen(Window);
			ShowWindow(Window, SW_SHOW);
			
			window_dimension Dimension = WinGetWindowDimension(Window);
			glViewport(0, 0, Dimension.Width, Dimension.Height);
			glClearColor(0.0f, 0.175f, 0.375f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			int RefreshRate = 60;
			HDC WindowDC = GetDC(Window);
			int WinRefreshRate = GetDeviceCaps(WindowDC, VREFRESH);
			if (WinRefreshRate > 0)
			{
				RefreshRate = WinRefreshRate;
			}
			int GameRefreshRate = RefreshRate / 2;
			r32 TargetSecondsPerFrame = 1.0f / GameRefreshRate;
			ReleaseDC(Window, WindowDC);

			game_memory GameMemory = {};
			GameMemory.PermanentStorageSize = Gigabytes(2);
			GameMemory.TemporaryStorageSize = Megabytes(128);
			GameMemory.DebugStorageSize = Megabytes(64);
			WinState.GameMemorySize = GameMemory.PermanentStorageSize + GameMemory.TemporaryStorageSize + GameMemory.DebugStorageSize;
			WinState.GameMemory = VirtualAlloc(0, WinState.GameMemorySize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
			GameMemory.PermanentStorage = WinState.GameMemory;
			GameMemory.TemporaryStorage = (u8 *)GameMemory.PermanentStorage + GameMemory.PermanentStorageSize;
			GameMemory.DebugStorage = (u8 *)GameMemory.TemporaryStorage + GameMemory.TemporaryStorageSize;

			GameMemory.JobSystemQueue = &JobSystem;

			GameMemory.PlatformAddEntry = WinAddEntry;
			GameMemory.PlatformCompleteAllWork = WinCompleteAllWork;
			GameMemory.PlatformReadEntireFile = WinReadEntireFile;
			GameMemory.PlatformAllocateMemory = WinAllocateMemory;
			GameMemory.PlatformFreeMemory = WinFreeMemory;
			GameMemory.PlatformOutputDebugString = WinOutputDebugString;

			GameMemory.PlatformLoadCodepointBitmap = WinLoadCodepointBitmap;
			GameMemory.PlatformBeginFont = WinBeginFont;
			GameMemory.PlatformLoadCodepointBitmap = WinLoadCodepointBitmap;
			GameMemory.PlatformEndFont = WinEndFont;

			// TODO(georgy): Make the path to be created automatically in .exe directory
			WinState.RecordStateFile = 
				CreateFileA("C:/Users/georg/source/repos/VoxelEngine 2.0/build/playback_state.vep", GENERIC_WRITE|GENERIC_READ, 
							0, 0, CREATE_ALWAYS, 0, 0);

			LARGE_INTEGER MaxSize;
			MaxSize.QuadPart = WinState.GameMemorySize;
			WinState.RecordStateMemoryMap = 
				CreateFileMapping(WinState.RecordStateFile, 0, PAGE_READWRITE,
								  MaxSize.HighPart, MaxSize.LowPart, 0);

			WinState.RecordStateMemory = 
				MapViewOfFile(WinState.RecordStateMemoryMap, FILE_MAP_ALL_ACCESS, 0, 0, WinState.GameMemorySize);


			game_input GameInput = {};
			GameInput.dt = TargetSecondsPerFrame;

			RAWINPUTDEVICE RID;
			RID.usUsagePage = 1;
			RID.usUsage = 2; // NOTE(georgy): 2 is for mouse input
			RID.dwFlags = 0;
			RID.hwndTarget = Window;
			RegisterRawInputDevices(&RID, 1, sizeof(RID));

			GlobalRunning = true;
			LARGE_INTEGER LastCounter = WinGetPerformanceCounter();
			while(GlobalRunning)
			{
#if VOXEL_ENGINE_INTERNAL
				if(WasDown(&GameInput.Pause))
				{
					GlobalGamePause = !GlobalGamePause;
					GameInput.dt = GlobalGamePause ? 0.0f : TargetSecondsPerFrame;
					GlobalDebugTable.ProfilePause = !GlobalDebugTable.ProfilePause || GlobalGamePause;
				}
#endif

				BEGIN_BLOCK(InputAndMessageTime);

				if (GlobalCursorShouldBeClipped)
				{
					RECT WindowRect;
					GetWindowRect(Window, &WindowRect);
					LONG WindowRectHeight = WindowRect.bottom - WindowRect.top;
					LONG WindowRectWidth = WindowRect.right - WindowRect.left;
					WindowRect.top = WindowRect.bottom = WindowRect.top + WindowRectHeight/2;
					WindowRect.left = WindowRect.right = WindowRect.left + WindowRectWidth/2;
					ClipCursor(&WindowRect);
				}
				else
				{
					ClipCursor(0);
				}

				GameInput.MouseXDisplacement = GameInput.MouseYDisplacement = 0;
				GameInput.MouseRight.HalfTransitionCount = GameInput.MouseLeft.HalfTransitionCount = 0;

				for(u32 ButtonIndex = 0;
					ButtonIndex < ArrayCount(GameInput.Buttons);
					ButtonIndex++)
				{
					button *Button = GameInput.Buttons + ButtonIndex;
					Button->HalfTransitionCount = 0;
				}

				window_dimension Dimension = WinGetWindowDimension(Window);
				POINT MouseP;
				GetCursorPos(&MouseP);
				ScreenToClient(Window, &MouseP);
				GameInput.MouseX = (MouseP.x - 0.5f*Dimension.Width);
				GameInput.MouseY = (0.5f*Dimension.Height - MouseP.y);				

				MSG Message;
				while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
				{
					switch (Message.message)
					{
						case WM_QUIT:
						{
							GlobalRunning = false;
						} break;

						case WM_SYSKEYDOWN:
						case WM_SYSKEYUP:
						case WM_KEYDOWN:
						case WM_KEYUP:
						{
							u32 KeyCode = (u32)Message.wParam;

							bool WasDown = ((Message.lParam & (1 << 30)) != 0);
							bool IsDown = ((Message.lParam & (1 << 31)) == 0);
							if (WasDown != IsDown)
							{
								if (KeyCode == 'W')
								{
									WinProcessKey(&GameInput.MoveForward, IsDown);
								}
								if (KeyCode == 'S')
								{
									WinProcessKey(&GameInput.MoveBack, IsDown);
								}
								if (KeyCode == 'D')
								{
									WinProcessKey(&GameInput.MoveRight, IsDown);
								}
								if (KeyCode == 'A')
								{
									WinProcessKey(&GameInput.MoveLeft, IsDown);
								}
								if (KeyCode == VK_SPACE)
								{
									WinProcessKey(&GameInput.MoveUp, IsDown);
								}

#if VOXEL_ENGINE_INTERNAL
								if (KeyCode == 'P')
								{
									// TODO(georgy): I don't want the pause button to be _game_ input
									WinProcessKey(&GameInput.Pause, IsDown);
									// if(IsDown)
									// {
									// 	GlobalGamePause = !GlobalGamePause;
									// 	GameInput.dt = GlobalGamePause ? 0.0f : TargetSecondsPerFrame;
									// 	GlobalDebugTable.ProfilePause = !GlobalDebugTable.ProfilePause || GlobalGamePause;
									// }
								}
								if (KeyCode == 'L')
								{
									if(IsDown)
									{
										if(!WinState.InputPlayback)
										{
											WinCompleteAllWork(GameMemory.JobSystemQueue);
											if(!WinState.InputRecording)
											{
												WinBeginRecordingInput(&WinState);
											}
											else
											{
												WinEndRecordingInput(&WinState);
												WinBeginInputPlayback(&WinState);
											}
										}
										else
										{
											WinEndInputPlayback(&WinState);
										}
									}
								}
								if (KeyCode == 0x32)
								{
									if(IsDown)
									{
										GlobalDEBUGCursor = !GlobalDEBUGCursor;
										GlobalCursorShouldBeClipped = !GlobalDEBUGCursor;
										ShowCursor(GlobalDEBUGCursor);
									}
								}
#endif
							}
						} break;

						case WM_INPUT:
						{
							RAWINPUT RawInput;
							UINT RawInputSize = sizeof(RawInput);
							GetRawInputData((HRAWINPUT)Message.lParam, RID_INPUT,
											&RawInput, &RawInputSize, sizeof(RAWINPUTHEADER));
							
							if (RawInput.header.dwType == RIM_TYPEMOUSE)
							{
								if(!GlobalDEBUGCursor)
								{
									GameInput.MouseXDisplacement = RawInput.data.mouse.lLastX;
									GameInput.MouseYDisplacement = RawInput.data.mouse.lLastY;
								}
								// GameInput.MouseX += RawInput.data.mouse.lLastX;
								// GameInput.MouseY += RawInput.data.mouse.lLastY;
								
								if(RawInput.data.mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_DOWN ||
								   RawInput.data.mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_UP)
								{
									WinProcessKey(&GameInput.MouseLeft, RawInput.data.mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_DOWN);
								}
									
								if(RawInput.data.mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_DOWN ||
								   RawInput.data.mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_UP)
								{
									WinProcessKey(&GameInput.MouseRight, RawInput.data.mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_DOWN);
								}
							}
						} break;
						
						default:
						{
							TranslateMessage(&Message);
							DispatchMessage(&Message);
						} break;
					}
				}
				
				END_BLOCK(InputAndMessageTime);

#if VOXEL_ENGINE_INTERNAL
				if(!GlobalGamePause)
				{
					if(WinState.InputRecording)
					{
						WinRecordInput(&WinState, &GameInput);
					}

					if(WinState.InputPlayback)
					{
						game_input TempInput = GameInput;
						WinPlaybackInput(&WinState, &GameInput);
						GameInput.MouseX = TempInput.MouseX;
						GameInput.MouseY = TempInput.MouseY;
						GameInput.MouseLeft = TempInput.MouseLeft;
						GameInput.Pause = TempInput.Pause;
					}
				}
#endif

				BEGIN_BLOCK(GameUpdateTime);

				GameUpdate(&GameMemory, &GameInput, GlobalGamePause, Dimension.Width, Dimension.Height);

				END_BLOCK(GameUpdateTime);

#if VOXEL_ENGINE_INTERNAL
				BEGIN_BLOCK(DebugStuffTime);

				DEBUGEndDebugFrameAndRender(&GameMemory, &GameInput, (r32)Dimension.Width, (r32)Dimension.Height);

				END_BLOCK(DebugStuffTime);
#endif

				BEGIN_BLOCK(UpdateWindowTime);

				HDC DeviceContext = GetDC(Window);
				WinUpdateWindow(DeviceContext, Dimension.Width, Dimension.Height);
				ReleaseDC(Window, DeviceContext);

				END_BLOCK(UpdateWindowTime);
				

				BEGIN_BLOCK(SleepTime);

#if 1
				r32 SecondsElapsedForFrame = WinGetSecondsElapsed(LastCounter, WinGetPerformanceCounter());
				if(SecondsElapsedForFrame < TargetSecondsPerFrame)
				{
					if(SleepIsGranular)
					{
						DWORD SleepMS = (DWORD)(1000.0f * (TargetSecondsPerFrame - SecondsElapsedForFrame));

						if(SleepMS > 0)
						{
							Sleep(SleepMS);
						}
					}

					while (SecondsElapsedForFrame < TargetSecondsPerFrame)
					{
						SecondsElapsedForFrame = WinGetSecondsElapsed(LastCounter, WinGetPerformanceCounter());
					}
				}
#endif

				END_BLOCK(SleepTime);


				LARGE_INTEGER EndCounter = WinGetPerformanceCounter();
				r32 MSPerFrame = 1000.0f * WinGetSecondsElapsed(LastCounter, EndCounter);
				FRAME_MARKER(MSPerFrame);
				LastCounter = EndCounter;
			}
		}
	}

	return(0);
}

#if VOXEL_ENGINE_INTERNAL

#include "voxel_engine_debug.hpp"

#endif
// TODO(georgy): Replace glew lib with my code
#include "gl\glew.h"
#include "gl\wglew.h"

// TOOD(georgy): Get rid of this
#include <stdio.h>

#include "voxel_engine_platform.h"
#include "voxel_engine.hpp"

#include <Windows.h>
#include <mmeapi.h>
#include <dsound.h>
#include <timeapi.h>

global_variable bool8 GlobalRunning;
global_variable bool8 GlobalGamePause;
global_variable LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;
global_variable bool8 GlobalCursorShouldBeClipped;
global_variable LARGE_INTEGER GlobalPerformanceFrequency;
global_variable WINDOWPLACEMENT GlobalWindowPosition = { sizeof(GlobalWindowPosition) };

global_variable bool8 GlobalDEBUGCursor;

internal ALLOCATE_MEMORY(WinAllocateMemory)
{
	void *Result = VirtualAlloc(0, Size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
	return(Result);
}

internal FREE_MEMORY(WinFreeMemory)
{
	if(Memory)
	{
		VirtualFree(Memory, 0, MEM_RELEASE);
	}
}

internal OUTPUT_DEBUG_STRING(WinOutputDebugString)
{
	OutputDebugString(String);
}

internal READ_ENTIRE_FILE(WinReadEntireFile)
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

struct win_file_handle
{
	HANDLE WinHandle;
};

internal OPEN_FILE(WinOpenFile)
{
	platform_file_handle Result = {};
	HANDLE *FileHandle = (HANDLE *)VirtualAlloc(0, sizeof(win_file_handle), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	Result.Platform = FileHandle;

	*FileHandle = CreateFile(Filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);

	return(Result);
}	

internal READ_DATA_FROM_FILE(WinReadDataFromFile)
{
	win_file_handle *FileHandle = (win_file_handle *)Source.Platform;
	if (FileHandle->WinHandle != INVALID_HANDLE_VALUE)
	{
		OVERLAPPED Overlapped = {};
		Overlapped.Offset = (DWORD)(DataOffset & 0xFFFFFFFF);
		Overlapped.OffsetHigh = (DWORD)((DataOffset >> 32) & 0xFFFFFFFF);

		DWORD BytesRead;
		if(ReadFile(FileHandle->WinHandle, Dest, Size, &BytesRead, &Overlapped) &&
		   (BytesRead == Size))
		{
			// NOTE(georgy): Successful read!
		}
		else
		{
			InvalidCodePath;
		}
	}
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
					WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
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

					glClearColor(SquareRoot(0.5f), 0.0f, SquareRoot(0.5f), 1.0f);
					glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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

typedef HRESULT WINAPI direct_sound_create(LPGUID lpGuid, LPDIRECTSOUND* ppDS, LPUNKNOWN  pUnkOuter);

internal void
WinInitDirectSound(HWND Window, u32 SamplesPerSecond, u32 BufferSize)
{
	HMODULE DirectSoundLibrary = LoadLibrary("dsound.dll");
	if(DirectSoundLibrary)
	{
		direct_sound_create *DirectSoundCreate = (direct_sound_create *)GetProcAddress(DirectSoundLibrary, "DirectSoundCreate");

		LPDIRECTSOUND DirectSound;
		if(DirectSoundCreate && (DirectSoundCreate(0, &DirectSound, 0) == DS_OK))
		{
			WAVEFORMATEX WaveFormat = {};
			WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
			WaveFormat.nChannels = 2;
			WaveFormat.nSamplesPerSec = SamplesPerSecond;
			WaveFormat.wBitsPerSample = 16;
			WaveFormat.nBlockAlign = (WaveFormat.nChannels * WaveFormat.wBitsPerSample) / 8;
			WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec * WaveFormat.nBlockAlign;

			if(DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY) == DS_OK)
			{
				DSBUFFERDESC BufferDescription = {sizeof(DSBUFFERDESC)};
				BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;

				LPDIRECTSOUNDBUFFER PrimaryBuffer;
				if(DirectSound->CreateSoundBuffer(&BufferDescription, &PrimaryBuffer, 0) == DS_OK)
				{
					if(PrimaryBuffer->SetFormat(&WaveFormat) == DS_OK)
					{
						// NOTE(georgy): Primary buffer's format is set
						OutputDebugString("Primary buffer's format is set.\n");
					}
					else
					{
						// TODO(georgy): Diagnostic
					}
				}
				else
				{
					// TODO(georgy): Diagnostic
				}
			}
			else
			{
				// TODO(georgy): Diagnostic
			}

			DSBUFFERDESC BufferDescription = {sizeof(DSBUFFERDESC)};
			BufferDescription.dwFlags =  DSBCAPS_GETCURRENTPOSITION2;
#if VOXEL_ENGINE_INTERNAL
			BufferDescription.dwFlags |= DSBCAPS_GLOBALFOCUS;
#endif
			BufferDescription.dwBufferBytes = BufferSize;
			BufferDescription.lpwfxFormat = &WaveFormat;
			if(DirectSound->CreateSoundBuffer(&BufferDescription, &GlobalSecondaryBuffer, 0) == DS_OK)
			{
				// NOTE(georgy): Secondary buffer created
				OutputDebugString("Secondary buffer created.\n");
			}
		}
		else
		{
			// TODO(georgy): Diagnostic
		}
	}
	else
	{
		// TODO(georgy): Diagnostic
	}
}

struct win_sound_output
{
	u32 SamplesPerSecond;
	u32 RunningSampleIndex;
	u32 BytesPerSample;
	DWORD SecondaryBufferSize;
	i32 LatencySampleCount;
	DWORD SafetyBytes;
};

internal void
WinFillSoundBuffer(win_sound_output *SoundOutput, DWORD ByteToLock, DWORD BytesToWrite, game_sound_output_buffer *SourceBuffer)
{
	VOID *Part1, *Part2;
	DWORD Part1Size, Part2Size;
	if(GlobalSecondaryBuffer->Lock(ByteToLock, BytesToWrite, &Part1, &Part1Size, &Part2, &Part2Size, 0) == DS_OK)
	{
		DWORD Part1SampleCount = Part1Size / SoundOutput->BytesPerSample;
		i16 *SampleDest = (i16 *)Part1;
		i16 *SampleSource = SourceBuffer->Samples;
		for(DWORD SampleIndex = 0;
			SampleIndex < Part1SampleCount;
			SampleIndex++)
		{
			*SampleDest++ = *SampleSource++;
			*SampleDest++ = *SampleSource++;

			SoundOutput->RunningSampleIndex++;
		}

		DWORD Part2SampleCount = Part2Size / SoundOutput->BytesPerSample;
		SampleDest = (i16 *)Part2;
		for(DWORD SampleIndex = 0;
			SampleIndex < Part2SampleCount;
			SampleIndex++)
		{
			*SampleDest++ = *SampleSource++;
			*SampleDest++ = *SampleSource++;

			SoundOutput->RunningSampleIndex++;
		}

		GlobalSecondaryBuffer->Unlock(Part1, Part1Size, Part2, Part2Size);
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
	SwapBuffers(DeviceContext);
	// glFinish();
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

			// WinUpdateWindow(DeviceContext, Dimension.Width, Dimension.Height);
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

internal PLATFORM_ADD_ENTRY(WinAddEntry)
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

internal PLATFORM_COMPLETE_ALL_WORK(WinCompleteAllWork)
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
	DEBUGGlobalPlaybackInfo.PlaybackPhase = true;
}

internal void
WinEndInputPlayback(win_state *WinState)
{
	Assert(WinState->InputPlayback);

	CloseHandle(WinState->PlaybackInputFile);
	WinState->PlaybackInputFile = 0;
	WinState->InputPlayback = false;

	DEBUGGlobalPlaybackInfo.PlaybackPhase = false;
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
	WindowClass.hCursor = LoadCursor(0, IDC_ARROW);
	// WindowClass.hbrBackground = CreateSolidBrush(RGB((u8)(SquareRoot(0.5f)*255.0f), 0, (u8)(SquareRoot(0.5f)*255.0f)));
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

			int RefreshRate = 60;
			HDC WindowDC = GetDC(Window);
			int WinRefreshRate = GetDeviceCaps(WindowDC, VREFRESH);
			if (WinRefreshRate > 0)
			{
				RefreshRate = WinRefreshRate;
			}
			// int GameRefreshRate = RefreshRate;
			int GameRefreshRate = RefreshRate / 2;
			r32 TargetSecondsPerFrame = 1.0f / GameRefreshRate;
			ReleaseDC(Window, WindowDC);

			win_sound_output SoundOutput = {};
			SoundOutput.SamplesPerSecond = 44100;
			SoundOutput.BytesPerSample = sizeof(i16)*2;
			SoundOutput.SecondaryBufferSize = SoundOutput.SamplesPerSecond* SoundOutput.BytesPerSample;
			SoundOutput.LatencySampleCount = 3*(SoundOutput.SamplesPerSecond / GameRefreshRate);
			SoundOutput.SafetyBytes = ((SoundOutput.SamplesPerSecond*SoundOutput.BytesPerSample) / GameRefreshRate) / 2;

			bool32 SoundIsValid = false;
			DWORD AudioLatencyBytes = 0;
			r32 AudioLatencySeconds = 0;

			i16 *Samples = (i16 *)VirtualAlloc(0, SoundOutput.SamplesPerSecond*SoundOutput.BytesPerSample, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
			WinInitDirectSound(Window, SoundOutput.SamplesPerSecond, SoundOutput.SecondaryBufferSize);
			// NOTE(georgy): Clear sound buffer
			{
				VOID *Part1, *Part2;
				DWORD Part1Size, Part2Size;
				if(GlobalSecondaryBuffer->Lock(0, SoundOutput.SecondaryBufferSize, &Part1, &Part1Size, &Part2, &Part2Size, 0) == DS_OK)
				{
					u8 *Dest = (u8 *)Part1;
					for(u32 ByteIndex = 0;
						ByteIndex < Part1Size;
						ByteIndex++)
					{
						*Dest++ = 0;
					}

					Dest = (u8 *)Part2;
					for(u32 ByteIndex = 0;
						ByteIndex < Part2Size;
						ByteIndex++)
					{
						*Dest++ = 0;
					}

					GlobalSecondaryBuffer->Unlock(Part1, Part1Size, Part2, Part2Size);
				}
			}

			game_memory GameMemory = {};
			GameMemory.PermanentStorageSize = Gigabytes(2);
			GameMemory.TemporaryStorageSize = Megabytes(128);
			GameMemory.DebugStorageSize = Megabytes(64);
			WinState.GameMemorySize = GameMemory.PermanentStorageSize + GameMemory.TemporaryStorageSize + GameMemory.DebugStorageSize;
			WinState.GameMemory = VirtualAlloc(0, WinState.GameMemorySize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
			GameMemory.PermanentStorage = WinState.GameMemory;
			GameMemory.TemporaryStorage = (u8 *)GameMemory.PermanentStorage + GameMemory.PermanentStorageSize;
			GameMemory.DebugStorage = (u8 *)GameMemory.TemporaryStorage + GameMemory.TemporaryStorageSize;

			if(GameMemory.PermanentStorage && GameMemory.TemporaryStorage && Samples)
			{
				GameMemory.JobSystemQueue = &JobSystem;

				GameMemory.PlatformAPI.AddEntry = WinAddEntry;
				GameMemory.PlatformAPI.CompleteAllWork = WinCompleteAllWork;
				GameMemory.PlatformAPI.ReadEntireFile = WinReadEntireFile;
				GameMemory.PlatformAPI.OpenFile = WinOpenFile;
				GameMemory.PlatformAPI.ReadDataFromFile = WinReadDataFromFile;
				GameMemory.PlatformAPI.AllocateMemory = WinAllocateMemory;
				GameMemory.PlatformAPI.FreeMemory = WinFreeMemory;
				GameMemory.PlatformAPI.OutputDebugString = WinOutputDebugString;

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

				game_input DebugCameraInput = {};

				RAWINPUTDEVICE RID;
				RID.usUsagePage = 1;
				RID.usUsage = 2; // NOTE(georgy): 2 is for mouse input
				RID.dwFlags = 0;
				RID.hwndTarget = Window;
				RegisterRawInputDevices(&RID, 1, sizeof(RID));

				GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

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

					game_input *Input;
					DEBUG_IF(DebugCamera, DebugTools)
					{
						Input = &DebugCameraInput;
					}				
					else
					{
						Input = &GameInput;
					}

					Input->MouseXDisplacement = Input->MouseYDisplacement = 0;
					Input->MouseRight.HalfTransitionCount = Input->MouseLeft.HalfTransitionCount = 0;

					for(u32 ButtonIndex = 0;
						ButtonIndex < ArrayCount(Input->Buttons);
						ButtonIndex++)
					{
						button *Button = Input->Buttons + ButtonIndex;
						Button->HalfTransitionCount = 0;
					}

					window_dimension Dimension = WinGetWindowDimension(Window);
					POINT MouseP;
					GetCursorPos(&MouseP);
					ScreenToClient(Window, &MouseP);
					Input->MouseX = (MouseP.x - 0.5f*Dimension.Width);
					Input->MouseY = (0.5f*Dimension.Height - MouseP.y);				

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
										WinProcessKey(&Input->MoveForward, IsDown);
									}
									if (KeyCode == 'S')
									{
										WinProcessKey(&Input->MoveBack, IsDown);
									}
									if (KeyCode == 'D')
									{
										WinProcessKey(&Input->MoveRight, IsDown);
									}
									if (KeyCode == 'A')
									{
										WinProcessKey(&Input->MoveLeft, IsDown);
									}
									if (KeyCode == VK_SPACE)
									{
										WinProcessKey(&Input->MoveUp, IsDown);
									}
									if (KeyCode == VK_ESCAPE)
									{
										WinProcessKey(&Input->Esc, IsDown);
									}

	#if VOXEL_ENGINE_INTERNAL
									if (KeyCode == 'P')
									{
										// TODO(georgy): I don't want the pause button to be _game_ input
										WinProcessKey(&Input->Pause, IsDown);
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

								if(IsDown)
								{
									bool32 AltKeyWasDown = (Message.lParam & (1 << 29));
									if((KeyCode == VK_F4) && AltKeyWasDown)
									{
										GlobalRunning = false;
									}
									if((KeyCode == VK_RETURN) && AltKeyWasDown)
									{
										if(Message.hwnd)
										{
											ToggleFullscreen(Message.hwnd);
										}
									}
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
										Input->MouseXDisplacement = RawInput.data.mouse.lLastX;
										Input->MouseYDisplacement = RawInput.data.mouse.lLastY;
									}
									// GameInput.MouseX += RawInput.data.mouse.lLastX;
									// GameInput.MouseY += RawInput.data.mouse.lLastY;
									
									if(RawInput.data.mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_DOWN ||
									RawInput.data.mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_UP)
									{
										WinProcessKey(&Input->MouseLeft, RawInput.data.mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_DOWN);
									}
										
									if(RawInput.data.mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_DOWN ||
									RawInput.data.mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_UP)
									{
										WinProcessKey(&Input->MouseRight, RawInput.data.mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_DOWN);
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

					GameUpdate(&GameMemory, &GameInput, Dimension.Width, Dimension.Height, GlobalGamePause, DebugCamera, &DebugCameraInput);
					if(GameInput.QuitRequested)
					{
						GlobalRunning = false;
					}

					END_BLOCK(GameUpdateTime);

					
					BEGIN_BLOCK(SoundUpdateTime);

					r32 FromBeginToAudioSeconds = WinGetSecondsElapsed(LastCounter, WinGetPerformanceCounter());

					DWORD PlayCursor;
					DWORD WriteCursor;
					if(GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor) == DS_OK)
					{
						if(!SoundIsValid)
						{
							SoundOutput.RunningSampleIndex = WriteCursor / SoundOutput.BytesPerSample;
							SoundIsValid = true;
						}

						DWORD ByteToLock = (SoundOutput.RunningSampleIndex*SoundOutput.BytesPerSample) % SoundOutput.SecondaryBufferSize;

						DWORD ExpectedSoundBytesPerFrame = (SoundOutput.SamplesPerSecond*SoundOutput.BytesPerSample) / GameRefreshRate;
						r32 SecondsLeftUntilFlip = TargetSecondsPerFrame - FromBeginToAudioSeconds;
						DWORD ExpectedBytesUntilFlip = (DWORD)((SecondsLeftUntilFlip/TargetSecondsPerFrame)*
																(r32)ExpectedSoundBytesPerFrame);

						DWORD ExpectedFrameBoundaryByte = PlayCursor + ExpectedBytesUntilFlip;

						DWORD SafeWriteCursor = WriteCursor;
						if(SafeWriteCursor < PlayCursor)
						{
							SafeWriteCursor += SoundOutput.SecondaryBufferSize;
						} 
						Assert(SafeWriteCursor >= PlayCursor);
						SafeWriteCursor += SoundOutput.SafetyBytes;

						bool32 AudioCardIsLowLatency = (SafeWriteCursor < ExpectedFrameBoundaryByte);

						DWORD TargetCursor = 0;
						if(AudioCardIsLowLatency)
						{
							TargetCursor = (ExpectedFrameBoundaryByte + ExpectedSoundBytesPerFrame);
						}
						else
						{
							TargetCursor = (WriteCursor + ExpectedSoundBytesPerFrame + SoundOutput.SafetyBytes);
						}
						TargetCursor = (TargetCursor % SoundOutput.SecondaryBufferSize);

						DWORD BytesToWrite = 0;
						if(ByteToLock > TargetCursor)
						{
							BytesToWrite = SoundOutput.SecondaryBufferSize - ByteToLock;
							BytesToWrite += TargetCursor;
						}
						else
						{
							BytesToWrite = TargetCursor - ByteToLock;
						}

						game_sound_output_buffer SoundBuffer = {};
						SoundBuffer.Samples = Samples;
						SoundBuffer.SamplesPerSecond = SoundOutput.SamplesPerSecond;
						SoundBuffer.SampleCount = BytesToWrite / SoundOutput.BytesPerSample;
						GameGetSoundSamples(&GameMemory, &SoundBuffer);

						DWORD PlayCursor;
						DWORD WriteCursor;
						GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor);

						WinFillSoundBuffer(&SoundOutput, ByteToLock, BytesToWrite, &SoundBuffer);
					}
					else
					{
						SoundIsValid = false;
					}

					END_BLOCK(SoundUpdateTime);


	#if VOXEL_ENGINE_INTERNAL
					BEGIN_BLOCK(DebugStuffTime);

					DEBUGEndDebugFrameAndRender(&GameMemory, Input, (r32)Dimension.Width, (r32)Dimension.Height);

					END_BLOCK(DebugStuffTime);
	#endif

					BEGIN_BLOCK(UpdateWindowTime);

					HDC DeviceContext = GetDC(Window);
					WinUpdateWindow(DeviceContext, Dimension.Width, Dimension.Height);
					ReleaseDC(Window, DeviceContext);

					END_BLOCK(UpdateWindowTime);			


#if 1
					BEGIN_BLOCK(SleepTime);

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

					END_BLOCK(SleepTime);
	#endif


					LARGE_INTEGER EndCounter = WinGetPerformanceCounter();
					r32 MSPerFrame = 1000.0f * WinGetSecondsElapsed(LastCounter, EndCounter);
					FRAME_MARKER(MSPerFrame);
					LastCounter = EndCounter;
				}
			}
		}
	}

	return(0);
}

#if VOXEL_ENGINE_INTERNAL

#include "voxel_engine_debug.hpp"

#endif
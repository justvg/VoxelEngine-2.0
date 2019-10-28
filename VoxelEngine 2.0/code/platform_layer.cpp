// TODO(georgy): Replace glew lib with my code
#include <gl\glew.h>
#include <gl\wglew.h>

#include "voxel_engine_platform.h"
#include "voxel_engine.hpp"

#include <Windows.h>
#include <timeapi.h>
#include <stdio.h>

global_variable bool8 GlobalRunning;
global_variable LARGE_INTEGER GlobalPerformanceFrequency;
global_variable bool8 GlobalCursorShouldBeClipped;

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

FREE_FILE_MEMORY(WinFreeFileMemory)
{
	VirtualFree(Memory, 0, MEM_RELEASE);
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
					WinFreeFileMemory(Result.Memory);
					Result.Memory = 0;
				}
			}
		}

		CloseHandle(FileHandle);
	}

	return(Result);
}

internal void
WinInitOpenGL(HWND Window, HINSTANCE Instance, LPCSTR WindowClassName)
{
	HWND FakeWindow = CreateWindowEx(0, WindowClassName,
									 "FakeWindow",
									 WS_OVERLAPPEDWINDOW | WS_VISIBLE,
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

internal void
WinUpdateWindow(HDC DeviceContext, int WindowWidth, int WindowHeight)
{
	glViewport(0, 0, WindowWidth, WindowHeight);
	glClearColor(1.0f, 0.0f, 1.0f, 0.0f);
	//glClear(GL_COLOR_BUFFER_BIT);
	SwapBuffers(DeviceContext);
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
				GlobalCursorShouldBeClipped = true;
			}
			else
			{
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

			WinUpdateWindow(DeviceContext, Dimension.Width, Dimension.Height);
			EndPaint(Window, &Paint);
		} break;

		default:
		{
			Result = DefWindowProc(Window, Message, WParam, LParam);
		} break;
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

struct platform_job_system_entry
{
	platform_job_system_callback *Callback;
	void *Data;	
};
struct platform_job_system_queue
{
	u32 volatile EntryToRead;
	u32 volatile EntryToWrite;
	HANDLE Semaphore;
	
	platform_job_system_entry Entries[128];
};

internal void
WinAddEntry(platform_job_system_queue *JobSystem, platform_job_system_callback *Callback, void *Data)
{
	u32 NewEntryToWrite = (JobSystem->EntryToWrite + 1) % ArrayCount(JobSystem->Entries);
	Assert(NewEntryToWrite != JobSystem->EntryToRead);

	platform_job_system_entry *Entry = JobSystem->Entries + JobSystem->EntryToWrite;
	Entry->Callback = Callback;
	Entry->Data = Data;

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
		}
	}
	else
	{
		Sleep = true;
	}

	return(Sleep);
}

inline void
WinCompleteAllWork(platform_job_system_queue *JobSystem)
{
	for(;;)
	{
		if(WinDoNextJobQueueEntry(JobSystem))
		{
			break;
		}
	}
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

int CALLBACK
WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CommandLine, int ShowCode)
{	
	platform_job_system_queue JobSystem;
	WinInitializeJobSystem(&JobSystem, 3);

	QueryPerformanceFrequency(&GlobalPerformanceFrequency);

	UINT DesiredSchedulerMS = 1;
	bool32 SleepIsGranular = (timeBeginPeriod(DesiredSchedulerMS) == TIMERR_NOERROR);

	WNDCLASS WindowClass = {};
	WindowClass.style = CS_OWNDC | CS_VREDRAW | CS_HREDRAW;
	WindowClass.lpfnWndProc = WinWindowCallback;
	WindowClass.hInstance = Instance;
	WindowClass.lpszClassName = "VoxelEngineWindowClass";

	if (RegisterClass(&WindowClass))
	{
		HWND Window = CreateWindowEx(0, WindowClass.lpszClassName,
									 "Voxel Engine 2.0", 
									 WS_OVERLAPPEDWINDOW | WS_VISIBLE,
									 CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
									 0, 0, Instance, 0);
		if (Window)
		{
			WinInitOpenGL(Window, Instance, WindowClass.lpszClassName);

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
#if 0
			GameMemory.PermanentStorageSize = Gigabytes(1);
			GameMemory.TemporaryStorageSize = Gigabytes(2);
			GameMemory.PermanentStorage = VirtualAlloc(0, GameMemory.PermanentStorageSize + GameMemory.TemporaryStorageSize, 
													   MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
			GameMemory.TemporaryStorage = (u8 *)GameMemory.PermanentStorage + GameMemory.PermanentStorageSize;
#else
			GameMemory.PermanentStorageSize = Gigabytes(1);
			GameMemory.PermanentStorage = VirtualAlloc(0, GameMemory.PermanentStorageSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
			GameMemory.TemporaryStorageSize = Gigabytes(2);
			GameMemory.TemporaryStorage = VirtualAlloc(0, GameMemory.TemporaryStorageSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#endif
			GameMemory.JobSystemQueue = &JobSystem;

			GameMemory.PlatformAddEntry = WinAddEntry;
			GameMemory.PlatformCompleteAllWork = WinCompleteAllWork;
			GameMemory.PlatformReadEntireFile = WinReadEntireFile;
			GameMemory.PlatformFreeFileMemory = WinFreeFileMemory;
			GameMemory.PlatformAllocateMemory = WinAllocateMemory;
			GameMemory.PlatformFreeMemory = WinFreeMemory;
			GameMemory.PlatformOutputDebugString = WinOutputDebugString;

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
				if (GlobalCursorShouldBeClipped)
				{
					RECT WindowRect, ClientRect;
					GetWindowRect(Window, &WindowRect);
					LONG WindowRectHeight = WindowRect.bottom - WindowRect.top;
					GetClientRect(Window, &ClientRect);
					LONG ClientRectHeight = ClientRect.bottom - ClientRect.top;
					LONG Diff = WindowRectHeight - ClientRectHeight;
					WindowRect.top += Diff;
					ClipCursor(&WindowRect);
				}
				else
				{
					ClipCursor(0);
				}

				GameInput.MouseXDisplacement = GameInput.MouseYDisplacement = 0;
				GameInput.MouseRight = GameInput.MouseLeft = false;

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
									GameInput.MoveForward = IsDown;
								}
								if (KeyCode == 'S')
								{
									GameInput.MoveBack = IsDown;
								}
								if (KeyCode == 'D')
								{
									GameInput.MoveRight = IsDown;
								}
								if (KeyCode == 'A')
								{
									GameInput.MoveLeft = IsDown;
								}
								if (KeyCode == VK_SPACE)
								{
									GameInput.MoveUp = IsDown;
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
								GameInput.MouseXDisplacement = RawInput.data.mouse.lLastX;
								GameInput.MouseYDisplacement = RawInput.data.mouse.lLastY;
								GameInput.MouseX += RawInput.data.mouse.lLastX;
								GameInput.MouseY += RawInput.data.mouse.lLastY;
								
								GameInput.MouseRight = RawInput.data.mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_DOWN;
								GameInput.MouseLeft= RawInput.data.mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_DOWN;
							}
						} break;
						
						default:
						{
							TranslateMessage(&Message);
							DispatchMessage(&Message);
						} break;
					}
				}

				window_dimension Dimension = WinGetWindowDimension(Window);
				GameUpdate(&GameMemory, &GameInput, Dimension.Width, Dimension.Height);
				
				r32 SecondsElapsedForFrame = WinGetSecondsElapsed(LastCounter, WinGetPerformanceCounter());
#if 0
				while (SecondsElapsedForFrame < TargetSecondsPerFrame)
				{
					SecondsElapsedForFrame = WinGetSecondsElapsed(LastCounter, WinGetPerformanceCounter());
				}
#else
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
				LARGE_INTEGER EndCounter = WinGetPerformanceCounter();
				r32 MSPerFrame = 1000.0f*WinGetSecondsElapsed(LastCounter, EndCounter);
				LastCounter = EndCounter;

				HDC DeviceContext = GetDC(Window);
				WinUpdateWindow(DeviceContext, Dimension.Width, Dimension.Height);
				ReleaseDC(Window, DeviceContext);
				
#if 0
				char MSBuffer[256];
				_snprintf_s(MSBuffer, sizeof(MSBuffer), "%.02fms/f\n", MSPerFrame);
				OutputDebugString(MSBuffer);
#endif
			}
		}
	}

	return(0);
}
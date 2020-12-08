#include "gl\glew.h"
#include "gl\wglew.h"
#include "glfw3.h"

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
global_variable LARGE_INTEGER GlobalPerformanceFrequency;
global_variable WINDOWPLACEMENT GlobalWindowPosition = { sizeof(GlobalWindowPosition) };
global_variable i32 GlobalWindowWidth;
global_variable i32 GlobalWindowHeight;

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

struct glfw_callback_struct
{
	win_state *WinState;
	game_input *Input;
	game_memory *GameMemory;
};
static void
GLFW_KeyCallback(GLFWwindow *Window, int Key, int ScanCode, int Action, int Mods)
{
	glfw_callback_struct *CallbackInfo = (glfw_callback_struct *)glfwGetWindowUserPointer(Window);

	win_state *WinState = CallbackInfo->WinState;
	game_input *Input = CallbackInfo->Input;
	game_memory *GameMemory = CallbackInfo->GameMemory;
	
	if(Key == GLFW_KEY_W)
	{
		WinProcessKey(&Input->MoveForward, Action);
	}
	if(Key == GLFW_KEY_S)
	{
		WinProcessKey(&Input->MoveBack, Action);
	}
	if(Key == GLFW_KEY_A)
	{
		WinProcessKey(&Input->MoveLeft, Action);
	}
	if(Key == GLFW_KEY_D)
	{
		WinProcessKey(&Input->MoveRight, Action);
	}
	if(Key == GLFW_KEY_SPACE)
	{
		WinProcessKey(&Input->MoveUp, Action);
	}
	if(Key == GLFW_KEY_ESCAPE)
	{
		WinProcessKey(&Input->Esc, Action);
	}

	if(Key == GLFW_KEY_P)
	{
		// TODO(georgy): I don't want the pause button to be _game_ input
		WinProcessKey(&Input->Pause, Action);
		// if(IsDown)
		// {
		// 	GlobalGamePause = !GlobalGamePause;
		// 	GameInput.dt = GlobalGamePause ? 0.0f : TargetSecondsPerFrame;
		// 	GlobalDebugTable.ProfilePause = !GlobalDebugTable.ProfilePause || GlobalGamePause;
		// }
	}
	if(Key == GLFW_KEY_L)
	{
		if(Action == GLFW_PRESS)
		{
			if(!WinState->InputPlayback)
			{
				WinCompleteAllWork(GameMemory->JobSystemQueue);
				if(!WinState->InputRecording)
				{
					WinBeginRecordingInput(WinState);
				}
				else
				{
					WinEndRecordingInput(WinState);
					WinBeginInputPlayback(WinState);
				}
			}
			else if(Action == GLFW_RELEASE)
			{
				WinEndInputPlayback(WinState);
			}
		}
	}

	if (Key == GLFW_KEY_2)
	{
		if(Action == GLFW_PRESS)
		{
			GlobalDEBUGCursor = !GlobalDEBUGCursor;
			int EnableCursor = GlobalDEBUGCursor ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED; 
			glfwSetInputMode(Window, GLFW_CURSOR, EnableCursor);
		}
	}
}

static void
GLFW_MouseButtonCallback(GLFWwindow *Window, int Button, int Action, int Mods)
{
	glfw_callback_struct *CallbackInfo = (glfw_callback_struct *)glfwGetWindowUserPointer(Window);

	game_input *Input = CallbackInfo->Input;

	if(Button == GLFW_MOUSE_BUTTON_1)
	{
		WinProcessKey(&Input->MouseLeft, Action);
	}
	if(Button == GLFW_MOUSE_BUTTON_2)
	{
		WinProcessKey(&Input->MouseRight, Action);
	}
}

static void
GLFW_CursorPosCallback(GLFWwindow *Window, double XPos, double YPos)
{
	glfw_callback_struct *CallbackInfo = (glfw_callback_struct *)glfwGetWindowUserPointer(Window);

	game_input *Input = CallbackInfo->Input;

	int BufferWidth, BufferHeight;
	glfwGetFramebufferSize(Window, &BufferWidth, &BufferHeight);

	XPos = (XPos - 0.5*GlobalWindowWidth);
	YPos = (0.5*GlobalWindowHeight - YPos);
	
	if(!GlobalDEBUGCursor)
	{
		Input->MouseXDisplacement = (i32)(XPos - Input->MouseX);
		Input->MouseYDisplacement = (i32)(YPos - Input->MouseY);
	}

	Input->MouseX = (r32)XPos;
	Input->MouseY = (r32)YPos;	
}

static void 
GLFW_WindowSizeCallback(GLFWwindow *Window, int Width, int Height)
{
	GlobalWindowWidth = Width;
	GlobalWindowHeight = Height;
}

int CALLBACK
WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CommandLine, int ShowCode)
{	
	win_state WinState = {};

	platform_job_system_queue JobSystem;
	WinInitializeJobSystem(&JobSystem, 5);

	DWORD ThreadID = GetCurrentThreadId();

	QueryPerformanceFrequency(&GlobalPerformanceFrequency);

	GlobalWindowWidth = 1366;
	GlobalWindowHeight = 768;

	if(glfwInit())
	{
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

		GLFWwindow *Window = glfwCreateWindow(GlobalWindowWidth, GlobalWindowHeight, "VoxelEngine2.0", 0, 0);
		if(Window)
		{
			glfwMakeContextCurrent(Window);
			glfwSwapInterval(1);

			glfwSetInputMode(Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

			if(glewInit() == GLEW_OK)
			{
				glEnable(GL_DEPTH_TEST);
				glEnable(GL_CULL_FACE);
				glEnable(GL_MULTISAMPLE);
				glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

				glClearColor(SquareRoot(0.5f), 0.0f, SquareRoot(0.5f), 1.0f);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

				GLFWmonitor *Monitor = glfwGetPrimaryMonitor();
				const GLFWvidmode *VidMode = glfwGetVideoMode(Monitor);
				i32 GameUpdateHz = VidMode->refreshRate;
				r32 TargetSecondsPerFrame = 1.0f / GameUpdateHz;

				game_memory GameMemory = {};
				GameMemory.PermanentStorageSize = Gigabytes(2);
				GameMemory.TemporaryStorageSize = Megabytes(128);
				GameMemory.DebugStorageSize = Megabytes(64);
				WinState.GameMemorySize = GameMemory.PermanentStorageSize + GameMemory.TemporaryStorageSize + GameMemory.DebugStorageSize;
				WinState.GameMemory = VirtualAlloc(0, WinState.GameMemorySize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
				GameMemory.PermanentStorage = WinState.GameMemory;
				GameMemory.TemporaryStorage = (u8 *)GameMemory.PermanentStorage + GameMemory.PermanentStorageSize;
				GameMemory.DebugStorage = (u8 *)GameMemory.TemporaryStorage + GameMemory.TemporaryStorageSize;

				win_sound_output SoundOutput = {};
				SoundOutput.SamplesPerSecond = 44100;
				SoundOutput.BytesPerSample = sizeof(i16)*2;
				SoundOutput.SecondaryBufferSize = SoundOutput.SamplesPerSecond* SoundOutput.BytesPerSample;
				SoundOutput.LatencySampleCount = 3*(SoundOutput.SamplesPerSecond / GameUpdateHz);
				SoundOutput.SafetyBytes = ((SoundOutput.SamplesPerSecond*SoundOutput.BytesPerSample) / GameUpdateHz) / 2;

				bool32 SoundIsValid = false;
				DWORD AudioLatencyBytes = 0;
				r32 AudioLatencySeconds = 0;

				i16 *Samples = (i16 *)VirtualAlloc(0, SoundOutput.SamplesPerSecond*SoundOutput.BytesPerSample, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
				WinInitDirectSound(GetForegroundWindow(), SoundOutput.SamplesPerSecond, SoundOutput.SecondaryBufferSize);
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

				if(GameMemory.PermanentStorage && GameMemory.TemporaryStorage)
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
					glfw_callback_struct GLFWCallbackStruct = {&WinState, &GameInput, &GameMemory};
					glfwSetWindowUserPointer(Window, &GLFWCallbackStruct);
					glfwSetKeyCallback(Window, GLFW_KeyCallback);
					glfwSetMouseButtonCallback(Window, GLFW_MouseButtonCallback);
					glfwSetCursorPosCallback(Window, GLFW_CursorPosCallback);
					glfwSetFramebufferSizeCallback(Window, GLFW_WindowSizeCallback);

					GameInput.dt = TargetSecondsPerFrame;

					game_input DebugCameraInput = {};

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

						game_input *Input;
						DEBUG_IF(DebugCamera, DebugTools)
						{
							Input = &DebugCameraInput;
						}				
						else
						{
							Input = &GameInput;
						}
						GLFWCallbackStruct.Input = Input;

						Input->MouseXDisplacement = Input->MouseYDisplacement = 0;
						Input->MouseRight.HalfTransitionCount = Input->MouseLeft.HalfTransitionCount = 0;

						for(u32 ButtonIndex = 0;
							ButtonIndex < ArrayCount(Input->Buttons);
							ButtonIndex++)
						{
							button *Button = Input->Buttons + ButtonIndex;
							Button->HalfTransitionCount = 0;
						}

						glfwPollEvents();

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

						GameUpdate(&GameMemory, &GameInput, GlobalWindowWidth, GlobalWindowHeight, GlobalGamePause, DebugCamera, &DebugCameraInput);
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

							DWORD ExpectedSoundBytesPerFrame = (SoundOutput.SamplesPerSecond*SoundOutput.BytesPerSample) / GameUpdateHz;
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

						DEBUGEndDebugFrameAndRender(&GameMemory, Input, (r32)GlobalWindowWidth, (r32)GlobalWindowHeight);

						END_BLOCK(DebugStuffTime);
	#endif

						BEGIN_BLOCK(UpdateWindowTime);

						glfwSwapBuffers(Window);

						END_BLOCK(UpdateWindowTime);			

						LARGE_INTEGER EndCounter = WinGetPerformanceCounter();
						r32 MSPerFrame = 1000.0f * WinGetSecondsElapsed(LastCounter, EndCounter);
						FRAME_MARKER(MSPerFrame);
						LastCounter = EndCounter;
					}
				}
			}
		}
	}

	return(0);
}

#if VOXEL_ENGINE_INTERNAL

#include "voxel_engine_debug.hpp"

#endif
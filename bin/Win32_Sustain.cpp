// Sustain.cpp : defines the entry point for the application
/*
	Things we need to work on later
	Save states
	handle to exe
	asset loading
	threading
	Raw inputs
	sleep/timeBeginPeriod
	clip cursor
	fullscreen support
	WM_SETCURSOR
	WM_ACTIVATEAPP
	bitblt
	Hardware ACCEL
	Keyboard Layouts
*/
//17 Dec 2016

#include <stdint.h>
#include <math.h>
#include <dsound.h>
#include <xinput.h>


#define local_persist static
#define global_variable static 
#define internal static
#define Pi32 3.14159265359f

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef int32 bool32;

typedef float real32;
typedef double real64;
#include "Sustain.cpp"
#include <malloc.h>
#include <stdio.h>
#include <windows.h>
#include "win32_Sustain.h"

//=============================================================================================================================================================================================================================================
//globals
//=============================================================================================================================================================================================================================================
global_variable bool GlobalRunning;
global_variable win32_Bitmap_buffer GlobalBitmapBuffer;
global_variable LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;
global_variable int XUSER_MAX_COUNT = 4; 

//=============================================================================================================================================================================================================================================
//XINPUT defines
//=============================================================================================================================================================================================================================================
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
//typedef DWORD WINAPI x_input_get_state(DWORD dwUserIndex, XINPUT_STATE *pState);
//typedef DWORD WINAPI x_input_set_state(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration);
typedef X_INPUT_GET_STATE(x_input_get_state);
typedef X_INPUT_SET_STATE(x_input_set_state);

X_INPUT_GET_STATE(XInputGetStateStub)
{
	return ERROR_DEVICE_NOT_CONNECTED;
}
X_INPUT_SET_STATE(XInputSetStateStub)
{
	return ERROR_DEVICE_NOT_CONNECTED;
}

global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;

#define XInputGetState XInputGetState_
#define XInputSetState XInputSetState_
#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);


//=============================================================================================================================================================================================================================================
//=============================================================================================================================================================================================================================================
//Functions
//=============================================================================================================================================================================================================================================
//=============================================================================================================================================================================================================================================
internal void DEBUGPlatformFreeFileMemory(void *Memory)
{
	if(Memory)
	{
		VirtualFree(Memory,0,MEM_RELEASE);
	}
}

internal debug_read_file_result DEBUGPlatformReadEntireFile(char *Filename)
{
	debug_read_file_result Result = {};
	HANDLE FileHandle = CreateFileA(Filename,GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,0,0);
	if(FileHandle != INVALID_HANDLE_VALUE)
	{
		LARGE_INTEGER FileSize;
		if(GetFileSizeEx(FileHandle,&FileSize))
		{
			//Assert(FileSize <= 0xFFFFFFFF);
			Result.Contents = VirtualAlloc(0,FileSize.QuadPart, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
			uint32 FileSize32 = SafeTruncateUInt64(FileSize.QuadPart);
			if(Result.Contents)
			{
				DWORD BytesRead;
				if(ReadFile(FileHandle,Result.Contents,FileSize32,&BytesRead,0) && FileSize32 == BytesRead)
				{
					Result.ContentsSize = FileSize32;
				}
				else
				{
					DEBUGPlatformFreeFileMemory(Result.Contents);
					Result.Contents = 0;
				}
			}
			else
			{
				//cout << "Unable to get allocate memory\n";
			}
		}
		else
		{
			//cout << "Unable to get File size\n";
		}
		CloseHandle(FileHandle);
	}
	else
	{
		//cout << "Unable to create File\n";
	}
	return Result;
}

internal bool32 DEBUGPlatformWriteEntireFile(char *Filename,void *Memory,uint32 MemorySize)
{
	bool32 Result = false;
	HANDLE FileHandle = CreateFileA(Filename,GENERIC_WRITE,0,0,CREATE_ALWAYS,0,0);
	if(FileHandle != INVALID_HANDLE_VALUE)
	{
		DWORD BytesWritten;
		if(WriteFile(FileHandle,Memory,MemorySize,&BytesWritten,0))
		{
				Result = (BytesWritten == MemorySize);
		}
		else
		{
			//cout << "unable to write file\n";
		}
		CloseHandle(FileHandle);
	}
	else
	{
		//cout << "Unable to create File\n";
	}
	return Result;
}
//=============================================================================================================================================================================================================================================
internal void Win32LoadXinput(void)
{
		// might not work for windows 8
		HMODULE XInputLibrary = LoadLibraryA("xinput1_4.dll");
		if(!XInputLibrary)
		{
			XInputLibrary = LoadLibraryA("xinput9_1_0.dll");
		}
		if(!XInputLibrary)
		{
			XInputLibrary = LoadLibraryA("xinput1_3.dll");
		}
		if(XInputLibrary)
		{
			XInputGetState = (x_input_get_state *)GetProcAddress(XInputLibrary,"XInputGetState");
			XInputSetState = (x_input_set_state *)GetProcAddress(XInputLibrary,"XInputSetState");
		}
}

internal void Win32ProccessXInputDigitalButton(DWORD XInputButtonState, game_button_state *OldState, DWORD ButtonBit,game_button_state *NewState)
{
	NewState->EndedDown = (XInputButtonState & ButtonBit) == ButtonBit;
	NewState->HalfTransitionCount = (OldState->EndedDown != NewState->EndedDown) ? 1 : 0;
}
//=============================================================================================================================================================================================================================================
internal void Win32InitSecondarySoundBuffer(win32_sound_output *SoundOutput)
{
	SoundOutput->SamplesPerSecond = 48000;
	SoundOutput->SecondaryBufferVolume = 3000;
	SoundOutput->RunningSampleIndex = 0;
	SoundOutput->BytesPerSample = sizeof(int16)*2;
	SoundOutput->SecondaryBufferSize = SoundOutput->SamplesPerSecond*SoundOutput->BytesPerSample;
	SoundOutput->LatencySampleCount = SoundOutput->SamplesPerSecond/15;
}
internal void Win32FillSecondarySoundBufferSin(win32_sound_output *SoundOutput, DWORD ByteToLock,DWORD BytesToWrite, game_sound_buffer *soundSource)
{
	VOID *Region1;
	DWORD Region1Size;
	VOID *Region2;
	DWORD Region2Size;
	if(SUCCEEDED(GlobalSecondaryBuffer->Lock(ByteToLock,BytesToWrite,&Region1,&Region1Size,&Region2,&Region2Size,0)))
	{
		DWORD Region1SampleCount = Region1Size/SoundOutput->BytesPerSample;
		int16 *DestSample = (int16 *)Region1;
		int16 *SourceSample = soundSource->Samples;
		for(DWORD SampleIndex = 0;SampleIndex < Region1SampleCount;++SampleIndex)
		{
			*DestSample++ = *SourceSample++;
			*DestSample++ = *SourceSample++;
			++SoundOutput->RunningSampleIndex;
		}

		DWORD Region2SampleCount = Region2Size/SoundOutput->BytesPerSample; 
		DestSample = (int16 *)Region2;
		for(DWORD SampleIndex = 0;SampleIndex < Region2SampleCount;++SampleIndex)
		{	
			*DestSample++ = *SourceSample++;
			*DestSample++ = *SourceSample++;
			++SoundOutput->RunningSampleIndex;
		}
		GlobalSecondaryBuffer->Unlock(Region1,Region1Size,Region2,Region2Size);
	}
}

internal void Win32ClearSoundBuffer(win32_sound_output *SoundOutput)
{
	VOID *Region1;
	DWORD Region1Size;
	VOID *Region2;
	DWORD Region2Size;
	if(SUCCEEDED(GlobalSecondaryBuffer->Lock(0,SoundOutput->SecondaryBufferSize,&Region1,&Region1Size,&Region2,&Region2Size,0)))
	{
		uint8 *DestSample = (uint8*)Region1;
		for(DWORD ByteIndex = 0;ByteIndex < Region1Size;++ByteIndex)
		{	
			*DestSample++ = 0;
		}
		DestSample = (uint8 *)Region2;
		for(DWORD ByteIndex = 0;ByteIndex < Region2Size;++ByteIndex)
		{	
			*DestSample++ = 0;
		}
	}
	GlobalSecondaryBuffer->Unlock(Region1,Region1Size,Region2,Region2Size);
}
//=============================================================================================================================================================================================================================================
internal void Win32InitDSound(HWND WindowHandle, int32 SamplesPerSecond, int32 BufferSize)
{
	HMODULE DSoundLibrary = LoadLibraryA("dsound.dll");
	int i = 0;
	//Load the library
	if(DSoundLibrary)
	{
		direct_sound_create *DirectSoundCreate = (direct_sound_create*)GetProcAddress(DSoundLibrary,"DirectSoundCreate");
		LPDIRECTSOUND DirectSound;
		if(DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0,&DirectSound,0)))
		{
			WAVEFORMATEX WaveFormat = {};
			WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
			WaveFormat.nChannels = 2;
			WaveFormat.nSamplesPerSec = SamplesPerSecond;
			WaveFormat.wBitsPerSample = 16;
			WaveFormat.nBlockAlign = (WaveFormat.nChannels*WaveFormat.wBitsPerSample)/8;
			WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec*WaveFormat.nBlockAlign;
			WaveFormat.cbSize = 0;
			// Get a direct sound object
			if(SUCCEEDED(DirectSound->SetCooperativeLevel(WindowHandle,DSSCL_PRIORITY)))
			{
				// Create a primary buffer
				DSBUFFERDESC BufferDescription = {};
				BufferDescription.dwSize = sizeof(BufferDescription);
				BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;

				LPDIRECTSOUNDBUFFER PrimaryBuffer;
				if(SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription,&PrimaryBuffer,0)))
				{
					if(SUCCEEDED(PrimaryBuffer->SetFormat(&WaveFormat)))
					{
			//OutputDebugStringA("PrimaryBuffer Set");
			//Never going to use this buffer, don't worry about it?
			//but needs to be here
			// provides handle to sound card
					}
					else
					{
			//more diagnostics
					}
				}
				else
				{
				}
				// secondary
			}
			else
			{
				//diagnostics
			}
			DSBUFFERDESC BufferDescription = {};
			BufferDescription.dwSize = sizeof(BufferDescription);
			BufferDescription.dwFlags = 0;
			BufferDescription.dwBufferBytes =  BufferSize;
			BufferDescription.lpwfxFormat = &WaveFormat;
			if(SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription,&GlobalSecondaryBuffer,0)))
			{
				// start playing
			}
			else
			{
					//diagnostics
			}
		}
		else
		{
			//more logging diagnostics
		}
	}
	else
	{
		//more logging
	}
}
//=============================================================================================================================================================================================================================================
internal win32_window_dimension GetWindowDimension(HWND Window)
{
	win32_window_dimension Result;
	RECT ClientRect;
	GetClientRect(Window,&ClientRect);
	Result.Width = ClientRect.right - ClientRect.left;
	Result.Height = ClientRect.bottom - ClientRect.top;
	return Result;
}
//=============================================================================================================================================================================================================================================


//=============================================================================================================================================================================================================================================
internal void Win32ResizeDIBSection(win32_Bitmap_buffer *Buffer,int Width,int Height)
{	
	if(Buffer->Memory)
	{
			VirtualFree(Buffer->Memory,0,MEM_RELEASE);
	}
	int BytesPerPixel = 4;
	Buffer->Width = Width;
	Buffer->Height = Height;
	Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
	Buffer->Info.bmiHeader.biWidth = Buffer->Width;
	Buffer->Info.bmiHeader.biHeight = -Buffer->Height;
	Buffer->Info.bmiHeader.biPlanes = 1;
	Buffer->Info.bmiHeader.biBitCount = 32;
	Buffer->Info.bmiHeader.biCompression = BI_RGB;
	//Buffer->Info.bmiColors[] = ; for palletes
	int BitmapMemorySize = (Buffer->Width*Buffer->Height)*BytesPerPixel;
	Buffer->Memory = VirtualAlloc(0,BitmapMemorySize, MEM_RESERVE| MEM_COMMIT,PAGE_READWRITE);
	Buffer->Pitch = Buffer->Width*BytesPerPixel;
}
//=============================================================================================================================================================================================================================================
internal void Win32UpdateBufferWindow(win32_Bitmap_buffer *Buffer,HDC DC, int WindowWidth, int WindowHeight)
{
	StretchDIBits(DC,//X,Y,Width,Height, X,Y,Width,Height,
					0,0,WindowWidth,WindowHeight,
					0,0,Buffer->Width,Buffer->Height,
				  	Buffer->Memory,&Buffer->Info,DIB_RGB_COLORS,SRCCOPY);
}

//=============================================================================================================================================================================================================================================
internal LRESULT CALLBACK Win32MainWindowCallback(HWND Window,UINT Msg,WPARAM wParam, LPARAM lParam)
{
	LRESULT Result = 0;
	switch(Msg){
		case WM_SIZE:
		{
			//win32_window_dimension Dimension = GetWindowDimension(Window);
			//Win32ResizeDIBSection(&GlobalBitmapBuffer,Dimension.Width,Dimension.Height);
			//OutputDebugStringA("WM_Size\n");
		} break;
		case WM_DESTROY:
		{
			GlobalRunning = false;
			//OutputDebugStringA("WM_DESTROY\n");
		} break;
		case WM_CLOSE:
		{
		GlobalRunning = false;
			//OutputDebugStringA("WM_CLOSE\n");
		} break;
		case WM_ACTIVATEAPP:
		{
			//OutputDebugStringA("WM_ACTIVATEAPP\n");
		}break;
		case WM_PAINT:
		{
			PAINTSTRUCT Paint;
			win32_window_dimension Dimension = GetWindowDimension(Window);
			HDC DC = BeginPaint(Window,&Paint);
			Win32UpdateBufferWindow(&GlobalBitmapBuffer,DC,Dimension.Width,Dimension.Height);
			//PatBlt(DC,X,Y,Width,Height,WHITENESS);
			EndPaint(Window,&Paint);
		}break;
		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
		case WM_KEYDOWN:
		case WM_KEYUP:
		{
			uint32 VKCode = wParam;
			bool32 WasDown = (lParam & (1 << 30));
			bool32 IsDown = (lParam & (1 << 31));
			if(WasDown != IsDown)
			{
				if(VKCode == 'W')
				{
					//OutputDebugStringA("W\n");	
				}
				else if(VKCode == 'A')
				{
					//OutputDebugStringA("A\n");	
				}
				else if(VKCode == 'S')
				{
					//OutputDebugStringA("S\n");	
				}
				else if(VKCode == 'D')
				{
					//OutputDebugStringA("W\n");	
				}
				else if(VKCode == 'Q')
				{
					//OutputDebugStringA("W\n");	
				}
				else if(VKCode == 'E')
				{
					//OutputDebugStringA("W\n");	
				}
				else if(VKCode == VK_UP)
				{
					//OutputDebugStringA("W\n");	
				}
				else if(VKCode == VK_DOWN)
				{
					//OutputDebugStringA("W\n");	
				}
				else if(VKCode == VK_LEFT)
				{
					//OutputDebugStringA("W\n");	
				}
				else if(VKCode == VK_RIGHT)
				{
					//OutputDebugStringA("W\n");	
				}
				else if(VKCode == VK_ESCAPE)
				{
					if(IsDown){
			//OutputDebugStringA("IsDown\n");
					}
					if(WasDown)
					{
						//OutputDebugStringA("WasDown\n");
					}
				}
				else if(VKCode == VK_SPACE)
				{

				}
				//May add more...
			}
			bool32 AltKeyWasDown = ((lParam & (1 << 29)) != 0);
			if((VKCode == VK_F4) && AltKeyWasDown)
			{
				GlobalRunning = false;
			}
		}break;
		default:
		{
			//OutputDebugStringA("default\n");
			Result = DefWindowProcA(Window,Msg,wParam,lParam);
		}break;
	}
	return(Result);
}
//=============================================================================================================================================================================================================================================
//=============================================================================================================================================================================================================================================
//=============================================================================================================================================================================================================================================
// MAIN
//=============================================================================================================================================================================================================================================
//=============================================================================================================================================================================================================================================
//=============================================================================================================================================================================================================================================

int CALLBACK WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR lpCmdLine,int nCmdShow)
{
	//initalize======================
	LARGE_INTEGER PerformanceFrequencyResult;
	QueryPerformanceFrequency(&PerformanceFrequencyResult);
	int64 PerfCountFrequency = PerformanceFrequencyResult.QuadPart;
	//================================================================
	//console for debugging
	//TODO :: REMOVE Console + debug strings after shipping
//#if 0
	//AllocConsole();
	//HANDLE out_console = GetStdHandle(STD_OUTPUT_HANDLE);
//#endif
	//================================================================

	Win32LoadXinput();
	WNDCLASSA WindowClass = {};
	Win32ResizeDIBSection(&GlobalBitmapBuffer,1280,720);
  	WindowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
  	WindowClass.lpfnWndProc = Win32MainWindowCallback;
  	WindowClass.hInstance = hInstance;
  	//WindowClass.hIcon = ;
  	WindowClass.lpszClassName = "Sustain";

	//===============================

  	
	if(RegisterClassA(&WindowClass))
	{
		HWND WindowHandle = CreateWindowExA(0,WindowClass.lpszClassName,"Sustain",WS_OVERLAPPEDWINDOW| WS_VISIBLE,CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,0,0,hInstance,0);
		if(WindowHandle)
		{
			HDC WindowDC =  GetDC(WindowHandle);
			//Graphics Test
			//Sound Test
			win32_sound_output SoundOut = {};
			Win32InitSecondarySoundBuffer(&SoundOut);

			Win32InitDSound(WindowHandle,SoundOut.SamplesPerSecond,SoundOut.SecondaryBufferSize);
			Win32ClearSoundBuffer(&SoundOut);
			GlobalSecondaryBuffer->Play(0,0,DSBPLAY_LOOPING);

			GlobalRunning = true;
			int16 *Samples = (int16 *) VirtualAlloc(0,SoundOut.SecondaryBufferSize,MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

#if SUSTAIN_INTERNAL
	LPVOID BaseAdress = (LPVOID)Terabytes(1);
#else
	LPVOID BaseAdress = 0;
#endif


			game_memory GameMemory = {};
			GameMemory.PermanentStorageSize = Megabytes(64);
			GameMemory.TransientStorageSize = Gigabytes(1);

			uint64 TotalSize = GameMemory.PermanentStorageSize + GameMemory.TransientStorageSize;

			GameMemory.PermanentStorage = VirtualAlloc(BaseAdress,GameMemory.PermanentStorageSize, MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
			GameMemory.TransientStorage = VirtualAlloc(BaseAdress,GameMemory.TransientStorageSize, MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
			//((uint8 *)GameMemory.PermanentStorage + GameMemory.PermanentStorageSize);
			if(Samples && GameMemory.PermanentStorage)
			{
				game_input gameInput[2];
				game_input *NewInput = &gameInput[0];
				game_input *OldInput = &gameInput[1];

				LARGE_INTEGER LastCounter;
				QueryPerformanceCounter(&LastCounter);
				uint64 LastCycleCount = __rdtsc();
				while(GlobalRunning)
				{

					MSG Message;

					while(PeekMessage(&Message,0,0,0,PM_REMOVE))
					{
						if(Message.message == WM_QUIT)
						{
							GlobalRunning = false;
						}
						TranslateMessage(&Message);
						DispatchMessageA(&Message);
					}
					//Controller Polling
					int MaxControllerCount = XUSER_MAX_COUNT;
					if(MaxControllerCount > ArrayCount(NewInput->controller))
					{
						MaxControllerCount  = ArrayCount(NewInput->controller);
					}

					for(DWORD ControllerIndex = 0; ControllerIndex < 4;++ControllerIndex)
					{
						game_controller_input *OldController = &OldInput->controller[ControllerIndex];
						game_controller_input *NewController = &NewInput->controller[ControllerIndex];
						XINPUT_STATE ControllerState;
						if(XInputGetState(ControllerIndex,&ControllerState))
						{
							// controller is plugged in
							XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;
							bool32 DPadUp = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
							bool32 DPadDown = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
							bool32 DPadLeft = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
							bool32 DPadRight = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
							bool32 GamePadStart = (Pad->wButtons & XINPUT_GAMEPAD_START);
							bool32 GamepadBack = (Pad->wButtons & XINPUT_GAMEPAD_BACK);
							bool32 GamePadLShoulder = (Pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
							bool32 GamePadRShoulder = (Pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
							
							NewController->IsAnalog = true;
							NewController->StartX = OldController->EndX;
							NewController->StartY = OldController->EndY;

							real32 X;
							if(Pad->sThumbLX < 0)
							{
								X = (real32)Pad->sThumbLX/32768.0f;
							}
							else
							{
								X = (real32)Pad->sThumbLX/32767.0f;
							}
							NewController->MinX = NewController->MaxX = NewController->EndX = X;
							real32 Y;
							
							if(Pad->sThumbLY < 0)
							{
								Y = (real32)Pad->sThumbLY/32768.0f;
							}
							else
							{
								Y = (real32)Pad->sThumbLY/32767.0f;
							}
							NewController->MinY = NewController->MaxY = NewController->EndY = Y;


							Win32ProccessXInputDigitalButton(Pad->wButtons,&OldController->Down,XINPUT_GAMEPAD_A,&NewController->Down);
							Win32ProccessXInputDigitalButton(Pad->wButtons,&OldController->Right,XINPUT_GAMEPAD_B,&NewController->Right);
							Win32ProccessXInputDigitalButton(Pad->wButtons,&OldController->Left,XINPUT_GAMEPAD_X,&NewController->Left);
							Win32ProccessXInputDigitalButton(Pad->wButtons,&OldController->Up,XINPUT_GAMEPAD_Y,&NewController->Up);
							Win32ProccessXInputDigitalButton(Pad->wButtons,&OldController->LeftShoulder,XINPUT_GAMEPAD_LEFT_SHOULDER,&NewController->LeftShoulder);
							Win32ProccessXInputDigitalButton(Pad->wButtons,&OldController->RightShoulder,XINPUT_GAMEPAD_RIGHT_SHOULDER,&NewController->RightShoulder);
						}
						else
						{
							// controller error or not plugged in
						}
					}
					//BOOL MessageResult = GetMessageA(&Message,0,0,0);
					//=============================================================================================================================================================================================================================
					//=============================================================================================================================================================================================================================
					//=============================================================================================================================================================================================================================
					// Game Update Graphics
					//SpecialRenderOffsetPattern(&GlobalBitmapBuffer,xoff,yoff);
					DWORD PlayCursor;
					DWORD WriteCursor;
					DWORD ByteToLock;
					DWORD TargetCursor;
					DWORD BytesToWrite;
					bool32 SoundIsValid = false;
					if(SUCCEEDED(GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor,&WriteCursor)))
					{
						ByteToLock = (SoundOut.RunningSampleIndex*SoundOut.BytesPerSample) % SoundOut.SecondaryBufferSize;
						TargetCursor = (PlayCursor + (SoundOut.LatencySampleCount*SoundOut.BytesPerSample)) % SoundOut.SecondaryBufferSize;
						if(ByteToLock > TargetCursor)
						{
							BytesToWrite = (SoundOut.SecondaryBufferSize - ByteToLock);
							BytesToWrite += TargetCursor;
						}
						else
						{
							BytesToWrite = TargetCursor - ByteToLock;
						}
						SoundIsValid = true;
					}
					
					game_sound_buffer gameSound = {};
					gameSound.SamplesPerSecond = SoundOut.SamplesPerSecond;
					gameSound.SampleCount = BytesToWrite/ SoundOut.BytesPerSample;
					gameSound.Samples = Samples;

					game_offscreen_buffer gameBuffer;
					gameBuffer.Memory = GlobalBitmapBuffer.Memory;
					gameBuffer.Width = GlobalBitmapBuffer.Width;
					gameBuffer.Height = GlobalBitmapBuffer.Height;
					gameBuffer.Pitch = GlobalBitmapBuffer.Pitch;


					GameUpdate(&GameMemory,NewInput,&gameBuffer,&gameSound);
					//=============================================================================================================================================================================================================================
					//=============================================================================================================================================================================================================================
					//=============================================================================================================================================================================================================================

					//Direct Sound output test
					if(SoundIsValid)
					{
						//=========================================================================================================================================================================================================================
						//=========================================================================================================================================================================================================================
						//=========================================================================================================================================================================================================================
						Win32FillSecondarySoundBufferSin(&SoundOut,ByteToLock,BytesToWrite,&gameSound);
						//Update Game Sound
						//=========================================================================================================================================================================================================================
						//=========================================================================================================================================================================================================================
						//=========================================================================================================================================================================================================================
					}
					win32_window_dimension Dimension = GetWindowDimension(WindowHandle);
					Win32UpdateBufferWindow(&GlobalBitmapBuffer,WindowDC,Dimension.Width,Dimension.Height);
					//======== rumble example ========
					/*
					XINPUT_VIBRATION Vibration;
					Vibration.wLeftMotorSpeed = 60000;
					Vibration.wRightMotorSpeed = 60000;
					XInputSetState(0,&Vibration);
					*/
					//================================
					game_input *Temp  = NewInput;
					NewInput = OldInput;
					OldInput = Temp;


					LARGE_INTEGER EndCounter;
					QueryPerformanceCounter(&EndCounter);
					// Print value of counter
					uint64 EndCycleCount = __rdtsc();
					uint64 CyclesElapsed =EndCycleCount - LastCycleCount;
					int64 CounterElapsed = EndCounter.QuadPart - LastCounter.QuadPart;
					real32 MSPerFrame = ((1000.0f*(real32)CounterElapsed)/(real32)PerfCountFrequency);
					real32 FPS = (real32)PerfCountFrequency/(real32)CounterElapsed;
					real32 MCPF = (real32)(CyclesElapsed/(1000.0f*1000.0f));
					//======================================================================================================
					// TODO :: Take out this debug code + console
				#if 0
					char Buffer[256];
					//OutputDebugStringA(Buffer);
					snprintf(Buffer,sizeof(Buffer),"Milliseconds/frame:%.02fms/f %.02fFPS %.02fmc/f\n",MSPerFrame,FPS, MCPF);
					VOID* consoleReserve;
					DWORD* written = 0;
					DWORD write = strlen(Buffer);
				#endif
					//======================================================================================================
					LastCounter = EndCounter;
					LastCycleCount = EndCycleCount;
				}
			}
		}
		else
		{
			//logging
		}
	}
	else
	{
		//logging
	}
	//MessageBoxA(0, "Sustain","Sustain Title", MB_OK|MB_ICONINFORMATION);
	return 0;
}
//=============================================================================================================================================================================================================================================
//=============================================================================================================================================================================================================================================
//=============================================================================================================================================================================================================================================
// MAIN
//=============================================================================================================================================================================================================================================
//=============================================================================================================================================================================================================================================
//=============================================================================================================================================================================================================================================

// Sustain.cpp : defines the entry point for the application
//17 Dec 2016
#include <windows.h>
#include <stdint.h>
#include <xinput.h>
#include <dsound.h>
#include <math.h>
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
//================================================================================================================================================================================================
//Structures
//================================================================================================================================================================================================
struct win32_Bitmap_buffer
{
	BITMAPINFO Info;
	void *Memory;
	int Width;
	int Height;
	int Pitch;
};

struct win32_window_dimension
{
	int Width;
	int Height;
};
struct win32_sound_output
{
	int SamplesPerSecond;
	int Hz;
	int16 SecondaryBufferVolume;
	uint32 RunningSampleIndex;
	int BytesPerSample;
	int WavePeriod;
	int SecondaryBufferSize = SamplesPerSecond*BytesPerSample;
	real32 tSine;
	int LatencySampleCount;
};

//================================================================================================================================================================================================
//globals
//================================================================================================================================================================================================
global_variable bool GlobalRunning;
global_variable win32_Bitmap_buffer GlobalBitmapBuffer;
global_variable LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;

//================================================================================================================================================================================================
//defines
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

//================================================================================================================================================================================================
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
//================================================================================================================================================================================================
internal win32_sound_output Win32InitSecondarySoundBuffer()
{
	win32_sound_output SoundOutput = {};
	SoundOutput.SamplesPerSecond = 48000;
	SoundOutput.Hz = 256; //261.64 hz middle c
	SoundOutput.SecondaryBufferVolume = 3000;
	SoundOutput.RunningSampleIndex = 0;
	SoundOutput.BytesPerSample = sizeof(int16)*2;
	SoundOutput.WavePeriod =  SoundOutput.SamplesPerSecond/SoundOutput.Hz;
	SoundOutput.SecondaryBufferSize = SoundOutput.SamplesPerSecond*SoundOutput.BytesPerSample;
	SoundOutput.LatencySampleCount = SoundOutput.SamplesPerSecond/15;
	return SoundOutput;
}

internal void Win32FillSecondarySoundBufferSin(win32_sound_output *SoundOutput, DWORD ByteToLock,DWORD BytesToWrite)
{
	VOID *Region1;
	DWORD Region1Size;
	VOID *Region2;
	DWORD Region2Size;
	if(SUCCEEDED(GlobalSecondaryBuffer->Lock(ByteToLock,BytesToWrite,&Region1,&Region1Size,&Region2,&Region2Size,0)))
	{
		int16 *SampleOut = (int16*)Region1;
		DWORD Region1SampleCount = Region1Size/SoundOutput->BytesPerSample;
		
		for(DWORD SampleIndex = 0;SampleIndex < Region1SampleCount;++SampleIndex)
		{
			//real32 t = 2.0f*Pi32*(real32)SoundOutput->RunningSampleIndex++/(real32)SoundOutput->WavePeriod;
			//real32 SineValue = sinf(t);
			
			real32 SineValue = sinf(SoundOutput->tSine);
			int16 SampleValue = (int16)(SineValue*SoundOutput->SecondaryBufferVolume);
			*SampleOut++ = SampleValue;
			*SampleOut++ = SampleValue;
			SoundOutput->tSine += 2.0f*Pi32*1.0f/(real32)SoundOutput->WavePeriod;
			++SoundOutput->RunningSampleIndex;

		}

		SampleOut = (int16*)Region2;
		DWORD Region2SampleCount = Region2Size/SoundOutput->BytesPerSample; 
		for(DWORD SampleIndex = 0;SampleIndex < Region2SampleCount;++SampleIndex)
		{	
			//real32 t = 2.0f*Pi32*(real32)SoundOutput->RunningSampleIndex++/(real32)SoundOutput->WavePeriod;
			//real32 SineValue = sinf(t);

			real32 SineValue = sinf(SoundOutput->tSine);
			int16 SampleValue = (int16)(SineValue*SoundOutput->SecondaryBufferVolume);
			*SampleOut++ = SampleValue;
			*SampleOut++ = SampleValue;
			SoundOutput->tSine += 2.0f*Pi32*1.0f/(real32)SoundOutput->WavePeriod;
			++SoundOutput->RunningSampleIndex;
		}
		GlobalSecondaryBuffer->Unlock(Region1,Region1Size,Region2,Region2Size);
	}
}
//================================================================================================================================================================================================
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
						OutputDebugStringA("PrimaryBuffer Set");
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
//================================================================================================================================================================================================
internal win32_window_dimension GetWindowDimension(HWND Window)
{
	win32_window_dimension Result;
	RECT ClientRect;
	GetClientRect(Window,&ClientRect);
	Result.Width = ClientRect.right - ClientRect.left;
	Result.Height = ClientRect.bottom - ClientRect.top;
	return Result;
}
//================================================================================================================================================================================================
internal void SpecialRenderOffsetPattern(win32_Bitmap_buffer *Buffer,int XOffset, int YOffset)
{
	//int Pitch = Buffer->Width*Buffer->BytesPerPixel;
	uint8 *Row = (uint8*)Buffer->Memory;
	for(int Y = 0; Y < Buffer->Height;++Y)
	{
		uint32 *Pixel = (uint32 *)Row;
		for(int X = 0; X < Buffer->Width;++X)
		{
			//uint8 R = 0;
			uint8 G = (Y+ YOffset);
			uint8 B = (X + XOffset);
			*Pixel++ = ((G << 8) | B);
		}
		Row += Buffer->Pitch;
	}

}

//================================================================================================================================================================================================
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
//================================================================================================================================================================================================
internal void Win32UpdateBufferWindow(win32_Bitmap_buffer *Buffer,HDC DC, int WindowWidth, int WindowHeight)
{
	StretchDIBits(DC,//X,Y,Width,Height, X,Y,Width,Height,
					0,0,WindowWidth,WindowHeight,
					0,0,Buffer->Width,Buffer->Height,
				  	Buffer->Memory,&Buffer->Info,DIB_RGB_COLORS,SRCCOPY);
}

//================================================================================================================================================================================================
internal LRESULT CALLBACK Win32MainWindowCallback(HWND Window,UINT Msg,WPARAM wParam, LPARAM lParam)
{
	LRESULT Result = 0;
	switch(Msg){
		case WM_SIZE:
		{
			//win32_window_dimension Dimension = GetWindowDimension(Window);
			//Win32ResizeDIBSection(&GlobalBitmapBuffer,Dimension.Width,Dimension.Height);
			OutputDebugStringA("WM_Size\n");
		} break;
		case WM_DESTROY:
		{
			GlobalRunning = false;
			OutputDebugStringA("WM_DESTROY\n");
		} break;
		case WM_CLOSE:
		{
		GlobalRunning = false;
			OutputDebugStringA("WM_CLOSE\n");
		} break;
		case WM_ACTIVATEAPP:
		{
			OutputDebugStringA("WM_ACTIVATEAPP\n");
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
						OutputDebugStringA("IsDown\n");
					}
					if(WasDown)
					{
						OutputDebugStringA("WasDown\n");
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
//================================================================================================================================================================================================
//================================================================================================================================================================================================
//================================================================================================================================================================================================
// MAIN
//================================================================================================================================================================================================
//================================================================================================================================================================================================
//================================================================================================================================================================================================
int CALLBACK WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR lpCmdLine,int nCmdShow)
{
	//initalize======================
	Win32LoadXinput();
	//===============================

	WNDCLASSA WindowClass = {};
	Win32ResizeDIBSection(&GlobalBitmapBuffer,1280,720);
  	WindowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
  	WindowClass.lpfnWndProc = Win32MainWindowCallback;
  	WindowClass.hInstance = hInstance;
  	//WindowClass.hIcon = ;
  	WindowClass.lpszClassName = "Sustain";
	if(RegisterClass(&WindowClass))
	{
		HWND WindowHandle = CreateWindowExA(
					0,
					WindowClass.lpszClassName,
					"Sustain",
					WS_OVERLAPPEDWINDOW| WS_VISIBLE,
					CW_USEDEFAULT,
					CW_USEDEFAULT,
					CW_USEDEFAULT,
					CW_USEDEFAULT,
					0,
					0,
					hInstance,
					0);
		if(WindowHandle)
		{
			HDC WindowDC =  GetDC(WindowHandle);
			//Graphics Test
			int xoff = 0;
			int yoff = 0;

			//Sound Test
			win32_sound_output SoundOut = {};
			SoundOut = Win32InitSecondarySoundBuffer();
			Win32InitDSound(WindowHandle,SoundOut.SamplesPerSecond,SoundOut.SecondaryBufferSize);
			Win32FillSecondarySoundBufferSin(&SoundOut,0,SoundOut.LatencySampleCount*SoundOut.BytesPerSample);
			GlobalSecondaryBuffer->Play(0,0,DSBPLAY_LOOPING);
			bool32 SoundPlaying = true;
			GlobalRunning = true;
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
				for(DWORD ControllerIndex = 0; ControllerIndex < 4;++ControllerIndex)
				{
					XINPUT_STATE ControllerState;
					if(XInputGetState(ControllerIndex,&ControllerState))
					{
						// controller is plugged in
						//20:00
						XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;
						bool32 DPadUp = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
						bool32 DPadDown = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
						bool32 DPadLeft = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
						bool32 DPadRight = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
						bool32 GamePadStart = (Pad->wButtons & XINPUT_GAMEPAD_START);
						bool32 GamepadBack = (Pad->wButtons & XINPUT_GAMEPAD_BACK);
						bool32 GamePadLShoulder = (Pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
						bool32 GamePadRShoulder = (Pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
						bool32 GamepadA = (Pad->wButtons & XINPUT_GAMEPAD_A);
						bool32 GamepadB = (Pad->wButtons & XINPUT_GAMEPAD_B);
						bool32 GamepadX = (Pad->wButtons & XINPUT_GAMEPAD_X);
						bool32 GamepadY = (Pad->wButtons & XINPUT_GAMEPAD_Y);

						int16 StickX = Pad->sThumbLX;
						int16 StickY = Pad->sThumbLY;
						/*
							Gamepad Example
						if(GamepadA)
						{
							yoff++;
						}
						*/
						yoff += StickY/(8192);
						xoff += StickX/(8192);
						SoundOut.Hz = 512 + ((int)(256.0f*(real32)StickY/30000.0f));
						SoundOut.WavePeriod =  SoundOut.SamplesPerSecond/SoundOut.Hz;
					}
					else
					{
						// controller error or not plugged in
					}
				}
				//BOOL MessageResult = GetMessageA(&Message,0,0,0);
				SpecialRenderOffsetPattern(&GlobalBitmapBuffer,xoff,yoff);
				//Direct Sound output test
				DWORD PlayCursor;
				DWORD WriteCursor;
				if(/*!SoundPlaying && */SUCCEEDED(GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor,&WriteCursor)))
				{
					DWORD ByteToLock = (SoundOut.RunningSampleIndex*SoundOut.BytesPerSample) % SoundOut.SecondaryBufferSize;
					DWORD TargetCursor = (PlayCursor + (SoundOut.LatencySampleCount*SoundOut.BytesPerSample)) % SoundOut.SecondaryBufferSize;
					DWORD BytesToWrite;
				if(ByteToLock > TargetCursor)
					{
						BytesToWrite = (SoundOut.SecondaryBufferSize - ByteToLock);
						BytesToWrite += TargetCursor;
					}
					else
					{
						BytesToWrite = TargetCursor - ByteToLock;
					}
					Win32FillSecondarySoundBufferSin(&SoundOut,ByteToLock,BytesToWrite);
				}
				win32_window_dimension Dimension = GetWindowDimension(WindowHandle);
				Win32UpdateBufferWindow(&GlobalBitmapBuffer,WindowDC,Dimension.Width,Dimension.Height);
				//ReleaseDC(WindowHandle,WindowDC);
				//xoff++;
				//yoff++;
				//======== rumble example ========
				/*
				XINPUT_VIBRATION Vibration;
				Vibration.wLeftMotorSpeed = 60000;
				Vibration.wRightMotorSpeed = 60000;
				XInputSetState(0,&Vibration);
				*/
				//================================
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
//================================================================================================================================================================================================
//================================================================================================================================================================================================
//================================================================================================================================================================================================
// MAIN
//================================================================================================================================================================================================
//================================================================================================================================================================================================
//================================================================================================================================================================================================
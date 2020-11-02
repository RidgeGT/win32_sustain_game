// Sustain.cpp : defines the entry point for the application
//23 Nov 2016
#include <windows.h>
#include <stdint.h>
#include <xinput.h>

#define local_persist static
#define global_variable static 
#define internal static
typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
//================================================================================================================================================================================================
//globals
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
global_variable bool Running;
global_variable win32_Bitmap_buffer BitmapBuffer;

//================================================================================================================================================================================================
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
//typedef DWORD WINAPI x_input_get_state(DWORD dwUserIndex, XINPUT_STATE *pState);
//typedef DWORD WINAPI x_input_set_state(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration);
typedef X_INPUT_GET_STATE(x_input_get_state);
typedef X_INPUT_SET_STATE(x_input_set_state);

X_INPUT_GET_STATE(XInputGetStateStub)
{
	return 0;
}
X_INPUT_SET_STATE(XInputSetStateStub)
{
	return 0;
}

global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;

#define XInputGetState XInputGetState_
#define XInputSetState XInputSetState_

internal void Win32LoadXinput(void)
{
		HMODULE XInputLibrary = LoadLibraryA("xinput1_3.dll");
		if(XInputLibrary)
		{
			XInputGetState = (x_input_get_state *)GetProcAddress(XInputLibrary,"XInputGetState");
			XInputSetState = (x_input_set_state *)GetProcAddress(XInputLibrary,"XInputSetState");
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
	Buffer->Memory = VirtualAlloc(0,BitmapMemorySize,MEM_COMMIT,PAGE_READWRITE);
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
			//Win32ResizeDIBSection(&BitmapBuffer,Dimension.Width,Dimension.Height);
			OutputDebugStringA("WM_Size\n");
		} break;
		case WM_DESTROY:
		{
			Running = false;
			OutputDebugStringA("WM_DESTROY\n");
		} break;
		case WM_CLOSE:
		{
		Running = false;
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
			Win32UpdateBufferWindow(&BitmapBuffer,DC,Dimension.Width,Dimension.Height);
			//PatBlt(DC,X,Y,Width,Height,WHITENESS);
			EndPaint(Window,&Paint);
		}break;
		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
		case WM_KEYDOWN:
		case WM_KEYUP:
		{
			uint32 VKCode = wParam;
			bool WasDown = ((lParam & (1 << 30)) != 0);
			bool IsDown = ((lParam & (1 << 31)) == 0);
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
					//OutputDebugStringA("W\n");	
				}
				//May add more...
			}
		}break;
		default:
		{
			//OutputDebugStringA("default\n");
			Result = DefWindowProc(Window,Msg,wParam,lParam);
		}break;
	}
	return(Result);
}


//================================================================================================================================================================================================
// MAIN
//================================================================================================================================================================================================
int CALLBACK WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR lpCmdLine,int nCmdShow)
{
	//initalize======================
	Win32LoadXinput();
	//===============================

	WNDCLASSA WindowClass = {};
	Win32ResizeDIBSection(&BitmapBuffer,1280,720);
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
			int xoff = 0;
			int yoff = 0;
			Running = true;
			while(Running)
			{
				MSG Message;
				while(PeekMessage(&Message,0,0,0,PM_REMOVE))
				{
					if(Message.message == WM_QUIT)
					{
						Running = false;
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
						bool DPadUp = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
						bool DPadDown = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
						bool DPadLeft = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
						bool DPadRight = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
						bool GamePadStart = (Pad->wButtons & XINPUT_GAMEPAD_START);
						bool GamepadBack = (Pad->wButtons & XINPUT_GAMEPAD_BACK);
						bool GamePadLShoulder = (Pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
						bool GamePadRShoulder = (Pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
						bool GamepadA = (Pad->wButtons & XINPUT_GAMEPAD_A);
						bool GamepadB = (Pad->wButtons & XINPUT_GAMEPAD_B);
						bool GamepadX = (Pad->wButtons & XINPUT_GAMEPAD_X);
						bool GamepadY = (Pad->wButtons & XINPUT_GAMEPAD_Y);

						int16 StickX = Pad->sThumbLX;
						int16 StickY = Pad->sThumbLY;
						/*
							Gamepad Example
						if(GamepadA)
						{
							yoff++;
						}
						*/
					}
					else
					{
						// controller error or not plugged in
					}
				}
				//BOOL MessageResult = GetMessageA(&Message,0,0,0);
					SpecialRenderOffsetPattern(&BitmapBuffer,xoff,yoff);
					win32_window_dimension Dimension = GetWindowDimension(WindowHandle);
					Win32UpdateBufferWindow(&BitmapBuffer,WindowDC,Dimension.Width,Dimension.Height);
					//ReleaseDC(WindowHandle,WindowDC);
					xoff++;
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

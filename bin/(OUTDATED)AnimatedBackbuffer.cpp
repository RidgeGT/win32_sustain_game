// Sustain.cpp : defines the entry point for the application
//22 Nov 2016
#include <windows.h>
#include <stdint.h>

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

win32_window_dimension GetWindowDimension(HWND Window)
{
	win32_window_dimension Result;
	RECT ClientRect;
	GetClientRect(Window,&ClientRect);
	Result.Width = ClientRect.right - ClientRect.left;
	Result.Height = ClientRect.bottom - ClientRect.top;
	return Result;
}
//================================================================================================================================================================================================
internal void SpecialRenderOffsetPattern(win32_Bitmap_buffer Buffer,int XOffset, int YOffset)
{
	//int Pitch = Buffer.Width*Buffer.BytesPerPixel;
	uint8 *Row = (uint8*)Buffer.Memory;
	for(int Y = 0; Y < Buffer.Height;++Y)
	{
		uint32 *Pixel = (uint32 *)Row;
		for(int X = 0; X < Buffer.Width;++X)
		{
			//uint8 R = 0;
			uint8 G = (Y+ YOffset);
			uint8 B = (X + XOffset);
			*Pixel++ = ((G << 8) | B);
		}
		Row += Buffer.Pitch;
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
internal void Win32UpdateBufferWindow(win32_Bitmap_buffer Buffer,HDC DC, int WindowWidth, int WindowHeight)
{
	StretchDIBits(DC,//X,Y,Width,Height, X,Y,Width,Height,
					0,0,WindowWidth,WindowHeight,
					0,0,Buffer.Width,Buffer.Height,
				  	Buffer.Memory,&Buffer.Info,DIB_RGB_COLORS,SRCCOPY);
}

//================================================================================================================================================================================================
LRESULT CALLBACK Win32MainWindowCallback(HWND Window,UINT Msg,WPARAM wParam, LPARAM lParam)
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
			Win32UpdateBufferWindow(BitmapBuffer,DC,Dimension.Width,Dimension.Height);
			//PatBlt(DC,X,Y,Width,Height,WHITENESS);
			EndPaint(Window,&Paint);
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
	WNDCLASS WindowClass = {};
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
				//BOOL MessageResult = GetMessageA(&Message,0,0,0);
					SpecialRenderOffsetPattern(BitmapBuffer,xoff,yoff);
					win32_window_dimension Dimension = GetWindowDimension(WindowHandle);
					Win32UpdateBufferWindow(BitmapBuffer,WindowDC,Dimension.Width,Dimension.Height);
					//ReleaseDC(WindowHandle,WindowDC);
					xoff++;
					yoff++;
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

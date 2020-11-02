// Sustain.cpp : defines the entry point for the application
#include <windows.h>

#define local_persist static
#define global_variable static 
#define internal static
//globals
global_variable bool Running = true;
global_variable BITMAPINFO BitmapInfo;
global_variable void *BitmapMemory;
global_variable HBITMAP BitmapHandle;
global_variable HDC BitmapDC;
internal void Win32ResizeDIBSection(int Width,int Height)
{	

	BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader);
	BitmapInfo.bmiHeader.biWidth = Width;
	BitmapInfo.bmiHeader.biHeight = Height;
	BitmapInfo.bmiHeader.biPlanes = 1;
	BitmapInfo.bmiHeader.biBitCount = 32;
	BitmapInfo.bmiHeader.biCompression = BI_RGB;
	//BitmapInfo.bmiColors[] = ; for palletes

}

internal void Win32UpdateWindow(HDC DC, int X, int Y, int Width, int Height)
{
	StretchDIBits(DC,X,Y,Width,Height, X,Y,Width,Height, 
				  BitmapMemory,&BitmapInfo,DIB_RGB_COLORS,SRCCOPY);
}

LRESULT CALLBACK Win32MainWindowCallback(HWND Window,UINT Msg,WPARAM wParam, LPARAM lParam)
{
	LRESULT Result = 0;
	switch(Msg){
		case WM_SIZE:
		{
			RECT ClientRect;
			GetClientRect(Window,&ClientRect);
			int Width = ClientRect.right - ClientRect.left;
			int Height = ClientRect.bottom - ClientRect.top;
			Win32ResizeDIBSection(Width,Height);
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

			HDC DC = BeginPaint(Window,&Paint);
			int X = Paint.rcPaint.left;
			int Y = Paint.rcPaint.top;
			int Width = Paint.rcPaint.right - Paint.rcPaint.left;
			int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;
			
			Win32UpdateWindow(DC,X,Y,Width,Height);
			PatBlt(DC,X,Y,Width,Height,WHITENESS);
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

int CALLBACK WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR lpCmdLine,int nCmdShow)
{
	WNDCLASS WindowClass = {};
  	WindowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
  	WindowClass.lpfnWndProc = Win32MainWindowCallback;
  	WindowClass.hInstance = hInstance;
  	//WindowClass.hIcon = ;
  	WindowClass.lpszClassName = "Sustain";
	if(RegisterClass(&WindowClass))
	{
		HWND WindowHandle = CreateWindowEx(
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
			while(Running)
			{
        		MSG Message;
				BOOL MessageResult = GetMessage(&Message,0,0,0);
				if(MessageResult > 0)
				{
					TranslateMessage(&Message);
					DispatchMessage(&Message);
				}
				else
				{
					break;
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

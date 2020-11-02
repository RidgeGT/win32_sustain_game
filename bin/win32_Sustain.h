#if !defined(WIN32_SUSTAIN_H)

//#define Functions
//=========================================================================================================


//=========================================================================================================

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
	int SecondaryBufferSize = SamplesPerSecond*BytesPerSample;
	real32 tSine;
	int LatencySampleCount;
};
#define WIN32_SUSTAIN_H
#endif
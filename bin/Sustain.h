#if !defined(SUSTAIN_H)

#define Kilobytes(Value)(1024ULL*Value)
#define Megabytes(Value)(1024ULL*Kilobytes(Value))
#define Gigabytes(Value)(1024ULL*Megabytes(Value))
#define Terabytes(Value)(1024ULL*Gigabytes(Value))

#if SUSTAIN_SNAIL
#define Assert(Expression) if(!(Expression)){*(int *)0 = 0;}
#else
#define Assert(Expression)
#endif

#define ArrayCount(Array) (sizeof(Array)/sizeof((Array[0])))
//=========================================================================================================
#if(SUSTAIN_INTERNAL)
	struct debug_read_file_result
	{
		uint32 ContentsSize;
		void *Contents;
	};	
	internal debug_read_file_result DEBUGPlatformReadEntireFile(char *Filename);
	internal void DEBUGPlatformFreeFileMemory(void *Memory);
	internal bool32 DEBUGPlatformWriteEntireFile(char *Filename,void *Memory,uint32 MemorySize);
#endif

inline uint32
SafeTruncateUInt64(uint64 Value)
{
	Assert(Value <= 0xFFFFFFFF)
	uint32 Result = (uint32)Value;
	return Result;
}


//structs
//=========================================================================================================
struct game_offscreen_buffer
{
	void *Memory;
	int Width;
	int Height;
	int Pitch;
};

//=========================================================================================================
struct game_sound_buffer
{
	int SampleCount;
	int SamplesPerSecond;
	int16 *Samples;
};
//=========================================================================================================
struct game_button_state
{
	int HalfTransitionCount;
	bool32 EndedDown;
};

struct game_controller_input
{
	bool32 IsAnalog;

	real32 StartX;
	real32 StartY;

	real32 MinX;
	real32 MinY;

	real32 MaxX;
	real32 MaxY;

	real32 EndX;
	real32 EndY;
	union
	{
		game_button_state Buttons[6];
		struct 
		{
			game_button_state Up;
			game_button_state Down;
			game_button_state Left;
			game_button_state Right;
			game_button_state LeftShoulder;
			game_button_state RightShoulder;
		};
	};
};

struct game_input
{
	//clocks
	game_controller_input controller[4];
};

struct game_memory
{
	bool32 IsInitialized;
	uint64 PermanentStorageSize;
	void *PermanentStorage;

	uint64 TransientStorageSize;
	void *TransientStorage;
};

struct game_state
{
	int ToneHz;
	int XOffset;
	int YOffset;
};


//=========================================================================================================

#define SUSTAIN_H
#endif
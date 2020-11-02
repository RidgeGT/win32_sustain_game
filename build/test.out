#include "Sustain.h"

internal void SpecialRenderOffsetPattern(game_offscreen_buffer *Buffer,int XOffset, int YOffset)
{
	//int Pitch = Buffer->Width*Buffer->BytesPerPixel;
	uint8 *Row = (uint8*)Buffer->Memory;
	for(int Y = 0; Y < Buffer->Height;++Y)
	{
		uint32 *Pixel = (uint32 *)Row;
		for(int X = 0; X < Buffer->Width;++X)
		{
			//uint8 R = 0;
			uint8 G = (Y + YOffset);
			uint8 B = (X + XOffset);
			*Pixel++ = ((G << 8) | B);
		}
		Row += Buffer->Pitch;
	}

}

internal void GameOutputSound(game_sound_buffer *SoundBuffer, int ToneHz)
{
	local_persist real32 tSine;
	int16 ToneVolume = 3000;
	int WavePeriod = SoundBuffer->SamplesPerSecond/ToneHz;
	int16 *SampleOut = SoundBuffer->Samples;
	for(int SampleIndex = 0;SampleIndex < SoundBuffer->SampleCount;++SampleIndex)
	{
		real32 SineValue = sinf(tSine);
		int16 SampleValue = (int16)(SineValue*ToneVolume);
		*SampleOut++ = SampleValue;
		*SampleOut++ = SampleValue;
		tSine += 2.0f*Pi32*1.0f/(real32)WavePeriod;
	}
}

internal void GameUpdate(game_memory *Memory,game_input *Input, game_offscreen_buffer *ScreenBuffer,game_sound_buffer *SoundBuffer)
{
	Assert(sizeof(game_state) <= Memory->PermanentStorageSize);

	game_state *GameState = (game_state*)Memory->PermanentStorage;
	if(!Memory->IsInitialized)
	{
		char *Filename = __FILE__;

		debug_read_file_result File = DEBUGPlatformReadEntireFile(Filename);
		if(File.Contents)
		{
			DEBUGPlatformWriteEntireFile("test.out",File.Contents,File.ContentsSize);
			DEBUGPlatformFreeFileMemory(File.Contents);
		}
		GameState->ToneHz = 256;
		Memory->IsInitialized = true;
	}


	game_controller_input *InputController1 = &Input->controller[1];
	if(InputController1->IsAnalog)
	{
		// analog tuning
		GameState->ToneHz = 256 +(int)(128.0f*InputController1->EndY);
		GameState->XOffset += (int)4.0f*((int)InputController1->EndX);
		GameState->YOffset += (int)4.0f*((int)InputController1->EndY);
	}
	else
	{
		// digital tuning
	}
	//Input.AButtoneEndedDown;
	//Input.NumberOfTransitions;
	if(InputController1->Down.EndedDown)
	{
		GameState->XOffset += 1;
	}
	GameOutputSound(SoundBuffer,GameState->ToneHz);
	SpecialRenderOffsetPattern(ScreenBuffer,GameState->XOffset,GameState->YOffset);
}
#if 0
internal game_state GameStartup()
{
		game_state *GameState  = new game_state;
		if(GameState)
		{
			GameState->XOffset = 0;
			GameState->YOffset = 0;
			GameState->ToneHz = 256;
		}
		return GameState;
}

internal void GameShutdown(game_state *GameState)
{
	delete GameState;
}
#endif
#include <cstdio>
#include <iostream>
#include <signal.h>
#include <SDL/SDL_config.h>
#include <SDL/SDL.h>
#include <SDL/SDL_thread.h>
#include <SDL/SDL_audio.h>
#include <stdlib.h>
using namespace std;
struct WAVE{
    SDL_AudioSpec spec; /*SDL Audio Spec Structure */
    Uint8 *sound; /*Pointer to wave data */
    Uint32 soundlen;/*Length of wave data */
    int soundpos;/*Current play position */
};
WAVE wave;
int cutOffFrequency = 0;//story cut-off frequency
double bCoefficients[6];
double aCoefficients[5];
double originalVoice[6];
int voiceInsertCount = 0;
double processedVoice[6];
int processedPointCount = 0;
bool  loopPlayMode = true;
static const int SCR_WIDTH = 240;
static const int SCR_HEIGHT = 180;
void poked(int);
static void quit(int);
void SDLCALL fillerup(void *, Uint8 *, int);
static int done = 0;
bool showHelp (int, char *[]);
bool checkCutOffFrequency(int, char *[]);
bool LoadSDLLib();
void makeScreen(SDL_Surface *&);
void loadWave(char *[]);
void setSignal();
void initFillerup();
void cleanSignal();
void ctrlAudio(char [], SDL_Event, Uint8 *&);
void process_audio(Uint8 *, int);
void showWAVInfo();
bool limitFormatSupport();
void setFilterCoefficients();
double butterworth(double);
void init();

int main(int argc, char *argv[])
{
	char name[32];
	SDL_Event event;
	Uint8 *keys;
	SDL_Surface *screen;
	//init
	init();
	if (showHelp (argc, argv) == true)	return 0;//show help
	////check cut-off frequency must 200Hz or 500Hz
	if (checkCutOffFrequency(argc, argv) == false) return 0;
	//Load the SDL library
	if (LoadSDLLib() == false) return 0;
	// Make a screen for visual information & for accepting keyboard events
	makeScreen(screen);
	/* Load the wave file into memory */
	loadWave(argv);
	//set signal
	setSignal();
	//limit whether format support
	if (limitFormatSupport() == false)	return 0;
	//set Filter Coefficients
	setFilterCoefficients();
	//init fillerup
	initFillerup();
	// start playing
	SDL_PauseAudio(0);
	ctrlAudio(name, event, keys);
	cleanSignal();
	return 0;
}
bool showHelp (int argc, char *argv[])
{
	string check(argv[1]);//convert char* to string
	if (argc == 2 && check == "-help")
	{
		cout << "This program is low-pass Butterworth filter!!!" << endl;
		cout << "The cut-off frequency must 200Hz or 500Hz" << endl;
		cout << "Run this program command line is:";
		cout << " lpplay fc file.wav" << endl;
		cout << "lpplay is program name" << endl;
		cout << "fc is cut-off frequency must 200Hz or 500Hz" << endl;
		cout << "file.wav is input file name" << endl;
		return true;
	}
	return false;
}
bool checkCutOffFrequency(int argc, char *argv[])
{
	//check cut-off frequency must 200Hz or 500Hz
	string check(argv[1]);//convert char* to string
	if (check == "200" && argc == 3)
	{
		cout << "Cut-off frequency is 200Hz" << endl;
		cutOffFrequency = 200;
		return true;
	}
	else if (check == "500" && argc == 3)
	{
		cout << "Cut-off frequency is 500Hz" << endl;
		cutOffFrequency = 500;
		return true;
	}
	else
		cout << "fc must is either 200 or 500" << endl;
	return false;
}
bool LoadSDLLib()
{
	if ( SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0 )
	{
		cerr << "Couldn't initialize SDL: " << SDL_GetError() << endl;
        	return false;
	}
	return true;
}
void makeScreen(SDL_Surface *& screen)
{
	#ifndef __DARWIN__
        	screen = SDL_SetVideoMode(SCR_WIDTH, SCR_HEIGHT, 0, 0);
	#else
		screen = SDL_SetVideoMode(SCR_WIDTH, SCR_HEIGHT, 24, 0);
	#endif
	if (!screen)
	{
		cerr << "SDL: could not set video mode - exiting" << endl;
		quit(1);
	}
	return;
}
void loadWave(char *argv[])
{
	if ( SDL_LoadWAV(argv[2],&wave.spec, &wave.sound, &wave.soundlen) == NULL )
	{
		cerr << "Couldn't load " << argv[2] << " " << SDL_GetError() << endl;
		quit(1);
	}

	wave.spec.callback = fillerup;
}
void setSignal()
{
	#if HAVE_SIGNAL_H
		/* Set the signals */
		#ifdef SIGHUP
			signal(SIGHUP, poked);
		#endif
			signal(SIGINT, poked);
		#ifdef SIGQUIT
			signal(SIGQUIT, poked);
		#endif
			signal(SIGTERM, poked);
	#endif /* HAVE_SIGNAL_H */
	return;
}
void initFillerup()
{
	/* Initialize fillerup() variables */
	if ( SDL_OpenAudio(&wave.spec, NULL) < 0 )
	{
		cerr << "Couldn't open audio: " << SDL_GetError() << endl;
		SDL_FreeWAV(wave.sound);
		quit(2);
	}
	return;
}
void cleanSignal()
{
	/* Clean up on signal */
	SDL_CloseAudio();
	SDL_FreeWAV(wave.sound);
	SDL_Quit();
	return;
}
void ctrlAudio(char name[], SDL_Event event, Uint8 *& keys)
{
	/* Let the audio run */
	cout << "Using audio driver: " << SDL_AudioDriverName(name, 32) << endl;
	while ( ! done && (SDL_GetAudioStatus() == SDL_AUDIO_PLAYING) )
	{
		// Poll input queue, run keyboard loop
		while ( SDL_PollEvent(&event) )
		{
			if ( event.type == SDL_QUIT )
			done = 1;
		}
		keys = SDL_GetKeyState(NULL);
		int temp = SDL_GetModState(); 
		temp = temp & KMOD_CAPS; 
		if (keys[SDLK_q])
		{
			done = 1;
		}
		else if (keys[SDLK_r])
		{
			wave.soundpos = 0;
		}
		else if (keys[SDLK_t] && temp != KMOD_CAPS)
		{
			if (loopPlayMode == true)
				loopPlayMode = false;
			else if (loopPlayMode == false)
				loopPlayMode = true;
		}
		SDL_Delay(100);
	}
	return;
}
void poked(int sig)
{
    done = 1;
}
/* Call this instead of exit(), so we can clean up SDL: atexit() is evil. */
static void quit(int rc)
{
    SDL_Quit();
    exit(rc);
}
//This is a call-back function that will be automatically called by SDL
void SDLCALL fillerup(void *unused, Uint8 *stream, int len)
{
	Uint8 *waveptr;
	int waveleft;

	/* Set up the pointers */
	waveptr = wave.sound + wave.soundpos;
	waveleft = wave.soundlen - wave.soundpos;

	/* Go! */
	while ( waveleft <= len )
	{
		/* Process samples */
		/* Uint8 process_buf[waveleft]; */
		Uint8 *process_buf = (Uint8 *)malloc(sizeof(Uint8)*waveleft);
		if (process_buf == NULL) break;

		SDL_memcpy(process_buf, waveptr, waveleft);
		process_audio(process_buf, waveleft);
	
		// play the end of the audio
		SDL_memcpy(stream, process_buf, waveleft);
		stream += waveleft;
		len -= waveleft;

		// ready to repeat play the audio
		if (loopPlayMode == true)
		{
			waveptr = wave.sound;
			waveleft = wave.soundlen;
			wave.soundpos = 0;
		}
		free(process_buf);
	}
	
	/* Process samples */
	/* Uint8 process_buf[len]; */
	Uint8 *process_buf = (Uint8 *)malloc(sizeof(Uint8)*len);
	if (process_buf == NULL) return;

	SDL_memcpy(process_buf, waveptr, len);
	process_audio(process_buf, len);
	
	// play the processed samples
	SDL_memcpy(stream, process_buf, len);
	wave.soundpos += len;
	free(process_buf);
}
/* Process the audio buffer */
void process_audio(Uint8 *audio_buf, int buf_size)
{
	int i;
	if (wave.spec.format == AUDIO_U16LSB)
	{
    
		//unsigned 16-bit sample, little-endian
		Uint16 *ptr = (Uint16 *)audio_buf;
		// process mono audio
		for (i=0; i<buf_size; i += sizeof(Uint16))
		{
			#if (SDL_BYTE_ORDER == SDL_BIG_ENDIAN)
				Uint16 swap = SDL_Swap16(*ptr);
				double tmp = swap;
				tmp = butterworth(tmp);
				tmp = tmp * 30;
				tmp = tmp > 65535.0 ? 65535.0 : tmp;
				swap = (Uint16)tmp;
				*ptr = SDL_Swap16(swap);
				ptr++; // note here ptr will move to 16 bits higher
			#else
				double tmp = (*ptr);
				tmp = butterworth(tmp);
				tmp = tmp * 30;
				tmp = tmp > 65535.0 ? 65535.0 : tmp;
				*ptr = (Uint16)tmp;
				ptr++;
			#endif
		}
	}
	else if (wave.spec.format == AUDIO_S16LSB)
	{	
		//signed 16-bit sample, little-endian
		Sint16 *ptr = (Sint16 *)audio_buf;
		// process mono audio
		for (i=0; i<buf_size; i += sizeof(Sint16))
		{
			#if (SDL_BYTE_ORDER == SDL_BIG_ENDIAN)
				Sint16 swap = SDL_Swap16(*ptr);
				double tmp = swap;
				tmp = butterworth(tmp);
				tmp = tmp * 30;
				tmp = tmp > 32767.0 ? 32767.0 : tmp;
				tmp = tmp < -32768.0 ? -32768.0 : tmp;
				swap = (Sint16)tmp;
				*ptr = SDL_Swap16(swap);
				ptr++;
			#else
				double tmp = (*ptr);
				tmp = butterworth(tmp);
				tmp = tmp * 30;
				tmp = tmp > 32767.0 ? 32767.0 : tmp;
				tmp = tmp < -32768.0 ? -32768.0 : tmp;
				*ptr = (Sint16)tmp;
				ptr++;
			#endif
		}
        }
        else if (wave.spec.format == AUDIO_U16MSB)
	{
		//unsigned 16-bit sample, big-endian
		Uint16 *ptr = (Uint16 *)audio_buf;
		// process mono audio
		for (i=0; i<buf_size; i += sizeof(Uint16))
		{
			#if (SDL_BYTE_ORDER == SDL_LIL_ENDIAN)
				Uint16 swap = SDL_Swap16(*ptr);
				double tmp = swap;
				tmp = butterworth(tmp);
				tmp = tmp * 30;
				tmp = tmp > 65535.0 ? 65535.0 : tmp;
				swap = (Uint16)tmp;
				*ptr = SDL_Swap16(swap);
				ptr++; // note here ptr will move to 16 bits higher
			#else
				double tmp = (*ptr);
				tmp = butterworth(tmp);
				tmp = tmp * 30;
				tmp = tmp > 65535.0 ? 65535.0 : tmp;
				*ptr = (Uint16)tmp;
				ptr++;
			#endif
		}
        } 
	else if (wave.spec.format == AUDIO_S16MSB)
	{
		//signed 16-bit sample, big-endian
		Sint16 *ptr = (Sint16 *)audio_buf;
		// process mono audio
		for (i=0; i<buf_size; i += sizeof(Sint16))
		{
			#if (SDL_BYTE_ORDER == SDL_LIL_ENDIAN)
				Sint16 swap = SDL_Swap16(*ptr);
				double tmp = swap;
				tmp = butterworth(tmp);
				tmp = tmp * 30;
				tmp = tmp > 32767.0 ? 32767.0 : tmp;
				tmp = tmp < -32768.0 ? -32768.0 : tmp;
				swap = (Sint16)tmp;
				*ptr = SDL_Swap16(swap);
				ptr++;
			#else
				double tmp = (*ptr);
				tmp = butterworth(tmp);
				tmp = tmp * 30;
				tmp = tmp > 32767.0 ? 32767.0 : tmp;
				tmp = tmp < -32768.0 ? -32768.0 : tmp;
				*ptr = (Sint16)tmp;
				ptr++;
			#endif
            }
        }
	return;
}
void showWAVInfo()
{
	int sampleBit = 16;
	//Number of channels
	cout << "A. Channels: " << static_cast<int>(wave.spec.channels) << endl;
	//Number of bits per sample
	cout << "B. Number of bits per sample: ";
	// unsigned 8-bit sample
	if (wave.spec.format == AUDIO_U8)
	{
		cout << "8-bit unsigned" << endl;
		sampleBit = 8;
	}
	// signed 8-bit sample
	else if (wave.spec.format == AUDIO_S8)
	{
		cout << "8-bit signed" << endl;
		sampleBit = 8;
	}
	// unsigned 16-bit sample, little-endian
	else if (wave.spec.format == AUDIO_U16LSB)
	{
		cout << "16-bit unsigned little-endian" << endl;
		sampleBit = 16;
	}
	// signed 16-bit sample, little-endian
	else if (wave.spec.format == AUDIO_S16LSB)
	{
		cout << "16-bit signed little-endian" << endl;
		sampleBit = 16;	
	}
	// unsigned 16-bit sample, big-endian
	else if (wave.spec.format == AUDIO_U16MSB)
	{
		cout << "16-bit unsigned big-endian" << endl;
		sampleBit = 16;	
	}
	// signed 16-bit sample, big-endian
	else if (wave.spec.format == AUDIO_S16MSB)
	{
		cout << "16-bit signed big-endian" << endl;
		sampleBit = 16;
	}
	else
		cout << "Unknow format!!!" << endl;
	//Sampling frequency
	cout << "C. Sampling frequency: "<< static_cast<int>(wave.spec.freq)<< endl;
	//Duration in seconds
	cout << "D. Duration in seconds: " << 8 * wave.soundlen/ static_cast<int>(wave.spec.freq)/sampleBit/static_cast<int>(wave.spec.channels)<< endl;
	return;	
}
bool limitFormatSupport()
{
	int HzOne = 44100;
	int HzTwo = 22050;
	bool support = true;
	if (static_cast<int>(wave.spec.channels) != 1)
	{
		cout << "The file is not 1 channels mono file" << endl;
		support = false;
	}
	if (wave.spec.format != AUDIO_U16LSB &&
	wave.spec.format != AUDIO_S16LSB &&
	wave.spec.format != AUDIO_U16MSB &&
	wave.spec.format != AUDIO_S16MSB)
	{
		cout << "The file is not 16 bits file" << endl;
		support = false;
	}
	if (static_cast<int>(wave.spec.freq) != HzOne && static_cast<int>(wave.spec.freq) != HzTwo)
	{
		cout << "The file Sampling frequency is not 44100Hz or 22050Hz" << endl;
		support = false;
	}
	if (support == false)
		showWAVInfo();
	return support;
}
void setFilterCoefficients()
{
	if (cutOffFrequency == 500 && static_cast<int>(wave.spec.freq) == 22050)
	{
		bCoefficients[0] = 4.92870054213945e-08;
		bCoefficients[1] = 2.46435027106973e-07;
		bCoefficients[2] = 4.92870054213945e-07;
		bCoefficients[3] = 4.92870054213945e-07;
		bCoefficients[4] = 2.46435027106973e-07;
		bCoefficients[5] = 4.92870054213945e-08;
		
		aCoefficients[0] = -4.77126970811754;
		aCoefficients[1] = 9.11101949606003;
		aCoefficients[2] = -8.70364051282520;
		aCoefficients[3] = 4.15937748789777;
		aCoefficients[4] = -0.795485185830889;
	}
	else if ((cutOffFrequency == 200&&static_cast<int>(wave.spec.freq) == 22050)
	|| (cutOffFrequency == 500&&static_cast<int>(wave.spec.freq) == 44100)
	|| (cutOffFrequency == 200&&static_cast<int>(wave.spec.freq) == 44100))
	{
		bCoefficients[0] = 2.74050909743195e-09;
		bCoefficients[1] = 1.37025454871598e-08;
		bCoefficients[2] = 2.74050909743195e-08;
		bCoefficients[3] = 2.74050909743195e-08;
		bCoefficients[4] = 1.37025454871598e-08;
		bCoefficients[5] = 2.74050909743195e-09;
		
		aCoefficients[0] = -4.87292228923095;
		aCoefficients[1] = 9.49972611051988;
		aCoefficients[2] = -9.26133224169541;
		aCoefficients[3] = 4.51518275374949;
		aCoefficients[4] = -0.880654245646732;
	}
	return;
}
void init()
{
	//init bCoefficients, originalVoice and processedVoice
	for (int i = 0; i < 6; i++)
	{
		originalVoice[i] = 0.0;
		processedVoice[i] = 0.0;
		bCoefficients[i] = 0.0;
	}
	for (int i = 0; i < 5; i++)
		aCoefficients[i] = 0.0;
	return;
}
double butterworth(double original)
{
	//when Voice data aray full insert from first element 
	if (voiceInsertCount == 6)
		voiceInsertCount = 0;
	//insert to original Voice data aray
	originalVoice[voiceInsertCount] = original;
	voiceInsertCount++;
	//process voice
	int tempOne = processedPointCount % 6;
	int tempTwo = tempOne - 1;
	if (tempTwo < 0)
		tempTwo = 5;
	if (processedPointCount == 0)
	{
		processedVoice[0] =
				 (bCoefficients[0] * originalVoice[0]
				 + bCoefficients[1] * originalVoice[0]
				 + bCoefficients[2] * originalVoice[0]
				 + bCoefficients[3] * originalVoice[0]
				 + bCoefficients[4] * originalVoice[0]
				 + bCoefficients[5] * originalVoice[0])
				 /(1 + aCoefficients[0]  + aCoefficients[1]
				  + aCoefficients[2]  + aCoefficients[3]
				  + aCoefficients[4]);		 
	}
	else if (processedPointCount == 1)
	{
		processedVoice[1]=
				bCoefficients[0] * originalVoice[1]
				+ bCoefficients[1] * originalVoice[0]
				+ bCoefficients[2] * originalVoice[0]
				+ bCoefficients[3] * originalVoice[0]
				+ bCoefficients[4] * originalVoice[0]
				+ bCoefficients[5] * originalVoice[0]
				- aCoefficients[0] * processedVoice[0]
				- aCoefficients[1] * processedVoice[0]
				- aCoefficients[2] * processedVoice[0]
				- aCoefficients[3] * processedVoice[0]
				- aCoefficients[4] * processedVoice[0];
	}
	else if (processedPointCount == 2)
	{
		processedVoice[2]=
				bCoefficients[0] * originalVoice[2]
				+ bCoefficients[1] * originalVoice[1]
				+ bCoefficients[2] * originalVoice[0]
				+ bCoefficients[3] * originalVoice[0]
				+ bCoefficients[4] * originalVoice[0]
				+ bCoefficients[5] * originalVoice[0]
				- aCoefficients[0] * processedVoice[1]
				- aCoefficients[1] * processedVoice[0]
				- aCoefficients[2] * processedVoice[0]
				- aCoefficients[3] * processedVoice[0]
				- aCoefficients[4] * processedVoice[0];
	}
	else if (processedPointCount == 3)
	{
		processedVoice[3]=
				bCoefficients[0] * originalVoice[3]
				+ bCoefficients[1] * originalVoice[2]
				+ bCoefficients[2] * originalVoice[1]
				+ bCoefficients[3] * originalVoice[0]
				+ bCoefficients[4] * originalVoice[0]
				+ bCoefficients[5] * originalVoice[0]
				- aCoefficients[0] * processedVoice[2]
				- aCoefficients[1] * processedVoice[1]
				- aCoefficients[2] * processedVoice[0]
				- aCoefficients[3] * processedVoice[0]
				- aCoefficients[4] * processedVoice[0];
	}
	else if (processedPointCount == 4)
	{
		processedVoice[4]=
				bCoefficients[0] * originalVoice[4]
				+ bCoefficients[1] * originalVoice[3]
				+ bCoefficients[2] * originalVoice[2]
				+ bCoefficients[3] * originalVoice[1]
				+ bCoefficients[4] * originalVoice[0]
				+ bCoefficients[5] * originalVoice[0]
				- aCoefficients[0] * processedVoice[3]
				- aCoefficients[1] * processedVoice[2]
				- aCoefficients[2] * processedVoice[1]
				- aCoefficients[3] * processedVoice[0]
				- aCoefficients[4] * processedVoice[0];
	}
	else
	{		
		processedVoice[tempOne] = 0.0;
		for (int i = 0; i < 6; i++)
		{
			processedVoice[tempOne] +=
				bCoefficients[i] * originalVoice[tempOne];
			tempOne--;
			if (tempOne < 0)
				tempOne = 5;
			
		}
		for (int i = 0; i < 5; i++)
		{
			processedVoice[tempOne] -=
				aCoefficients[i] * processedVoice[tempTwo];
			tempTwo--;
			if (tempTwo < 0)
				tempTwo = 5;
			
		}
	}
	processedPointCount++;
	return processedVoice[tempOne];
	
}

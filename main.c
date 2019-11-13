// SP Music Player
// Copyright (c) 2019 John Bliss
// All rights reserved. License: 2-clause BSD

#define _XOPEN_SOURCE   600
#define _POSIX_C_SOURCE 1
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <SDL.h>
#include "ym2151.h"

#define AUDIO_SAMPLES 4096
#define SAMPLERATE 22050

#define MHZ 8

void *emulator_loop(void *param);

const char *audio_dev_name = NULL;
SDL_AudioDeviceID audio_dev = 0;
uint32_t clockticks6502 = 0, clockgoal6502 = 0;
int32_t last_perf_update = 0;
int32_t perf_frame_count = 0;
int window_scale = 1;
char *scale_quality = "best";
char window_title[30];
char* music_filename = "MUSIC.SP";
FILE *music_file = NULL;
bool mode = 0;

static void
usage()
{
	printf("\nSP Music Player  (C)2019 John Bliss\n");
	printf("All rights reserved. License: 2-clause BSD\n\n");
	printf("Usage: spplayer [option] ...\n\n");
	printf("-input <file.sp>\n");
	printf("\tOverride Default file MUSIC.SP\n");
	printf("-sound <output device>\n");
	printf("\tSet the output device used for audio emulation");
	printf("-mode <e/a>\n");
	printf("\tChoose which timing mode to use. Either efficient mode, or accurate mode. (e or a)");
	printf("\n");
	exit(1);
}


void audioCallback(void* userdata, Uint8 *stream, int len)
{
	YM_stream_update((uint16_t*) stream, len / 4);
}

void usageSound()
{
	// SDL_GetAudioDeviceName doesn't work if audio isn't initialized.
	// Since argument parsing happens before initializing SDL, ensure the
	// audio subsystem is initialized before printing audio device names.
	SDL_InitSubSystem(SDL_INIT_AUDIO);

	// List all available sound devices
	printf("The following sound output devices are available:\n");
	const int sounds = SDL_GetNumAudioDevices(0);
	for (int i=0; i < sounds; ++i) {
		printf("\t%s\n", SDL_GetAudioDeviceName(i, 0));
	}

	SDL_Quit();
	exit(1);
}

void
init_audio()
{
	SDL_AudioSpec want;
	SDL_AudioSpec have;

	// setup SDL audio
	want.freq = SAMPLERATE;
	want.format = AUDIO_S16SYS;
	want.channels = 2;
	want.samples = AUDIO_SAMPLES;
	want.callback = audioCallback;
	want.userdata = NULL;

	if (audio_dev > 0)
	{
		SDL_CloseAudioDevice(audio_dev);
	}

	audio_dev = SDL_OpenAudioDevice(audio_dev_name, 0, &want, &have, 9 /* freq | samples */);
	if ( audio_dev <= 0 ){
		fprintf(stderr, "SDL_OpenAudioDevice failed: %s\n", SDL_GetError());
		if (audio_dev_name != NULL) usageSound();
		exit(-1);
	}

	// init YM2151 emulation. 4 MHz clock
	YM_Create(4000000);
	YM_init(have.freq, 60);

	// start playback
	SDL_PauseAudioDevice(audio_dev, 0);
}

void closeAudio()
{
	SDL_CloseAudioDevice(audio_dev);
}

int
main(int argc, char **argv)
{
	argc--;
	argv++;

	while (argc > 0) {
		if (!strcmp(argv[0], "-sound")) {
			argc--;
			argv++;
			if (!argc || argv[0][0] == '-') {
				usageSound();
			}
			audio_dev_name = argv[0];
			argc--;
			argv++;
		} else if (!strcmp(argv[0], "-input")){
			argc--;
			argv++;
			if (!argc || argv[0][0] == '-') {
				usage();
			}
			music_filename = argv[0];
			argc--;
			argv++;
		}else if (!strcmp(argv[0], "-mode")){
			argc--;
			argv++;
			if (!argc || argv[0][0] == '-') {
				usage();
			}
			if(!strcmp(argv[0], "e")){
				mode = 0;
			}else if(!strcmp(argv[0], "a")){
				mode = 1;
			}else {
				usage();
			}
			argc--;
			argv++;
		}else {
			usage();
		}
	}


	music_file = fopen(music_filename, "rb");
	if (!music_file) {
		printf("Cannot open %s!\n", music_filename);
		exit(1);
	}
	uint8_t * junk = malloc(sizeof(uint8_t)*2);
	fread(junk,2,1,music_file);



	SDL_Init(SDL_INIT_AUDIO);

	init_audio();

	emulator_loop(NULL);

	closeAudio();

	SDL_Quit();

	fclose(music_file);

	return 0;
}



typedef struct{
	uint8_t reg;
	uint8_t data;
}  YM2151_write;

void*
emulator_loop(void *param)
{


	bool running = true;
	int delay = 0;
	uint32_t frame = 0;
	while(running){
		if(mode == 0){
			int32_t sdlTicks = SDL_GetTicks();
			int32_t diff_time = 1000 * frame / 60 - sdlTicks;
			if (diff_time > 0) {
				usleep(1000 * diff_time);
			}
		}else{
			int32_t time_to_wait = frame * 1000/60 - SDL_GetTicks();
			while (time_to_wait > 0) {
				// printf("Time to wait: %d\n",time_to_wait);
				time_to_wait = frame * 1000/60 - SDL_GetTicks();
			}
		}

		//DO Sound stuff
		if(delay == 0){
			uint8_t * numWrites = malloc(sizeof(uint8_t));
			fread(numWrites,1,1,music_file);
			if(*numWrites == 0){
				running = false;
			}else{
				YM2151_write * writes = malloc((*numWrites) * sizeof(YM2151_write));
				fread(writes,sizeof(YM2151_write),*numWrites,music_file);
				for(int i=0; i<*numWrites; i++){
					YM2151_write * thisWrite = writes + i;
					YM_write_reg(thisWrite->reg,thisWrite->data);
				}
				uint8_t * new_delay = malloc(sizeof(uint8_t));
				fread(new_delay,1,1,music_file);
				delay = *new_delay;
			}
		}else{
			delay -= 1;
		}


		frame++;
	}



	return 0;
}

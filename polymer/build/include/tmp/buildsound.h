
typedef struct {
	unsigned long  samplerate;	// sample rate of the sound
	unsigned short channels;	// number of channels
	unsigned short bitspersample;	// bits per sample
	unsigned short format;		// 0 = pcm
	unsigned long  soundlength;	// bytes required for sound storage
} SoundInfo;


int ReadVOCInfo(int fh, SoundInfo *snd);
int ReadVOCData(int fh, char *data, int bufferlen);

int ReadWAVInfo(int fh, SoundInfo *snd);
int ReadWAVData(int fh, char *data, int bufferlen);

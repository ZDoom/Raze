#ifndef __jfaud_hpp__
#define __jfaud_hpp__

#include "file.hpp"
#include "mixer.hpp"
#include "cda.hpp"

#ifdef JFAUD_INTERNAL
struct _wavechan {
	JFAudMixerChannel *h;
	int priority;
	unsigned age;
};
#endif

class JFAud {
private:
	JFAudFile* (*useropenfunc)(const char *, const char *);

	JFAudMixer *wavemixer;

	int numwavechans;
	JFAudCDA *cddev;

#ifdef JFAUD_INTERNAL
	struct _wavechan *wavechans, *musicchan;
	WaveOut *waveout;
	SoundCache *cache;
	JFAudMidiSynth *midisynth;
#else
	void *wavechans, *musicchan;
	void *waveout, *cache, *midisynth;
#endif

	struct _wavechan * FindFreeWaveChan(int priority);

	void *winhnd;

protected:
public:
	JFAud();
	~JFAud();

	// Initialisation and general control
	void SetUserOpenFunc(JFAudFile* (*func)(const char *, const char *)) { useropenfunc = func; }
		// Pass NULL to use conventional stdio file services, otherwise pass
		// a suitable pointer to a function that will handle opening files.
		// The function must return a JFAudFile object or NULL, which means
		// it must return an object of a class derived from JFAudFile.

	bool InitWave(const char *name, int numvoices = 32, int frequency = 44100);
		// 'name' may be a string of the format
		//    driver[:device id]
		// The only valid driver is currently "openal".
		// If no device id is specified the default for the driver will be used.
		// If 'name' is NULL, the default driver will be used.
		// The numvoices and frequency parameters are only used for drivers that accept them.

	bool SetWindowHandle(void *hwnd);
		// WIN32: 'hwnd' is the window handle cast to a void*
		// Other: function does nothing

	bool SetCacheSize(unsigned cache, unsigned object);
		// Sets the total cache and per-object size limits
	
	bool SetCacheItemAge(unsigned age);
		// Sets the maximum age of an unused cache item

	bool InitMIDI(const char *name);
		// 'name' may be a string naming the device to use.
		// If 'name' is NULL, the default device will be used.
		// WIN32: give the MIDI device name
		// etc: no MIDI supported

	bool InitCDA(const char *name);
		// 'name' may be a string naming the device to use.
		// If 'name' is NULL, the default device will be used.
		// WIN32: give the drive letter to use for playback
		// LINUX,BSD: give the drive number that SDL would use for playback

	bool Uninit(void);
		// Perform system uninitialisation

	bool Update(bool agecache = false);
		// Update streams, etc.

	bool AgeCache(void);
		// Ages the cache

	// Device enumeration
	static char **EnumerateWaveDevices(const char *name, char **def);
		// 'name' may be NULL to return a list of valid drivers, or the name of a driver
		// to return a list of valid device ids.
		// The return value is a dynamically allocated array of strings, terminated by a NULL pointer,
		// that you must free.

	static char **EnumerateMIDIDevices(char **def);
		// Returns a list of legal device names for InitMIDI().
		// The return value is a dynamically allocated array of strings, terminated by a NULL pointer,
		// that you must free.
	
	static char **EnumerateCDADevices(char **def);
		// Returns a list of legal device names for InitCDA().
		// The return value is a dynamically allocated array of strings, terminated by a NULL pointer,
		// that you must free.
	
	// Sound effect playback
	// Hint: if you want to play from a file in memory, use the MemoryFile class in memfile.hpp to wrap
	// the pointer you have in a JFAudFile class which you can then pass to PlaySoundFile.
	JFAudMixerChannel *PlaySound(const char *filename, const char *subfilename = (const char*)0, int priority = 1);
	JFAudMixerChannel *PlaySoundFile(JFAudFile **file, int priority = 1);
		// Prepares a channel for playing the named sound. The channel is paused ready
		// for set up and play. You can call any of the methods of JFAudMixerChannel, but don't
		// delete the handle yourself--pass it to FreeSound() instead.
		//
		// When passing a JFAudFile object, you have to pass a pointer to a file object. If the
		// function returns a valid handle, *file will be set to NULL. If the function returns
		// a NULL handle, *file might also be set to NULL if the file was properly identified and
		// an error occurred while attempting to set up the mixer channel, otherwise it will be
		// untouched and you are responsible for disposing of it. In the event everything succeeds,
		// don't touch the file object ever again. The mixer will dispose of it when it's no longer
		// needed.

	JFAudMixerChannel *PlayRawSound(const char *filename, const char *subfilename = (const char*)0, int priority = 1, int samplerate = 11025, int channels = 1, int bytespersample = 2, bool bigendian = false);
	JFAudMixerChannel *PlayRawSoundFile(JFAudFile **file, int priority = 1, int samplerate = 11025, int channels = 2, int bytespersample = 2, bool bigendian = false);
		// Like PlaySound(), only these play headerless PCM of the format you specify.

	bool FreeSound(JFAudMixerChannel *chan);
		// Releases a channel. Pass a channel object to this function to dispose of it rather
		// than deleting it yourself.
	
	bool IsValidSound(JFAudMixerChannel *chan) const;
		// Checks to see if a the passed channel is still valid. Because of the priority system, a call
		// to PlaySound or PlayMusic might kick out a channel, which leaves you with a pointer that's no
		// longer valid, so verify that it's still safe before working with the channel.

	JFAudMixer * GetWave(void) const { return wavemixer; }
		// Don't try deleting the object you get from this function or bad thing will happen. Just
		// ignore it when you're done with it.

	// Music playback
	bool PlayMusic(const char *filename, const char *subfilename = (const char*)0);
	bool PlayMusicFile(JFAudFile **file);
	bool PauseMusic(bool onf);
	bool StopMusic(void);

	// CD audio playback
	JFAudCDA * GetCDA(void) const { return cddev; }
		// Don't try deleting the object you get from this function or bad things will happen. Just
		// ignore it when you're done with it.

	bool SetWaveVolume(float vol);
	bool SetMIDIVolume(float vol);
	bool SetCDAVolume(float vol);
};

void JFAud_SetLogFunc(void (*func)(const char *));

#endif

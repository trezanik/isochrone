/*
 * Consider this public domain; most of the code was acquired from existing open
 * source, and very little effort went into this files creation. Do as you please.
 * 
 * This can be used for:
 * 1) Validating the listed libraries are all that's required for linking into a build [does this build]
 * 2) Validating the linked libraries actually execute as expected [does this run]
 * 3) Testing each dependency as standalone as possible to check system compatibility [does output reflect expectations]
 * 
 * Assumes Windows and Visual Studio.
 * 
 * NOT a supported file/project - this is for internal use only, and is only
 * updated/used as needed. Amendments and contributions will be accepted, but
 * don't expect issues to be given attention; no effort was put in to make this
 * maintainable or legible!
 */

/*
 * These are the only items that should be modified unless you know explicitly
 * what you're doing.
 */
static bool  testing_flac = false;
static bool  testing_freetype = false;
static bool  testing_imgui = false;
static bool  testing_oggopus = true;
static bool  testing_oggvorbis = false;
static bool  testing_openalsoft = true;
static bool  testing_openssl = false;
static bool  testing_pugixml = false;
static bool  testing_sdl = false;
static bool  testing_sqlite = false;
static bool  testing_stb = false;


#if _MSC_VER
#	define _CRT_SECURE_NO_WARNINGS  // fopen
#	if TZK_USING_FLAC
#		pragma comment ( lib, "FLAC.lib" )
#	endif
#	if TZK_USING_FREETYPE
#		pragma comment ( lib, "freetype.lib" )
#	endif
#	if TZK_USING_IMGUI
#		pragma comment ( lib, "imgui.lib" )
#	endif
#	if TZK_USING_OGGOPUS
#		pragma comment ( lib, "ogg.lib" )
#		pragma comment ( lib, "opus.lib" )
#		pragma comment ( lib, "opusfile.lib" )
#	endif
#	if TZK_USING_OGGVORBIS
#		pragma comment ( lib, "ogg.lib" )
#		pragma comment ( lib, "vorbis.lib" )
#		pragma comment ( lib, "vorbisfile.lib" )
#	endif
#	if TZK_USING_OPENALSOFT
#		pragma comment ( lib, "OpenAL32.lib" )
#	endif
#	if TZK_USING_OPENSSL
#		pragma comment ( lib, "libcrypto.lib" )
#		pragma comment ( lib, "libssl.lib" )
#	endif
#	if TZK_USING_PUGIXML
#		pragma comment ( lib, "pugixml.lib" )
#	endif
#	if TZK_USING_SDL
#		pragma comment ( lib, "SDL2d.lib" )
#		pragma comment ( lib, "opengl32.lib" )
#	endif
#	if TZK_USING_SQLITE
#		pragma comment ( lib, "sqlite3.lib" )
#	endif
#endif

#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cinttypes>
#include <chrono>
#include <thread>
#include <vector>
#include <set>
#include <unordered_set>

#if TZK_USING_IMGUI
// Dependency: imgui (internal-only)
#include "imgui/dear_imgui/imgui.h"
#include "imgui/ImGuiImpl_Base.h"
#	if TZK_USING_SDL
#include "imgui/dear_imgui/imgui_impl_sdl2.h"
#include "imgui/dear_imgui/imgui_impl_sdlrenderer2.h"
#	endif
#endif

#if TZK_USING_PUGIXML
// Dependency: pugixml
#include <pugixml.hpp>
#endif

#if TZK_USING_FREETYPE
// Dependency: freetype (dynamic library)
#include <freetype/freetype.h>
#endif

#if TZK_USING_OGGOPUS
// Dependency: ogg+vorbis (dynamic library)
#include <ogg/ogg.h>
#include <opus.h>
#include <opus/opusfile.h>
#include <io.h> // for dup
#endif

#if TZK_USING_OGGVORBIS
// Dependency: ogg+vorbis (dynamic library)
#include <ogg/ogg.h>
#include <vorbis/vorbisfile.h>
#endif

#if TZK_USING_OPENALSOFT
// Dependency: openal (dynamic library)
#include <AL/al.h>
#include <AL/alc.h>
void
CheckOpenAL(
	uint32_t sample_rate,
	uint16_t bits_per_sample,
	uint16_t num_channels,
	uint32_t data_size,
	void* audio_data
);
#endif

#if TZK_USING_SDL
// Dependency: SDL (dynamic library)
#if !defined(SDL_MAIN_HANDLED)
#	define SDL_MAIN_HANDLED  // don't override main
#endif
#include <SDL.h>
#endif

#if TZK_USING_SQLITE
#	include <sqlite3.h>
#endif

#if TZK_USING_STB
// Dependency: STB (header only)
#define STB_IMAGE_IMPLEMENTATION // this file implements
#include <stb_image.h>
#endif

#include <Windows.h>

// don't use anything that requires dynamic memory allocation!!
#include "core/util/hash/Hash_SHA1.h"
#include "core/util/filesystem/env.h"
#include "core/util/string/string.h"
#include "core/UUID.h"
#pragma comment ( lib, "core.lib" )


// lovely globals

#if TZK_USING_SQLITE
// can't use our expand_env as it requires services, so hardcoding
char      g_dbfpath[2048]{ "C:\\Users\\localadmin\\AppData\\Roaming\\Trezanik\\isochrone\\rss.db" };
sqlite3*  g_db;
std::vector<std::string>  g_all_hashvals_for_feed;
#endif


void
CheckWav()
{
	struct riff_header
	{
		uint8_t   riff_sig[4];     ///< "RIFF"
		uint32_t  chunk_size;      ///< little endian
		uint8_t   riff_subtype[4]; ///< "WAVE" - we don't support others
	};
	struct wav_chunk_info
	{
		uint8_t   chunk_id[4];  ///< "fmt " signature
		uint32_t  chunk_size;   ///< FMT chunk size
	};
	struct wav_fmt_chunk
	{
		uint16_t  audio_format; ///< 1 = PCM
		uint16_t  num_channels; ///< 1 = mono, 2 = stereo
		uint32_t  sample_rate;  ///< in Hz (number of samples per second)
		uint32_t  bytes_per_second; ///< = (sample_rate * bits_per_sample * num_channels) / 8
		uint16_t  block_align; ///< 1 = 8-bit mono, 2 = 16-bit mono, 4 = 16-bit stereo
		uint16_t  bits_per_sample; ///< (bit size * channels) / 8
	};
	void*     audio_data = nullptr;
	uint32_t  data_size = 0;
	uint32_t  sample_rate = 0;
	uint32_t  bytes_per_second = 0;
	uint16_t  num_channels = 0;
	uint16_t  bits_per_sample = 0;
	{
		const char*  filename = "605559_Spice-Refinery.wav";
		FILE* fp = fopen(filename, "rb");
		if ( fp == nullptr )
		{
			std::printf("File could not be opened: %s\n", filename);
			return;
		}
		fseek(fp, 0, SEEK_SET);

		size_t  rd;
		riff_header  hdr;
		char    fmt_id[]  = "fmt ";
		char    data_id[] = "data";

		rd = fread(&hdr, sizeof(hdr), 1, fp);
		wav_chunk_info  chunk;

		while ( (rd = fread(&chunk, sizeof(chunk), 1, fp)) > 0 )
		{
			if ( memcmp(&chunk.chunk_id, fmt_id, sizeof(chunk.chunk_id)) == 0 )
			{
				wav_fmt_chunk  fmt;
				fread(&fmt, sizeof(fmt), 1, fp);
				num_channels = fmt.num_channels;
				sample_rate = fmt.sample_rate;
				bytes_per_second = fmt.bytes_per_second;
				bits_per_sample = fmt.bits_per_sample;
			}
			else if ( memcmp(&chunk.chunk_id, data_id, sizeof(chunk.chunk_id)) == 0 )
			{
				data_size = chunk.chunk_size;
				audio_data = malloc(data_size);
				if ( audio_data == nullptr )
				{
					std::printf("malloc failed\n");
					fclose(fp);
					return;
				}
				std::printf("Audio size = %u\n", data_size);
				size_t  rd = fread(audio_data, 1, data_size, fp);
				std::printf("Read %zu\n", rd);
				break;
			}
			else
			{
				fseek(fp, chunk.chunk_size, SEEK_CUR);
			}
		}
		fclose(fp);
	}

#if TZK_USING_OPENALSOFT
	CheckOpenAL(sample_rate, bits_per_sample, num_channels, data_size, audio_data);
#endif
}


#if TZK_USING_OGGOPUS

static void print_duration(FILE* _fp, ogg_int64_t _nsamples, int _frac)
{
	ogg_int64_t seconds;
	ogg_int64_t minutes;
	ogg_int64_t hours;
	ogg_int64_t days;
	ogg_int64_t weeks;
	_nsamples += _frac ? 24 : 24000;
	seconds = _nsamples / 48000;
	_nsamples -= seconds * 48000;
	minutes = seconds / 60;
	seconds -= minutes * 60;
	hours = minutes / 60;
	minutes -= hours * 60;
	days = hours / 24;
	hours -= days * 24;
	weeks = days / 7;
	days -= weeks * 7;
	if ( weeks )fprintf(_fp, "%liw", (long)weeks);
	if ( weeks || days )fprintf(_fp, "%id", (int)days);
	if ( weeks || days || hours )
	{
		if ( weeks || days )fprintf(_fp, "%02ih", (int)hours);
		else fprintf(_fp, "%ih", (int)hours);
	}
	if ( weeks || days || hours || minutes )
	{
		if ( weeks || days || hours )fprintf(_fp, "%02im", (int)minutes);
		else fprintf(_fp, "%im", (int)minutes);
		fprintf(_fp, "%02i", (int)seconds);
	}
	else fprintf(_fp, "%i", (int)seconds);
	if ( _frac )fprintf(_fp, ".%03i", (int)(_nsamples / 48));
	fprintf(_fp, "s");
}

static void print_size(FILE* _fp, opus_int64 _nbytes, int _metric,
	const char* _spacer)
{
	static const char SUFFIXES[7] = { ' ','k','M','G','T','P','E' };
	opus_int64 val;
	opus_int64 den;
	opus_int64 round;
	int        base;
	int        shift;
	base = _metric ? 1000 : 1024;
	round = 0;
	den = 1;
	for ( shift = 0; shift < 6; shift++ )
	{
		if ( _nbytes < den * base - round )break;
		den *= base;
		round = den >> 1;
	}
	val = (_nbytes + round) / den;
	if ( den > 1 && val < 10 )
	{
		if ( den >= 1000000000 )val = (_nbytes + (round / 100)) / (den / 100);
		else val = (_nbytes * 100 + round) / den;
		fprintf(_fp, "%li.%02i%s%c", (long)(val / 100), (int)(val % 100),
			_spacer, SUFFIXES[shift]);
	}
	else if ( den > 1 && val < 100 )
	{
		if ( den >= 1000000000 )val = (_nbytes + (round / 10)) / (den / 10);
		else val = (_nbytes * 10 + round) / den;
		fprintf(_fp, "%li.%i%s%c", (long)(val / 10), (int)(val % 10),
			_spacer, SUFFIXES[shift]);
	}
	else fprintf(_fp, "%li%s%c", (long)val, _spacer, SUFFIXES[shift]);
}

void
CheckOggOpus()
{
	int  err;
	OggOpusFile*  opus_file;

	const char*  filename = "sample3.opus";
	FILE* fp = fopen(filename, "rb");
	if ( fp == nullptr )
	{
		std::printf("File could not be opened: %s\n", filename);
		return;
	}

	OpusFileCallbacks opus_callbacks = {
		nullptr, nullptr, nullptr, nullptr
	};
	
	opus_file = op_open_callbacks(op_fdopen(&opus_callbacks, _dup(_fileno(fp)), "rb"), &opus_callbacks, nullptr, 0, &err);
	
	if ( err != 0 )
	{

		return;
	}

	std::vector<unsigned char>  data;
	ogg_int64_t   duration = 0;
	int           rc;
	int           output_seekable;

	output_seekable = fseek(fp, 0, SEEK_CUR) != -1;

	ogg_int64_t pcm_offset;
	ogg_int64_t pcm_print_offset;
	ogg_int64_t nsamples = 0;
	opus_int32  bitrate;
	int         prev_li = -1;
	int num_channels = 0;
	
	pcm_offset = op_pcm_tell(opus_file);

	if ( pcm_offset != 0 )
	{
		fprintf(stderr, "Non-zero starting PCM offset: %li\n", (long)pcm_offset);
	}
	pcm_print_offset = pcm_offset - 48000;
	bitrate = 0;
	for ( ;; )
	{
		ogg_int64_t   next_pcm_offset;
		opus_int16    pcm[120 * 48 * 2];
		unsigned char out[120 * 48 * 2 * 2]; // 120ms * sample rate * sample_width * num_channels
		int           li;
		int           si;

		/*Although we would generally prefer to use the float interface, WAV
		   files with signed, 16-bit little-endian samples are far more
		   universally supported, so that's what we output.*/
		rc = op_read_stereo(opus_file, pcm, sizeof(pcm) / sizeof(*pcm));
		if ( rc == OP_HOLE )
		{
			fprintf(stderr, "\nHole detected! Corrupt file segment?\n");
			continue;
		}
		else if ( rc < 0 )
		{
			return;
		}
		li = op_current_link(opus_file);
		if ( li != prev_li )
		{
			const OpusHead* head;
			const OpusTags* tags;
			int             binary_suffix_len;
			int             ci;
			/* We found a new link. Print out some information.*/
			fprintf(stderr, "Decoding link %i:                          \n", li);
			head = op_head(opus_file, li);
			num_channels = head->channel_count;
			fprintf(stderr, "  Channels: %i\n", head->channel_count);
			if ( op_seekable(opus_file) )
			{
				ogg_int64_t duration;
				opus_int64  size;
				duration = op_pcm_total(opus_file, li);
				fprintf(stderr, "  Duration: ");
				print_duration(stderr, duration, 3);
				fprintf(stderr, " (%li samples @ 48 kHz)\n", (long)duration);
				size = op_raw_total(opus_file, li);
				fprintf(stderr, "  Size: ");
				print_size(stderr, size, 0, "");
				fprintf(stderr, "\n");
			}
			if ( head->input_sample_rate )
			{
				fprintf(stderr, "  Original sampling rate: %lu Hz\n",
					(unsigned long)head->input_sample_rate);
			}
			tags = op_tags(opus_file, li);
			fprintf(stderr, "  Encoded by: %s\n", tags->vendor);
			for ( ci = 0; ci < tags->comments; ci++ )
			{
				const char* comment;
				comment = tags->user_comments[ci];
				if ( opus_tagncompare("METADATA_BLOCK_PICTURE", 22, comment) == 0 )
				{
					OpusPictureTag pic;
					int            err;
					err = opus_picture_tag_parse(&pic, comment);
					fprintf(stderr, "  %.23s", comment);
					if ( err >= 0 )
					{
						fprintf(stderr, "%u|%s|%s|%ux%ux%u", pic.type, pic.mime_type,
							pic.description, pic.width, pic.height, pic.depth);
						if ( pic.colors != 0 )fprintf(stderr, "/%u", pic.colors);
						if ( pic.format == OP_PIC_FORMAT_URL )
						{
							fprintf(stderr, "|%s\n", pic.data);
						}
						else
						{
							fprintf(stderr, "|<%u bytes of image data>\n", pic.data_length);
						}
						opus_picture_tag_clear(&pic);
					}
					else fprintf(stderr, "<error parsing picture tag>\n");
				}
				else fprintf(stderr, "  %s\n", tags->user_comments[ci]);
			}
			if ( opus_tags_get_binary_suffix(tags, &binary_suffix_len) != NULL )
			{
				fprintf(stderr, "<%u bytes of unknown binary metadata>\n",
					binary_suffix_len);
			}
			fprintf(stderr, "\n");
			if ( !op_seekable(opus_file) )
			{
				pcm_offset = op_pcm_tell(opus_file) - rc;
				if ( pcm_offset != 0 )
				{
					fprintf(stderr, "Non-zero starting PCM offset in link %i: %li\n",
						li, (long)pcm_offset);
				}
			}
		}
		if ( li != prev_li || pcm_offset >= pcm_print_offset + 48000 )
		{
			opus_int32 next_bitrate;
			opus_int64 raw_offset;
			next_bitrate = op_bitrate_instant(opus_file);
			if ( next_bitrate >= 0 )bitrate = next_bitrate;
			raw_offset = op_raw_tell(opus_file);
			fprintf(stderr, "\r ");
			print_size(stderr, raw_offset, 0, "");
			fprintf(stderr, "  ");
			print_duration(stderr, pcm_offset, 0);
			fprintf(stderr, "  (");
			print_size(stderr, bitrate, 1, " ");
			fprintf(stderr, "bps)                    \r");
			pcm_print_offset = pcm_offset;
			fflush(stderr);
		}
		next_pcm_offset = op_pcm_tell(opus_file);
		if ( pcm_offset + rc != next_pcm_offset )
		{
			fprintf(stderr, "\nPCM offset gap! %li+%i!=%li\n",
				(long)pcm_offset, rc, (long)next_pcm_offset);
		}
		pcm_offset = next_pcm_offset;
		if ( rc <= 0 )
		{
			rc = 0;
			break;
		}
		/*Ensure the data is little-endian before writing it out.*/
		for ( si = 0; si < 2 * rc; si++ )
		{
			out[2 * si + 0] = (unsigned char)(pcm[si] & 0xFF);
			out[2 * si + 1] = (unsigned char)(pcm[si] >> 8 & 0xFF);
		}
		//if ( !fwrite(out, sizeof(*out) * 4 * rc, 1, stdout) )
		
		size_t  data_size = sizeof(*out) * 4 * rc;
		
		std::copy_n(out, data_size, std::back_inserter(data));

		
		nsamples += rc;
		prev_li = li;
	}
	if ( rc == 0 )
	{
		fprintf(stderr, "\nDone: played ");
		print_duration(stderr, nsamples, 3);
		fprintf(stderr, " (%li samples @ 48 kHz).\n", (long)nsamples);
	}
	
#if TZK_USING_OPENALSOFT
	// our openal func will free this memory, so copy it into fresh allocation to avoid crashing
	size_t  alloc = data.size();
	void*   audio_data = malloc(alloc);
	if ( audio_data == nullptr )
	{
		std::printf("malloc of %zu failed\n", alloc);
		return;
	}
	memcpy(audio_data, static_cast<void*>(data.data()), alloc);

	CheckOpenAL(48000, 16, num_channels, alloc, audio_data);
#endif

	op_free(opus_file);
}

#endif  // TZK_USING_OGGOPUS


#if TZK_USING_OGGVORBIS

void
CheckOggVorbis()
{
	uint32_t sample_rate;
	uint16_t bits_per_sample = 16; // vorbis is 16-bit samples only
	uint16_t num_channels;
	uint32_t data_size;
	void* audio_data;

	int    sample_width = 2; // typically 2, 16-bit samples (1 for 8-bit)
	int    signed_data = 1;  // typically 1, signed
	int    endianess = 0;    // typically 0, little-endian
	int    bit_stream = 0;   // ov logic
	long   bytes;
	char   buffer[65536];     // read in 4KB chunks, common sector size
	std::vector<char>  data;

	const char* filename = "410903_It_s_A_Mystery.ogg";
	FILE* fp = fopen(filename, "rb");
	if ( fp == nullptr )
	{
		std::printf("File could not be opened: %s\n", filename);
		return;
	}

	OggVorbis_File  ovfile;
	int  rc;
#if _WIN32  // CRT linking mismatch prevention
	if ( (rc = ov_open_callbacks(fp, &ovfile, nullptr, 0, OV_CALLBACKS_DEFAULT)) != 0 )
#else
	if ( (rc = ov_open(fp, &ovfile, nullptr, 0)) != 0 )
#endif
	{
		std::printf("ov_open failed: %d\n", rc);
		return;
	}

#if 0  // we determine format in AL
	if ( ovfile.vi->channels == 1 )
		format = AL_FORMAT_MONO16;
	else
		format = AL_FORMAT_STEREO16;
#endif

	sample_rate = ovfile.vi->rate;
	num_channels = ovfile.vi->channels;

	// could pre-size data to reduce allocations
	data.reserve(1000000);// 1mil bytes

	do
	{
		// read decoded sound data
		bytes = ov_read(&ovfile, buffer, sizeof(buffer), endianess, sample_width, signed_data, &bit_stream);
		switch ( bytes )
		{
		case OV_HOLE:
			std::printf("ov_read failed: OV_HOLE (data interruption)\n");
			break;
		case OV_EBADLINK:
			std::printf("ov_read failed: OV_EBADLINK (invalid stream section)\n");
			break;
		case OV_EINVAL:
			std::printf("ov_read failed: OV_EINVAL (initial headers unreadable)\n");
			break;
		case 0:
			// eof
			break;
		default:
			// good read, append to end
			std::copy_n(buffer, bytes, std::back_inserter(data));
			break;
		}
	} while ( bytes > 0 );

	ov_clear(&ovfile);

	// this is quick testing, not production :)
	data_size = data.size();
	// our openal func will free this memory, so copy it into fresh allocation to avoid crashing
	audio_data = malloc(data_size);
	if ( audio_data == nullptr )
	{
		std::printf("malloc of %u failed\n", data_size);
		return;
	}
	memcpy(audio_data, static_cast<void*>(data.data()), data_size);
	//audio_data = std::data(data);

#if TZK_USING_OPENALSOFT
	CheckOpenAL(sample_rate, bits_per_sample, num_channels, data_size, audio_data);
#endif
}

#endif  // TZK_USING_OGGVORBIS


#if TZK_USING_OPENALSOFT

void
CheckOpenAL(
	uint32_t sample_rate,
	uint16_t bits_per_sample,
	uint16_t num_channels,
	uint32_t data_size,
	void* audio_data
)
{
	ALCdevice*  al_device = nullptr;
	ALCcontext* al_context = nullptr;
	ALenum  err;
	ALuint  buffer;
	ALuint  source;
	ALint   source_state;

	_putenv_s("ALSOFT_LOGLEVEL", "3");

	// init OpenAL
	if ( (al_device = alcOpenDevice(nullptr)) != nullptr )
	{
		if ( (al_context = alcCreateContext(al_device, nullptr)) == nullptr )
		{
			std::printf("[OpenAL] alcOpenDevice failed\n");
			return;
		}
		if ( !alcMakeContextCurrent(al_context) )
		{
			std::printf("[OpenAL] alcMakeContextCurrent failed\n");
			return;
		}
	}
	else
	{
		std::printf("[OpenAL] alcOpenDevice failed\n");
		return;
	}
	// bind
	{
		ALenum  format = AL_FORMAT_MONO16;
		bool    stereo = num_channels > 1;
		if ( bits_per_sample == 16 )
		{
			format = stereo ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16;
		}
		else if ( bits_per_sample == 8 )
		{
			format = stereo ? AL_FORMAT_STEREO8 : AL_FORMAT_MONO8;
		}
		else
		{
			std::printf("Unknown format\n");
		}
		alGetError();
		alGenSources(1, &source);
		if ( (err = alGetError()) != AL_NO_ERROR )
			std::printf("[OpenAL] alGenSources failed\n");
		alGenBuffers(1, &buffer);
		if ( (err = alGetError()) != AL_NO_ERROR )
			std::printf("[OpenAL] alGenBuffers failed\n");
		alBufferData(buffer, format, audio_data, data_size, sample_rate);
		if ( (err = alGetError()) != AL_NO_ERROR )
			std::printf("[OpenAL] alBufferData failed\n");
		alSourcei(source, AL_BUFFER, buffer);
		if ( (err = alGetError()) != AL_NO_ERROR )
			std::printf("[OpenAL] alSourcei failed\n");
	}
	// hold for playback
	{
#if 0 // none of these are required for 'static' sounds
		ALfloat  ori[] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f };

		alListener3f(AL_POSITION, 0, 0, 1.0f);
		alListener3f(AL_VELOCITY, 0, 0, 0);
		alListenerfv(AL_ORIENTATION, ori);

		alSourcef(source, AL_PITCH, 1);
		alSourcef(source, AL_GAIN, 1);
		alSource3f(source, AL_POSITION, 0, 0, 0);
		alSource3f(source, AL_VELOCITY, 0, 0, 0);
		alSourcei(source, AL_LOOPING, AL_FALSE);
#endif
		std::printf(
			"Audio Data: sample_rate(%u) bits_per_sample(%u) channels(%u) size(%u) memory(%" PRIxPTR ")\n",
			sample_rate, bits_per_sample, num_channels, data_size, audio_data
		);
		std::printf("[OpenAL] playing source %u\n", source);
		alSourcePlay(source);
		if ( (err = alGetError()) != AL_NO_ERROR )
			std::printf("[OpenAL] alSourcePlay failed\n");

		do
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
			alGetSourcei(source, AL_SOURCE_STATE, &source_state);
			if ( (err = alGetError()) != AL_NO_ERROR )
				std::printf("[OpenAL] alGetSourcei failed\n");
		} while ( source_state == AL_PLAYING );
	}
	// cleanup
	{
		alDeleteSources(1, &source);
		alDeleteBuffers(1, &buffer);
		alcMakeContextCurrent(nullptr);

		if ( al_context != nullptr )
		{
			alcDestroyContext(al_context);
		}
		if ( al_device != nullptr )
		{
			alcCloseDevice(al_device);
		}
		if ( audio_data != nullptr )
		{
			free(audio_data);
		}
	}

	std::printf("[OpenAL] Completed\n");
}

#endif // TZK_USING_OPENALSOFT


#if TZK_USING_PUGIXML && TZK_USING_SQLITE

void
ChannelCallback(
	std::string& feed_name,
	pugi::xml_node channel_node
)
{
	/*
	 * We can reuse the prepared statements by using sqlite3_reset()
	 * Not doing it here due to callback and quick-poc state, fresh
	 * creation each loop
	 */

	std::string  title = "title";
	std::string  link = "link";
	std::string  desc = "description";
	std::string  item = "item";
	std::string  pub_date = "pubDate";

	// this callback is only interested in each item

	for ( auto n : channel_node.children(item.c_str()) )
	{
		pugi::xml_node  item_title_node = n.child(title.c_str());
		pugi::xml_node  item_link_node = n.child(link.c_str());
		pugi::xml_node  item_desc_node = n.child(desc.c_str());
		pugi::xml_node  item_pubdate_node = n.child(pub_date.c_str());

		if ( item_title_node == nullptr )
		{
			std::printf("Item 'title' node not found - required\n");
			continue;
		}
		if ( item_link_node == nullptr )
		{
			std::printf("Item 'link' node not found - required\n");
			continue;
		}
		if ( item_desc_node == nullptr )
		{
			std::printf("Item 'description' node not found - required\n");
			continue;
		}

		std::string  data_title = item_title_node.child_value();
		std::string  data_link = item_link_node.child_value();
		std::string  data_desc = item_desc_node.child_value();
		std::string  data_pubdate = item_pubdate_node.child_value();

		using namespace trezanik::core::aux;

		unsigned char  title_hash[sha1_hash_size];
		unsigned char  link_hash[sha1_hash_size];
		unsigned char  desc_hash[sha1_hash_size];
		unsigned char  amalgamated[sha1_hash_size * 3];

		sha1_of_buffer(reinterpret_cast<const unsigned char*>(data_title.c_str()), data_title.length(), title_hash);
		sha1_of_buffer(reinterpret_cast<const unsigned char*>(data_link.c_str()), data_link.length(), link_hash);
		sha1_of_buffer(reinterpret_cast<const unsigned char*>(data_desc.c_str()), data_desc.length(), desc_hash);
		memcpy(&amalgamated[sha1_hash_size * 0], title_hash, sizeof(title_hash));
		memcpy(&amalgamated[sha1_hash_size * 1], link_hash, sizeof(link_hash));
		memcpy(&amalgamated[sha1_hash_size * 2], desc_hash, sizeof(desc_hash));

		unsigned char  amalgamated_hash[sha1_hash_size];

		sha1_of_buffer(amalgamated, sha1_hash_size * 3, amalgamated_hash);

		char  hashval[sha1_string_buffer_size];
		sha1_to_string(amalgamated_hash, hashval, sizeof(hashval));

		// track this value for later comparison
		g_all_hashvals_for_feed.emplace_back(hashval);

		
		int    rc;
		const char*  tail = nullptr;
		int    auto_size = -1;

		// for looking up this hash value
		sqlite3_stmt* stmt_hash_search;

		std::string  sel = "SELECT ROWID,HashVal FROM FeedData WHERE HashVal = ?;";

		if (( rc = sqlite3_prepare_v2(g_db, sel.c_str(), auto_size, &stmt_hash_search, &tail)) != SQLITE_OK )
		{
			printf("sqlite3_prepare_v2 failed: error %i (%s)\n", rc, sqlite3_errmsg(g_db));
		}
		if (( rc = sqlite3_bind_text(stmt_hash_search, 1, hashval, auto_size, nullptr)) != SQLITE_OK )
		{
			printf("sqlite3_bind_text failed: error %i (%s)\n", rc, sqlite3_errmsg(g_db));
		}
		if (( rc = sqlite3_step(stmt_hash_search)) == SQLITE_ROW )
		{
			// found this entry in the DB already 
			/*int  col = 0;
			std::string  col_text = reinterpret_cast<const char*>(sqlite3_column_text(stmt_hash_search, col));
			printf("Already found at row: %s\n", col_text.c_str());
			int row = std::atoi(col_text.c_str());*/
			sqlite3_finalize(stmt_hash_search);
			continue;
		}

		sqlite3_finalize(stmt_hash_search);


		// entry not found, prep for insert
		std::string  sel_uri = "SELECT ID FROM Feeds WHERE URI = ?;";

		sqlite3_stmt* stmt_select_feed;

		if (( rc = sqlite3_prepare_v2(g_db, sel_uri.c_str(), auto_size, &stmt_select_feed, &tail)) != SQLITE_OK )
		{
			printf("sqlite3_prepare_v2 failed: error %i (%s)\n", rc, sqlite3_errmsg(g_db));
		}
		if (( rc = sqlite3_bind_text(stmt_select_feed, 1, feed_name.c_str(), auto_size, nullptr)) != SQLITE_OK )
		{
			printf("sqlite3_bind_text failed: error %i (%s)\n", rc, sqlite3_errmsg(g_db));
		}
		// stop on errors

		// performs actual execution
		if ( (rc = sqlite3_step(stmt_select_feed)) != SQLITE_ROW )
		{
			sqlite3_finalize(stmt_select_feed);
			continue;
		}


		int  col = 0;
		std::string  col_text = reinterpret_cast<const char*>(sqlite3_column_text(stmt_select_feed, col));
		printf("Feed ID = %s\n", col_text.c_str());
		int  feed_id = std::atoi(col_text.c_str());

		sqlite3_finalize(stmt_select_feed);
		

		sqlite3_stmt* stmt_insert;

		std::string  insert = "INSERT INTO FeedData VALUES(?, ?, ?, ?, ?, ?);";

		if (( rc = sqlite3_prepare_v2(g_db, insert.c_str(), auto_size, &stmt_insert, &tail)) != SQLITE_OK )
		{
			printf("sqlite3_prepare_v2 failed: error %i (%s)\n", rc, sqlite3_errmsg(g_db));
		}
		if (( rc = sqlite3_bind_int(stmt_insert, 1, feed_id)) != SQLITE_OK )
		{
			printf("sqlite3_bind_int failed: error %i (%s)\n", rc, sqlite3_errmsg(g_db));
		}
		if (( rc = sqlite3_bind_text(stmt_insert, 2, hashval, auto_size, nullptr)) != SQLITE_OK )
		{
			printf("sqlite3_bind_text failed: error %i (%s)\n", rc, sqlite3_errmsg(g_db));
		}
		if (( rc = sqlite3_bind_text(stmt_insert, 3, data_title.c_str(), auto_size, nullptr)) != SQLITE_OK )
		{
			printf("sqlite3_bind_text failed: error %i (%s)\n", rc, sqlite3_errmsg(g_db));
		}
		if (( rc = sqlite3_bind_text(stmt_insert, 4, data_link.c_str(), auto_size, nullptr)) != SQLITE_OK )
		{
			printf("sqlite3_bind_text failed: error %i (%s)\n", rc, sqlite3_errmsg(g_db));
		}
		if (( rc = sqlite3_bind_text(stmt_insert, 5, data_desc.c_str(), auto_size, nullptr)) != SQLITE_OK )
		{
			printf("sqlite3_bind_text failed: error %i (%s)\n", rc, sqlite3_errmsg(g_db));
		}
		// if present, auto-null if nullptr
		if (( rc = sqlite3_bind_text(stmt_insert, 6, data_pubdate.c_str(), auto_size, nullptr)) != SQLITE_OK )
		{
			printf("sqlite3_bind_text failed: error %i (%s)\n", rc, sqlite3_errmsg(g_db));
		}
		if (( rc = sqlite3_step(stmt_insert)) != SQLITE_DONE )
		{
			// expected for inserting duplicate hash values!
			printf("sqlite3_step failed: error %i (%s)\n", rc, sqlite3_errmsg(g_db));
		}
		else
		{
			int64_t last_row_id = sqlite3_last_insert_rowid(g_db);
			col = 0;
			printf("Inserted at Row %lli: %s\n", last_row_id, sqlite3_column_text(stmt_insert, col));
		}
		
		sqlite3_finalize(stmt_insert);

		// *** track all items regardless, and remove any existing DB entries
	}
}

#endif  // TZK_USING_PUGIXML && TZK_USING_SQLITE


#if TZK_USING_PUGIXML

typedef void  (*cb_each_channel)(std::string& feed_name, pugi::xml_node channel_node);

void
CheckPugiRSS(
	std::string& feed_name,
	const char* text,
	cb_each_channel callback
)
{
	pugi::xml_document  doc;
	pugi::xml_parse_result  res;

	res = doc.load_string(text);

	if ( res.status != pugi::status_ok )
	{
		printf("[pugixml] Failed to load RSS feed markup: %s\n", res.description());
		return;
	}

	// todo: verify xml version and encoding

	pugi::xml_node  root_node = doc.root();
	pugi::xml_node  rss_node = doc.child("rss");

	if ( rss_node == nullptr )
	{
		printf("No 'rss' node found\n");
		return;
	}
	/*if ( strcmp(root_node.name(), "rss") != 0 )
	{
		printf("Root node is not 'rss' - found '%s'\n", root_node.name());
		return;
	}*/

	pugi::xml_attribute  ver_attr = rss_node.attribute("version");

	if ( ver_attr.empty() )
	{
		printf("No 'version' attribute found in %s\n", root_node.name());
		return;
	}
	else
	{
		ver_attr.value(); // check for 0.91 or 2.0, only support these
	}

	pugi::xml_node  channel_node = rss_node.child("channel");
	if ( channel_node == nullptr )
	{
		printf("No 'channel' node found\n");
		return;
	}

	if ( callback != nullptr )
	{
		callback(feed_name, channel_node);
		return;
	}

	std::string  title = "title";
	std::string  link = "link";
	std::string  desc = "description";
	std::string  item = "item";

	// channel must have title, link, description
	pugi::xml_node  title_node = channel_node.child(title.c_str());
	pugi::xml_node  link_node = channel_node.child(link.c_str());
	pugi::xml_node  desc_node = channel_node.child(desc.c_str());

	if ( title_node == nullptr )
	{
		std::printf("Channel 'title' node not found - required\n");
		return;
	}
	if ( link_node == nullptr )
	{
		std::printf("Channel 'link' node not found - required\n");
		return;
	}
	if ( desc_node == nullptr )
	{
		std::printf("Channel 'description' node not found - required\n");
		return;
	}

	// channel optionally has (that we will support to a degree)
	std::string  category = "category";
	std::string  copyright = "copyright";
	std::string  docs = "docs";
	std::string  generator = "generator";
	std::string  language = "language";
	std::string  last_build = "lastBuildDate";
	std::string  pub_date = "pubDate";
	std::string  ttl = "ttl";

	// each channel can have one or more item elements, which have the same 3 nodes as above

	for ( auto n : channel_node.children(item.c_str()) )
	{
		pugi::xml_node  item_title_node = n.child(title.c_str());
		pugi::xml_node  item_link_node = n.child(link.c_str());
		pugi::xml_node  item_desc_node = n.child(desc.c_str());

		if ( item_title_node == nullptr )
		{
			std::printf("Item 'title' node not found - required\n");
			continue;
		}
		if ( item_link_node == nullptr )
		{
			std::printf("Item 'link' node not found - required\n");
			continue;
		}
		if ( item_desc_node == nullptr )
		{
			std::printf("Item 'description' node not found - required\n");
			continue;
		}

		std::printf("[Item]\n\tTitle = %s\n\tLink = %s\n\tDesc = %s\n", 
			item_title_node.child_value(),
			item_link_node.child_value(),
			item_desc_node.child_value()
		);

		// item optionally has
		std::string  author = "author";
		// category, again
		std::string  comments = "comments";
		std::string  enclosure = "enclosure";
		std::string  guid = "guid";
		std::string  pub_date = "pubDate";
	}

}
#endif




int
main(
	int argc,
	char** argv
)
{
	int  rc;

	// so we can find files, alongside this binary
	wchar_t  workingdir[MAX_PATH];
	wchar_t* r;
	::GetModuleFileName(nullptr, workingdir, _countof(workingdir));
	r = wcsrchr(workingdir, '\\');
	*++r = '\0';
	::SetCurrentDirectoryW(workingdir);


#if 0 // testing output, should be in unit tests
	trezanik::core::aux::ByteConversionFlags  flags = 0;
	flags |= trezanik::core::aux::byte_conversion_flags_si_units;
	//flags |= trezanik::core::aux::byte_conversion_flags_two_decimal;
	flags |= trezanik::core::aux::byte_conversion_flags_terminating_space;
	flags |= trezanik::core::aux::byte_conversion_flags_comma_separate;
	std::printf("%s\n", trezanik::core::aux::BytesToReadable(999, flags).c_str());
	std::printf("%s\n", trezanik::core::aux::BytesToReadable(1000, flags).c_str());
	std::printf("%s\n", trezanik::core::aux::BytesToReadable(1023, flags).c_str());
	std::printf("%s\n", trezanik::core::aux::BytesToReadable(1024, flags).c_str());
	std::printf("%s\n", trezanik::core::aux::BytesToReadable(317979, flags).c_str());
	std::printf("%s\n", trezanik::core::aux::BytesToReadable(1243885, flags).c_str());
	std::printf("%s\n", trezanik::core::aux::BytesToReadable(88954621, flags).c_str());
	std::printf("%s\n", trezanik::core::aux::BytesToReadable(18994184894, flags).c_str());
#endif

#if TZK_USING_FLAC
	if ( testing_flac )
	{
	}
#endif

#if TZK_USING_FREETYPE
	if ( testing_freetype ) {
	FT_Library  library;
	FT_Face     face;

	rc = FT_Init_FreeType(&library);
	if ( rc != FT_Err_Ok )
	{
		std::printf("FT_Init_FreeType() failed: %d\n", rc);
	}

	rc = FT_New_Face(library, "SQR721N.TTF", 0, &face);
	if ( rc != FT_Err_Ok )
	{
		std::printf("FT_New_Face() failed: %d\n", rc);
	}
	else
	{
		rc = FT_Set_Char_Size(face,    /* handle to face object           */
		0,       /* char_width in 1/64th of points  */
		16 * 64,   /* char_height in 1/64th of points */
		300,     /* horizontal device resolution    */
		300);   /* vertical device resolution      */

		FT_Done_Face(face);
	}
	FT_Done_FreeType(library);
	}
#endif

#if TZK_USING_IMGUI && TZK_USING_SDL
	if ( testing_imgui )
	{
		ImGuiContext*  ctx = nullptr;
		SDL_Window*    window = nullptr;
		SDL_Renderer*  renderer = nullptr;

		// init - we assume no failures as crashes/incompleteness will be the case!
		{
			SDL_SetMainReady();

			if ( (rc = SDL_Init(SDL_INIT_EVERYTHING)) != 0 )
			{
				std::printf("SDL_Init failed: %d\n", rc);
				return -1;
			}

			window = SDL_CreateWindow(
				"Dear ImGui Test",
				SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1024, 768, SDL_WINDOW_ALLOW_HIGHDPI
			);
			renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
			
			ctx = ImGui::CreateContext();
			ImGuiIO& io = ImGui::GetIO();

			io.IniFilename = nullptr;
			io.LogFilename = nullptr;

			io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls

			ImGui::StyleColorsDark();

			if ( !ImGui_ImplSDL2_InitForSDLRenderer(window, renderer) )
			{
				return -1;
			}
			if ( !ImGui_ImplSDLRenderer2_Init(renderer) )
			{
				return -1;
			}
		}

		SDL_StartTextInput();
		SDL_Event  evt;
		bool  quit = false;
		bool  show_demo = true;

		bool  show_popup_primary = true;
		bool  show_popup_secondary = false;
		bool  show_popup_tertiary = false;

#if 0	// when we were testing ImNodeFlow
		ImFlow::ImNodeFlow  nodeflow;
		bool  once = false;
		std::shared_ptr<ImFlow::BaseNode>  basenode;
#endif

		// loop; testing here focuses on display/visual, not input
		while ( !quit )
		{
			while ( SDL_PollEvent(&evt) )
			{
				ImGui_ImplSDL2_ProcessEvent(&evt);
				
				if ( evt.type == SDL_QUIT )
					quit = true;
			}
			
			ImGui_ImplSDLRenderer2_NewFrame();
			ImGui_ImplSDL2_NewFrame();
			ImGui::NewFrame();
			//-----------------------------------
			ImGui::ShowDemoWindow(&show_demo);
			
#if 0	// when we were testing ImNodeFlow
			if ( ImGui::Begin("nodegraph") )
			{
				if ( basenode != nullptr )
				{
					basenode = nodeflow.addNode<ImFlow::BaseNode>(ImVec2(20,20));
				}
				nodeflow.update();
			}
			ImGui::End();
#endif

#if 0
			ImVec2  wnd_size = ImVec2(240.f, 512.f);
			float   confirm_height = 20.f;
			float   button_height = 40.f;

			//ImGui::SetNextWindowPos(ImVec2(ImGui::GetWindowSize().x, ImGui::GetWindowSize().y));
			ImGui::SetNextWindowSize(wnd_size, ImGuiCond_Always);
			ImGui::SetNextWindowSizeConstraints(wnd_size, wnd_size);

			if ( show_popup_primary )
			{
				ImGui::OpenPopup("Popup Primary");

				if ( ImGui::BeginPopupModal("Popup Primary") )
				{
					if ( ImGui::Button("Secondary Popup") )
					{
						show_popup_secondary = true;
					}
					if ( ImGui::Button("Close") )
					{
						show_popup_primary = false;
						ImGui::CloseCurrentPopup();
					}
					

					if ( show_popup_secondary )
					{
						ImGui::OpenPopup("Popup Secondary");

						if ( ImGui::BeginPopupModal("Popup Secondary") )
						{
							if ( ImGui::Button("Close") )
							{
								show_popup_secondary = false;
								ImGui::CloseCurrentPopup();
							}
							ImGui::EndPopup();
						}
					}

					ImGui::EndPopup();
				}
			}
#endif


			
			//-----------------------------------
			ImGui::Render();
			//ImGuiIO& io = ImGui::GetIO();
			//SDL_RenderSetScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
			SDL_RenderClear(renderer);
			ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData());
			SDL_RenderPresent(renderer);
		}

		// cleanup
		{
			ImGui_ImplSDL2_Shutdown();
			ImGui_ImplSDLRenderer2_Shutdown();
			ImGui::DestroyContext();
			SDL_StopTextInput();
			SDL_DestroyRenderer(renderer);
			SDL_DestroyWindow(window);
			SDL_Quit();
		}
	}
#endif

#if TZK_USING_OGGOPUS
	if ( testing_oggopus )
	{
		// also runs against OpenAL if enabled, to confirm playback
		CheckOggOpus();
	}
	else
	{
		FILE*     fp = fopen("assets/audio/music/sample03.opus", "rb");
		OpusHead  oph;
		unsigned char  buffer[64];

		if ( fp == nullptr )
		{
			std::printf("File open failure\n");
		}
		else
		{
			int  rd = fread(buffer, sizeof(unsigned char), sizeof(buffer), fp);
			// as noted in documentation, up to initial is checked for validity up to that point
			if ( op_test(&oph, buffer, rd) != 0 )
			{
				std::printf("Not an opus file\n");
			}

			fclose(fp);
		}
	}
#endif

#if TZK_USING_OGGVORBIS
	if ( testing_oggvorbis )
	{
		// also runs against OpenAL if enabled, to confirm playback
		CheckOggVorbis();
	}
	else
	{
		OggVorbis_File*  ovf = (OggVorbis_File*)std::malloc(sizeof(OggVorbis_File));
		FILE*  fp = fopen("assets/audio/effects/press.ogg", "rb");

		if ( fp == nullptr )
		{
			std::printf("File open failure\n");
		}
		else
		{
			rc = ov_open_callbacks(fp, ovf, nullptr, 0, OV_CALLBACKS_STREAMONLY_NOCLOSE);
			if ( rc != 0 )
			{
				// expected as we pass in nullptr as file handle
				std::printf("[oggvorbis] ov_open_callbacks failed: %u\n", rc);
			}	
			else
			{
				rc = ov_clear(ovf);
			}

			fclose(fp);
		}

		std::free(ovf);
	}
#endif

#if TZK_USING_OPENALSOFT
	if ( testing_openalsoft )
	{
		// runs OpenAL check
		CheckWav();
	}
#endif

#if 0//TZK_USING_PNG
	png_bytepp   row_pointers;

	FILE*  fp = fopen("example.png", "rb");
	if ( fp != nullptr )
	{
		png_structp  png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
		png_infop    info_ptr = png_create_info_struct(png_ptr);

		png_init_io(png_ptr, fp);
		png_read_info(png_ptr, info_ptr);

		//png_read_png
		//row_pointers = png_get_rows(png_ptr, info_ptr);

		std::printf("[libpng] h=%u w=%u coltype=%u bd=%u\n",
			png_get_image_height(png_ptr, info_ptr),
			png_get_image_width(png_ptr, info_ptr),
			png_get_color_type(png_ptr, info_ptr),
			png_get_bit_depth(png_ptr, info_ptr)
		);

		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

		fclose(fp);
	}
#endif

#if TZK_USING_PUGIXML
	if ( testing_pugixml ) {
	//pugi::xml_document  doc;
	//pugi::xml_parse_result  res = doc.load_string(""); // intentional failure

		std::string  feed_name;
		{
		    FILE*  fp = fopen("rssfeed-content.xml", "r");
			if ( fp != nullptr )
			{
				fseek(fp, 0, SEEK_END);
				size_t  alloc = ftell(fp);
				fseek(fp, 0, SEEK_SET);

				char*  buf = (char*)malloc(alloc);
				fread(buf, 1, alloc, fp);
				fclose(fp);

				CheckPugiRSS(feed_name, buf, nullptr);

				free(buf);
			}
		}
	}
#endif

#if TZK_USING_SDL
	if ( testing_sdl ) {
	SDL_SetMainReady();

	if ( (rc = SDL_Init(SDL_INIT_EVERYTHING)) != 0 )
	{
		std::printf("SDL_Init failed: %d\n", rc);
	}

	SDL_Quit();
	}
#endif

	if ( testing_sqlite )
	{
		printf("SQLite version: %s\n", sqlite3_libversion());

		//sqlite3_open(":memory:", &g_db);
		//sqlite3_close(g_db);

				
		// get from disk
		if ( sqlite3_open_v2(g_dbfpath, &g_db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr) != SQLITE_OK )
		{
			// this automatically attempts to create the db if it doesn't already exist
			printf("Failed to open RSS database '%s'; %s", g_dbfpath, sqlite3_errmsg(g_db));
		}
		else
		{
			int    rc;
			char*  errmsg = nullptr;
			std::string   create_table_feeds =
				"CREATE TABLE IF NOT EXISTS \"Feeds\" ("
				"'ID'	INTEGER NOT NULL,"
				"'URI'	TEXT NOT NULL UNIQUE,"
				"'Title'	TEXT NOT NULL,"
				"'Link'	TEXT NOT NULL,"
				"'Description'	TEXT NOT NULL,"
				"PRIMARY KEY('ID')"
				");";
			std::string  create_table_feed_data = 
				"CREATE TABLE IF NOT EXISTS \"FeedData\" ("
				"'FeedID'	INTEGER NOT NULL,"
				"'HashVal'	TEXT NOT NULL UNIQUE,"
				"'Title'	TEXT NOT NULL,"
				"'Link'	TEXT NOT NULL,"
				"'Description'	TEXT NOT NULL,"
				"'Timestamp'	TEXT,"
				"FOREIGN KEY('FeedID') REFERENCES \"Feeds\"('ID')"
				");";

			// create tables
			// no callback, results not needed
			if ( (rc = sqlite3_exec(g_db, create_table_feeds.c_str(), nullptr, nullptr, &errmsg)) != SQLITE_OK )
			{
				printf("Table Feeds creation failed; %s\n", errmsg);
			}
			if ( errmsg != nullptr )
				sqlite3_free(errmsg);

			if ( (rc = sqlite3_exec(g_db, create_table_feed_data.c_str(), nullptr, nullptr, &errmsg)) != SQLITE_OK )
			{
				printf("Table FeedData creation failed; %s\n", errmsg);
			}
			if ( errmsg != nullptr )
				sqlite3_free(errmsg);
			

			// insert feeds
			std::string  create_feed = 
				"INSERT INTO Feeds ("
				"	URI, Title, Link, Description"
				") VALUES("
				"	'https://www.citrix.com/content/citrix/en_us/downloads/citrix-adc.rss',"
				"	'Citrix',"
				"	'citrix.com',"
				"	'Citrix ADC'"
				");"
				;
			if (( rc = sqlite3_exec(g_db, create_feed.c_str(), nullptr, nullptr, &errmsg)) != SQLITE_OK )
			{
				printf("Table Feeds population failed; %s\n", errmsg);
			}
			if ( errmsg != nullptr )
				sqlite3_free(errmsg);


			//===============

			std::string  feed_name = "https://www.citrix.com/content/citrix/en_us/downloads/citrix-adc.rss";
			FILE* fp = fopen("rssfeed-content.xml", "r");
			if ( fp != nullptr )
			{
				fseek(fp, 0, SEEK_END);
				size_t  alloc = ftell(fp);
				fseek(fp, 0, SEEK_SET);

				char* buf = (char*)malloc(alloc);
				fread(buf, 1, alloc, fp);
				fclose(fp);

				CheckPugiRSS(feed_name, buf, ChannelCallback);

				free(buf);
			}


			/*
			 * Now run through all the feed data in the database,
			 * comparing it to what we have returned from the server
			 * in the last execution run, and if there's entires
			 * that exist in the database but not the data, purge
			 * them from the db
			 */
			int  feed_id = -1;
			int  auto_size = -1;
			const char* tail = nullptr;
			sqlite3_stmt* stmt_feed_id_from_uri;

			std::string  citrix_rss = "https://www.citrix.com/content/citrix/en_us/downloads/citrix-adc.rss";
			std::string  sel = "SELECT ID FROM Feeds WHERE URI = ?;";

			int  index = 0;
			if ( (rc = sqlite3_prepare_v2(g_db, sel.c_str(), auto_size, &stmt_feed_id_from_uri, &tail)) != SQLITE_OK )
			{
				printf("sqlite3_prepare_v2 failed: error %i (%s)\n", rc, sqlite3_errmsg(g_db));
			}
			if ( (rc = sqlite3_bind_text(stmt_feed_id_from_uri, 1, citrix_rss.c_str(), auto_size, nullptr)) != SQLITE_OK )
			{
				printf("sqlite3_bind_text failed: error %i (%s)\n", rc, sqlite3_errmsg(g_db));
			}
			// stop on errors

			// performs actual execution
			rc = sqlite3_step(stmt_feed_id_from_uri);
			if ( rc == SQLITE_ROW )
			{
				int  col = 0;
				std::string  col_text = reinterpret_cast<const char*>(sqlite3_column_text(stmt_feed_id_from_uri, col));
				printf("%s\n", col_text.c_str());
				feed_id = std::atoi(col_text.c_str());
			}
			else
			{
				// stop
			}
			sqlite3_finalize(stmt_feed_id_from_uri);

			if ( feed_id != -1 )
			{
				/*
				 * Grab the row for each entry so when we come to cleanup,
				 * we can reference directly
				 * 
				 * 1 = Row
				 * 2 = FeedID
				 * 3 = HashVal
				 */
				sqlite3_stmt* stmt_hashval_feed;
				std::vector<std::tuple<int, int, std::string>>  hashval_data;
				std::string  sel = "SELECT ROWID,HashVal FROM FeedData WHERE FeedID = ?;";
				
				if ( (rc = sqlite3_prepare_v2(g_db, sel.c_str(), auto_size, &stmt_hashval_feed, &tail)) != SQLITE_OK )
				{
					printf("sqlite3_prepare_v2 failed: error %i (%s)\n", rc, sqlite3_errmsg(g_db));
					// stop
				}
				if ( (rc = sqlite3_bind_int(stmt_hashval_feed, 1, feed_id)) != SQLITE_OK )
				{
					printf("sqlite3_bind_text failed: error %i (%s)\n", rc, sqlite3_errmsg(g_db));
					// stop
				}

				while (( rc = sqlite3_step(stmt_hashval_feed)) == SQLITE_ROW )
				{
					int  col;
					int  row;
					std::string  hashval;
					std::string  data;

					col = 0;
					data = reinterpret_cast<const char*>(sqlite3_column_text(stmt_hashval_feed, col));
					row = std::atoi(data.c_str());

					col = 1;
					data = reinterpret_cast<const char*>(sqlite3_column_text(stmt_hashval_feed, col));
					hashval = data;

					hashval_data.emplace_back(row, feed_id, hashval);
				}

				sqlite3_finalize(stmt_hashval_feed);

				/*
				 * Now compare this to the feed data we have
				 */
				std::vector<int>  rows_to_remove;
				bool found;
				for ( auto& hd : hashval_data )
				{
					found = false;

					for ( auto& h : g_all_hashvals_for_feed )
					{
						if ( h == static_cast<std::string>(std::get<2>(hd)) )
						{
							found = true;
							break;
						}
					}

					if ( !found )
					{
						rows_to_remove.emplace_back(std::get<0>(hd));
					}
				}


				/*
				 * If there's anything left, cleanup the entries
				 */
				if ( !rows_to_remove.empty() )
				{
					std::string  del = "DELETE FROM FeedData WHERE ROWID IN (";

					for ( auto& r : rows_to_remove )
					{
						del += std::to_string(r);
						del += ",";
					}

					// safe, always at least one if we're here
					del[del.length() - 1] = ')';
					del += ";";

					if (( rc = sqlite3_exec(g_db, del.c_str(), nullptr, nullptr, &errmsg)) != SQLITE_OK )
					{
						printf("FeedData deletion failed; %s\n", errmsg);
					}
					if ( errmsg != nullptr )
						sqlite3_free(errmsg);
				}
			}



			//===============


			// trial data
			/*
Create the prepared statement object using sqlite3_prepare_v2().
Bind values to parameters using the sqlite3_bind_*() interfaces.
Run the SQL by calling sqlite3_step() one or more times.
Reset the prepared statement using sqlite3_reset() then go back to step 2. Do this zero or more times.
Destroy the object using sqlite3_finalize().
			 */
#if 0
			int  feed_id = -1;
			int  auto_size = -1;
			const char*  tail = nullptr;
			sqlite3_stmt*  stmt_feed_id_from_uri;
			sqlite3_stmt*  stmt_insert_into_feed_data;
			
			std::string  citrix_rss = "https://www.citrix.com/content/citrix/en_us/downloads/citrix-adc.rss";
			std::string  sel = "SELECT ID FROM Feeds WHERE URI = ?;";

			int  index = 0;
			if (( rc = sqlite3_prepare_v2(g_db, sel.c_str(), auto_size, &stmt_feed_id_from_uri, &tail)) != SQLITE_OK )
			{
				printf("sqlite3_prepare_v2 failed: error %i (%s)\n", rc, sqlite3_errmsg(g_db));
			}
			if (( rc = sqlite3_bind_text(stmt_feed_id_from_uri, 1, citrix_rss.c_str(), auto_size, nullptr)) != SQLITE_OK )
			{
				printf("sqlite3_bind_text failed: error %i (%s)\n", rc, sqlite3_errmsg(g_db));
			}
			// stop on errors

			// performs actual execution
			rc = sqlite3_step(stmt_feed_id_from_uri);
			if ( rc == SQLITE_ROW )
			{
				int  col = 0;
				std::string  col_text = reinterpret_cast<const char*>(sqlite3_column_text(stmt_feed_id_from_uri, col));
				printf("%s\n", col_text.c_str());
				feed_id = std::atoi(col_text.c_str());
			}
			else
			{
			// stop
			}

			if ( feed_id != -1 )
			{
			index = 0;
			std::string  title = "New - List of Apps";
			std::string  link = "https://www.citrix.com/downloads/citrix-adc/splunk-apps/list-of-apps.html";
			std::string  description = "New downloads are available for NetScaler";
			std::string  datetime = "Tue, 26 Mar 2024 14:30:00 -0400";
			std::string  insert = "INSERT INTO FeedData VALUES(?, ?, ?, ?, ?);";

			if (( rc = sqlite3_prepare_v2(g_db, insert.c_str(), auto_size, &stmt_insert_into_feed_data, &tail)) != SQLITE_OK )
			{
				printf("sqlite3_prepare_v2 failed: error %i (%s)\n", rc, sqlite3_errmsg(g_db));
			}
			if (( rc = sqlite3_bind_int(stmt_insert_into_feed_data, 1, feed_id)) != SQLITE_OK )
			{
				printf("sqlite3_bind_int failed: error %i (%s)\n", rc, sqlite3_errmsg(g_db));
			}
			if (( rc = sqlite3_bind_text(stmt_insert_into_feed_data, 2, title.c_str(), auto_size, nullptr)) != SQLITE_OK )
			{
				printf("sqlite3_bind_text failed: error %i (%s)\n", rc, sqlite3_errmsg(g_db));
			}
			if (( rc = sqlite3_bind_text(stmt_insert_into_feed_data, 3, link.c_str(), auto_size, nullptr)) != SQLITE_OK )
			{
				printf("sqlite3_bind_text failed: error %i (%s)\n", rc, sqlite3_errmsg(g_db));
			}
			if (( rc = sqlite3_bind_text(stmt_insert_into_feed_data, 4, description.c_str(), auto_size, nullptr)) != SQLITE_OK )
			{
				printf("sqlite3_bind_text failed: error %i (%s)\n", rc, sqlite3_errmsg(g_db));
			}
			// if present, auto-null if nullptr
			if (( rc = sqlite3_bind_text(stmt_insert_into_feed_data, 5, datetime.c_str(), auto_size, nullptr)) != SQLITE_OK )
			{
				printf("sqlite3_bind_text failed: error %i (%s)\n", rc, sqlite3_errmsg(g_db));
			}
			rc = sqlite3_step(stmt_insert_into_feed_data);
			if ( rc == SQLITE_DONE )
			{
				int64_t last_row_id = sqlite3_last_insert_rowid(g_db);

				printf("Row %lli: %s\n", last_row_id, sqlite3_column_text(stmt_insert_into_feed_data, 0));
			}
			else
			{
				printf("sqlite3_step failed: error %i (%s)\n", rc, sqlite3_errmsg(g_db));
			}
			sqlite3_finalize(stmt_insert_into_feed_data);
			}

			sqlite3_finalize(stmt_feed_id_from_uri);
#endif
		}



		sqlite3_close(g_db);
	}

#if TZK_USING_STB
	int  w, h, comp;
	unsigned char* image = stbi_load("example.png", &w, &h, &comp, STBI_rgb);

	std::printf("[stb] png h=%u w=%u\n", h, w);

	stbi_image_free(image);
#endif

	
	//sleep(std::chrono::seconds(2));
	

	std::printf("Terminating");
	return EXIT_SUCCESS;
}

// SOUND.C - playing WAV files (22 KHz, 8 bits, stereo/mono, PCM)
// (MAX_STREAMS) sound streams mixing in real-time

#define SB_IRQ_VEC     (0x08 + sb_irq_number)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <dos.h>
#include <i86.h>
#include <math.h>

#include "SOUND.H"

SoundStream streams[MAX_STREAMS];
unsigned char mix_buffer[BUFFER_SIZE];

unsigned int  sb_port         = 0x220;
unsigned char sb_irq_number   = 7;
unsigned char sb_dma          = 1;
unsigned char sb_type         = 4;

// unsigned char *wav_data;
// unsigned int wav_data_size;

unsigned int dsp_version = 0;  // 0 = not defined

unsigned char *current_dma_target_ptr = NULL;

unsigned long phys;
unsigned int page, offset;

//const unsigned int dma_buffer_size = BUFFER_SIZE;

// Two buffers for double buffering

// unsigned char *dma_buffer = NULL;
// unsigned short dma_selector = 0;
// unsigned short dma_segment = 0;
unsigned char *dma_buffers[NUM_BUFFERS] = {NULL, NULL};
unsigned short dma_selectors[NUM_BUFFERS] = {0, 0};
unsigned short dma_segments[NUM_BUFFERS] = {0, 0};

unsigned int bytes_to_play; // Number of bytes for playback

unsigned int current_buffer = 0;        // The buffer that is currently playing
unsigned int next_buffer = 1;           // Buffer which we fill
unsigned int buffer_times = 0;
int playing_final_chunk = 0;
unsigned int first_run = 1;

volatile int is_playing = 0;
volatile int dma_block_finished_flag = 0;

volatile int playing_buffer = 0;
volatile int free_buffer = -1;

// Для дебага
volatile int counter = 0;

volatile unsigned char irq_debug_last_status = 0;
volatile unsigned char irq_debug_read_data = 0;

static unsigned long current_playback_position;

static void (__interrupt __far *old_irq_handler)() = NULL;

unsigned int wav_data_size = 0;
unsigned char *wav_data = NULL;

// **************************************************************
// Mixing different sounds
// **************************************************************

// Initialization
void mixer_init() {
    int i;

    memset(streams, 0, sizeof(streams));
    memset(mix_buffer, 0x80, BUFFER_SIZE); // 0x80 = тишина для 8-бит
    
    // Включаем все каналы
    for (i = 0; i < MAX_STREAMS; i++) {
        streams[i].playing = 0;
        streams[i].volume = 255;
    }

    dsp_reset();
    dsp_version = get_dsp_version();
    
    // Setup stereo on mixer output
    outp(sb_port + 0x04, 0x0E);
    outp(sb_port + 0x05, 0x03);
    delay(10);


    
    // Starting playback
    // dsp_write(DSP_CMD_8BIT_DMA_SETUP);
    // dsp_write(LOBYTE(BUFFER_SIZE - 1));
    // dsp_write(HIBYTE(BUFFER_SIZE - 1));

    //dsp_write(DSP_CMD_8BIT_SINGLE_OUTPUT);
    
    //dsp_write(DSP_MODE_UNSIGNED);
    //dsp_write(DSP_MODE_MONO);

    //set_sb_sample_rate(22050);
    set_sb_sample_rate(SAMPLE_RATE);

    // Configure DMA
    setup_dma_for_buffer(dma_buffers[0], 0);
    //setup_dma_controller_only(dma_buffers[0]);

    // This is sequential
    //dsp_write(DSP_CMD_8BIT_AUTO_OUTPUT);
    dsp_write(DSP_CMD_8BIT_SINGLE_OUTPUT);
    dsp_write(DSP_MODE_STEREO);

    // dsp_write(0xC0);
    // dsp_write(0x20);

    dsp_write(DSP_CMD_SPEAKER_ON);  // Turn on the speakers!
}

// Explicit channel stop
void mixer_stop(int channel) {
    if (channel < 0 || channel >= MAX_STREAMS) return;
    streams[channel].playing = 0;
    streams[channel].position = 0;
    streams[channel].data = NULL;
    streams[channel].size = 0;
    streams[channel].volume = 255;
}

// Full channel cleanup (to release resources)
void mixer_free_channel(int channel) {
    mixer_stop(channel);
}

// Search for an available or least prioritized sound stream
int find_free_channel(int priority) {
    int i;
    // If there are no free ones, we look for the ones with the lowest priority
    int lowest_priority = 999;
    int lowest_channel = -1;

    // First we find a free
    for (i = 0; i < MAX_STREAMS; i++) {
        if (!streams[i].playing) return i;
    }
    
    for (i = 0; i < MAX_STREAMS; i++) {
        if (streams[i].priority < lowest_priority) {
            lowest_priority = streams[i].priority;
            lowest_channel = i;
        }
    }
    
    // If the new sound is more important, replace it
    if (lowest_channel != -1 && priority < streams[lowest_channel].priority) {
        streams[lowest_channel].playing = 0;
        return lowest_channel;
    }
    
    return -1; // No free stream
}

// playing sound
int mixer_play_sound_ex(int channel, Sound *sound, int looping, int priority)
{
    if (channel == -1) {
        channel = find_free_channel(priority);
        if (channel == -1) return -1;
    }
    if (!sound || !sound->data || sound->size == 0) return -1;
    
    if (channel >= MAX_STREAMS) return -1;

    streams[channel].playing     = 0;
    streams[channel].data        = (unsigned char*)sound->data;
    streams[channel].position    = 0;
    streams[channel].size        = sound->size;
    streams[channel].sample_rate = sound->sample_rate;
    streams[channel].channels    = sound->channels;
    streams[channel].volume      = 255;
    streams[channel].volume_l    = 255;
    streams[channel].volume_r    = 255;
    streams[channel].playing     = 1;
    streams[channel].looping     = looping;
    streams[channel].priority    = priority;
    return channel;
}

// panning sounds
void mixer_set_pan(int channel, int pan)
{
    if (channel < 0 || channel >= MAX_STREAMS) return;
    if (pan < 0)   pan = 0;
    if (pan > 255) pan = 255;
    streams[channel].volume_l = 255 - pan;
    streams[channel].volume_r = pan;
}

// louder or softer
void mixer_set_volume_stereo(int channel, int vol_l, int vol_r)
{
    if (channel < 0 || channel >= MAX_STREAMS) return;
    streams[channel].volume_l = vol_l & 0xFF;
    streams[channel].volume_r = vol_r & 0xFF;
}

// Mixing all sound streams
void mixer_mix(unsigned char *output, unsigned int samples) {
    // samples = number of BYTES in the output buffer.
    // Output is always stereo (2 bytes per frame) as DSP is configured.

    int i, j;
    int active = 0;
    int sample, mixed;
    SoundStream *ch;
    int out_frames;

    out_frames = samples / 2;   // frames in the output buffer

    // Clean the output buffer (silence)
    memset(output, 0x80, samples);
    
    // Mix each active channel
    for (i = 0; i < MAX_STREAMS; i++) {
        if (!streams[i].playing) continue;
        
        active++;
        ch = &streams[i];
        
        //for (j = 0; j < samples; j++) {
        for (j = 0; j < out_frames; j++) {
            int L, R, m, out_l, out_r;


            // Checking the end of the sound
            if (ch->position >= ch->size) {
                if (ch->looping) {
                    ch->position = 0;
                } else {
                    ch->playing = 0;
                    ch->position = 0;
                    ch->data = NULL;
                    ch->size = 0;
                    //ch->volume = 255;
                    break;
                }
            }
            
            // Now stereo
            // we work in "symbolic" (centered) arithmetic:
            // subtract 128 from the sample, sum, then add 128 back;
            // clipping is done before adding 128, in the range [-128, 127];
            // we always consider the output buffer stereo — this is simpler and matches the DSP setting.
            if (ch->channels == 2) {
                // Stereo: two bytes per frame
                L = (int)ch->data[ch->position    ] - 128;
                R = (int)ch->data[ch->position + 1] - 128;
                ch->position += 2;
            } else {
                // Mono: duplicate in both channels
                m = (int)ch->data[ch->position] - 128;
                ch->position += 1;
                L = m;
                R = m;
            }

            // Applying channel volume
            L = (L * ch->volume_l) / 256;
            R = (R * ch->volume_r) / 256;

            // Mixing with clipping
            out_l = (int)output[j*2    ] - 128 + L;
            out_r = (int)output[j*2 + 1] - 128 + R;
            if (out_l >  127) out_l =  127;
            if (out_l < -128) out_l = -128;
            if (out_r >  127) out_r =  127;
            if (out_r < -128) out_r = -128;
            output[j*2    ] = (unsigned char)(out_l + 128);
            output[j*2 + 1] = (unsigned char)(out_r + 128);
        }
    }
}

// The main function of sound processing - called every frame
void mixer_process() {

    int next;

    if (dma_block_finished_flag) {
        dma_block_finished_flag = 0;
        
        next = (current_buffer + 1) % NUM_BUFFERS;
        mixer_mix(dma_buffers[next], BUFFER_SIZE);
    }
}

// **************************************************************
// Initial setup - parsing string BLASTER
// **************************************************************
int setup_sound_blaster() {

    char *p;
    char *blaster;
    unsigned int val;
    
    blaster = getenv("BLASTER");
    if (!blaster || !*blaster) {
        puts("[ERROR] Environment value BLASTER is not found! Using default values: A220 I7 D1 T4\n");
        return 0;
    }

    p = blaster;
    while (*p) {
        if (*p == 'A') {
            
            if (sscanf(p+1, "%x", &val) == 1) {
                sb_port = val;
            }
        }
        else if (*p == 'I') {
            
            if (sscanf(p+1, "%u", &val) == 1) {
                sb_irq_number = (unsigned char)val;
            }
        }
        else if (*p == 'D') {
            
            if (sscanf(p+1, "%u", &val) == 1) {
                sb_dma = (unsigned char)val;
            }
        }
        else if (*p == 'T') {
            
            if (sscanf(p+1, "%u", &val) == 1) {
                sb_type = (unsigned char)val;
            }
        }
        while (*p && *p != ' ') p++;
        while (*p == ' ') p++;
    }

    printf("[SETUP] BLASTER parsed: A%03X I%d D%d T%d\n",
           sb_port, sb_irq_number, sb_dma, sb_type);

    // here you can check the validity, for example
    if (sb_port < 0x210 || sb_port > 0x280 || (sb_port & 0x0F) != 0) {
        printf("[ERROR] Wrong port or address!\n");
        return -1;
    }

    return 1;   // success
}

// **************************************************************
// Working with interrupts
// **************************************************************
void __interrupt __far irq_handler() {

    int next;
    
    counter++;

    // DMA pause (without this and CONTINUE_DMA it did not work in 86box)
    dsp_write(DSP_CMD_PAUSE_DMA);   // 0xD0
    
    irq_debug_last_status = inp(DSP_DATA_AVAIL);   // что вернул статус
    while (inp(DSP_DATA_AVAIL) & 0x80) {
        irq_debug_read_data = inp(DSP_READ);
    }
    
    next = (current_buffer + 1) % NUM_BUFFERS;
    setup_dma_for_buffer(dma_buffers[next], next);
    
    current_buffer = next;

    dsp_write(DSP_CMD_8BIT_SINGLE_OUTPUT);          // 0xC0
    dsp_write(DSP_MODE_STEREO);

    dma_block_finished_flag = 1;

    // Resume DMA — but DSP will read from a NEW address
    
    // This did not work in 86box without this
    dsp_write(DSP_CMD_CONTINUE_DMA);   // 0xD4

    outp(0x20, 0x20);
}

void setup_irq_handler(void) {
    unsigned char mask;
    old_irq_handler = _dos_getvect(SB_IRQ_VEC);
    _dos_setvect(SB_IRQ_VEC, irq_handler);
    mask = inp(0x21); 	
    outp(0x21, mask & ~(1 << (sb_irq_number & 7)));
    printf("[IRQ] Handler installed for IRQ %d (vector 0x%02X)\n", sb_irq_number, SB_IRQ_VEC);
}

void cleanup_irq_handler(void) {
    unsigned char mask;
	if (old_irq_handler != NULL) {
        _dos_setvect(SB_IRQ_VEC, old_irq_handler);
        old_irq_handler = NULL;
    }
	
    mask = inp(0x21);
    outp(0x21, mask | (1 << (sb_irq_number & 7)));

    printf("[IRQ] Handler removed\n");
}

// **************************************************************
// Low-level functions for managing a memory segment
// below 640 kb for DMA buffer operation
// **************************************************************

unsigned char *alloc_low_dos_memory(
    unsigned long bytes,
    unsigned short *selector,
    unsigned short *segment){
    union REGS regs;
    *selector = 0;
    *segment = 0;
    regs.w.ax = 0x0100;
    regs.w.bx = (bytes + 15UL) / 16UL;
    int386(0x31, &regs, &regs);
    if (regs.x.cflag != 0) {
        printf("[DPMI] Alloc failed, AX=%04X\n", regs.w.ax);
        return NULL;
    }
    *segment  = regs.w.ax;
    *selector = regs.w.dx;
    printf("[DPMI] Allocated %lu bytes, segment %04X, selector %04X\n",
           bytes, *segment, *selector);
    return (unsigned char *)((unsigned long)(*segment) << 4);
}

void free_low_dos_memory(unsigned short selector) {
    union REGS regs;
    if (selector == 0) return;
    regs.w.ax = 0x0101; // Setting the DPMI function number: 0x0101 — Free DOS Memory Block.
    regs.w.dx = selector;
    int386(0x31, &regs, &regs);
    printf("\n");
    if (regs.x.cflag != 0) {
        printf("[DPMI] Free failed, AX=%04X\n", regs.w.ax);
    } else {
        printf("[DPMI] Freed selector %04X\n", selector);
    }
}

// **************************************************************
// Low-level functions for managing DSP Sound Blaster
// **************************************************************

// *** dsp_reset() ***
// Tries to reset the DSP chip Sound Blaster and checks what response it gave 0xAA
// Returns: nothing, but diagnostic information is output to the console
void dsp_reset()
{
    int timeout;
    unsigned char resp;

    puts("[DSP] Reset start...\n");

    // Write a unit to the register on 3 ms
    outp(DSP_RESET, 1);
    delay(3);

    // Write zero to the register for 100 ms
    outp(DSP_RESET, 0); delay(100);

    // Wait for the response
    timeout = 10000;
    while ((inp(DSP_DATA_AVAIL) & 0x80) == 0 && timeout > 0) {
        timeout--;
        delay(1);
    }

    if (timeout <= 0) {
        puts("[DSP] ERROR: timeout waiting for data after reset\n");
        return;
    }

    resp = inp(DSP_READ);
    printf("[DSP] Reset response: 0x%02X (waiting 0xAA)\n", resp);
    if (resp != 0xAA) {
        puts("[DSP] WARNING: bad reset response!\n");
    } else {
        puts("[DSP] Reset OK\n");
    }
}

void dsp_write(unsigned char cmd)
{
    int timeout = 1000000;

    while ((inp(DSP_STATUS) & 0x80) != 0 && timeout > 0) {
        timeout--;
    }

    if (timeout <= 0) {
        printf("[DSP] Write timeout for cmd 0x%02X\n", cmd);
        return;
    }

    outp(DSP_WRITE, cmd);
}

unsigned char dsp_read()
{
    int timeout = 10000;

    while ((inp(DSP_DATA_AVAIL) & 0x80) == 0 && timeout > 0) {
        timeout--;
    }

    if (timeout <= 0) {
        puts("[DSP] Read timeout\n");
        return 0;
    }

    return inp(DSP_READ);
}

void set_sb_sample_rate(unsigned int hz)
{
    if (hz == 0) {
        puts("[DSP] Error: sample rate 0 not allowed\n");
        return;
    }

    if (dsp_version == 0) {
        puts("[DSP] Version not detected, assuming SB Pro 2\n");
        dsp_version = 0x0301;  // fallback on SB Pro 2
    }

    printf("[DSP] Setting sample rate %u Hz (DSP v%d.%02d)\n",
           hz, (dsp_version >> 8), (dsp_version & 0xFF));

    if (dsp_version >= 0x0400) {  // SB16 and higher (DSP 4.xx)
        dsp_write(DSP_CMD_SET_PLAYBACK_RATE);
        dsp_write((unsigned char)(hz >> 8));   // high byte
        dsp_write((unsigned char)(hz & 0xFF)); // low byte
        printf("[SB16] Direct rate: %u Hz\n", hz);
    } else {  // SB Pro 2 and below (DSP 3.xx and earlier)
        // Time Constant = 256 - (1 000 000 / sample rate)
        unsigned int byte_rate = hz * 2;   // 2 bytes per frame
        unsigned char tc = (unsigned char)(256.0f - (1000000.0f / (float)byte_rate));
        dsp_write(DSP_CMD_SAMPLE_RATE);
        dsp_write(tc);
		//dsp_write(0xC1);
        printf("[SB Pro] Time Constant: 0x%02X (effective rate ~%u Hz)\n", tc, hz);

        // hz — frame rate (for example, 22050)
        // for stereo TC it is calculated from doubled byte rate
        
        
    }
}

unsigned int get_dsp_version()
{
    unsigned char hi, lo;

    printf("[DSP] Request version (0xE1)\n");
    dsp_write(DSP_CMD_GET_VERSION);

    hi = dsp_read();
    lo = dsp_read();

    printf("[DSP] Version: %d.%02d\n", (int)hi, (int)lo);
    return ((unsigned int)hi << 8) | (unsigned int)lo;
}

void dsp_set_stereo(int enable)
{
    // Order is important: first the command, then its parameter
    dsp_write(enable ? DSP_MODE_STEREO : DSP_MODE_MONO);
    printf("[DSP] Stereo mode: %s\n", enable ? "ON" : "OFF");
}

// ************************************************
// Working with buffers
// ************************************************

// Sets DMA for the specified buffer
// Only DMA controller programming. Without DSP commands.
void setup_dma_controller_only(unsigned char *buffer)
{
    phys = (unsigned long)buffer;
    page = phys >> 16;
    offset = phys & 0xFFFF;

    //_disable();

    outp(DMA_MASK_REGISTER, 0x04 | sb_dma);
    outp(DMA_CLEAR_BYTE_POINTER, 0x00);
    //outp(DMA_MODE_REGISTER, DMA_MODE_AUTO_INIT | sb_dma);   // 0x58
    outp(DMA_MODE_REGISTER, DMA_MODE_SINGLE_CYCLE | sb_dma);  // 0x48
    outp((sb_dma << 1) + 0, LOBYTE(offset));
    outp((sb_dma << 1) + 0, HIBYTE(offset));
    outp(DMA_PAGE_REGISTER, page);
    // outp((sb_dma << 1) + 1, LOBYTE(dma_buffer_size - 1));
    // outp((sb_dma << 1) + 1, HIBYTE(dma_buffer_size - 1));
    outp((sb_dma << 1) + 1, LOBYTE(BUFFER_SIZE - 1));
    outp((sb_dma << 1) + 1, HIBYTE(BUFFER_SIZE - 1));
    outp(DMA_MASK_REGISTER, sb_dma);

    //_enable();
}

// Full configuration: DMA + DSP block size. Used once.
void setup_dma_for_buffer(unsigned char *buffer, int buffer_index)
{
    setup_dma_controller_only(buffer);
    dsp_write(DSP_CMD_8BIT_DMA_SETUP);
    // dsp_write(LOBYTE(dma_buffer_size - 1));
    // dsp_write(HIBYTE(dma_buffer_size - 1));
    dsp_write(LOBYTE(BUFFER_SIZE - 1));
    dsp_write(HIBYTE(BUFFER_SIZE - 1));
}

// Fills the specified buffer with data from WAV
// Returns 1 if the end of the file is reached
int fill_buffer(unsigned char *buffer, int buffer_index)
{
    unsigned long to_copy;
    unsigned long part1;
    int is_final = 0;

    if (!buffer) {
        printf("[ERROR] buffer is NULL!\n");
        return 1;
    }

    //to_copy = dma_buffer_size;
    to_copy = BUFFER_SIZE;
    if (current_playback_position + to_copy <= wav_data_size) {
        memcpy(buffer, wav_data + current_playback_position, to_copy);
        current_playback_position += to_copy;
        is_final = 0;
    } else {
        part1 = wav_data_size - current_playback_position;
        memcpy(buffer, wav_data + current_playback_position, part1);
        // We fill the remaining buffer with silence (0x80 for 8-bit PCM)
        memset(buffer + part1, 0x80, to_copy - part1);
        current_playback_position = wav_data_size;
        is_final = 1;
        playing_final_chunk = 1;
    }

    return is_final;
}

// Switches buffers on interrupt
void swap_buffers()
{
    // Filling the next buffer
    int final = fill_buffer(dma_buffers[next_buffer], next_buffer);
    
    // Configure DMA for the next buffer
    setup_dma_for_buffer(dma_buffers[next_buffer], next_buffer);
    
    // Switch buffers
    current_buffer = next_buffer;
    next_buffer = (next_buffer + 1) % NUM_BUFFERS;
    
    // Showing progress
    //show_progress();
    
    if (final) {
        is_playing = 0;
    }
}

// Outputs a progress bar
void show_progress()
{
    unsigned int percent;
    unsigned int filled;
    unsigned int i;
    unsigned int progress_len = 50;

    percent = (unsigned int)((current_playback_position * 100UL) / wav_data_size);
    filled = (unsigned int)((current_playback_position * (unsigned long)progress_len) / wav_data_size);

    printf("\r[");
    for (i = 0; i < progress_len; i++) {
        if (i < filled)
            putchar(219);
        else
            putchar(177);
    }
    printf("] %3u%%  %lu / %lu   ", percent, current_playback_position, wav_data_size);
    fflush(stdout);
}

// Frees all buffers
void cleanup_buffers()
{
    int i;
    for (i = 0; i < NUM_BUFFERS; i++) {
        if (dma_selectors[i] != 0) {
            free_low_dos_memory(dma_selectors[i]);
            dma_buffers[i] = NULL;
            dma_selectors[i] = 0;
        }
    }
}

void sb_stop_playback(void) {
    dsp_write(DSP_CMD_EXIT_AUTO_INIT);                     /* exit auto-init */
    dsp_write(DSP_CMD_PAUSE_DMA);                     /* pause */
}

// Loading sound WAV-file
int load_wav(
    const char *filename,
    Sound *sound)
{
    FILE *soundFile;
    unsigned char header[44];
    unsigned short format, num_channels, bitspersample;
    unsigned int samplerate;
	unsigned int len;
    unsigned int i;
    unsigned char *wav_data;
    unsigned int wav_data_size;

    if (!sound) return -1;

    // Initializing in case of an error
    sound->data = NULL;
    sound->size = 0;
    sound->sample_rate = 0;
    sound->channels = 0;

    soundFile = fopen(filename, "rb");
    if (!soundFile) {
        printf("[ERROR] Cannot open %s\n", filename);
        return -2;
    }
	
	if (fread(header, 1, 44, soundFile) != 44 ||
		strncmp((char*)header,      "RIFF", 4) != 0 ||
		strncmp((char*)(header+8),  "WAVE", 4) != 0 ||
		strncmp((char*)(header+12), "fmt ", 4) != 0)
	{
		printf("[ERROR] This is not a WAV file: %s\n", filename);
		fclose(soundFile);
		return -3;
	}

	format        = *(unsigned short *)(header + 20);
	num_channels      = *(unsigned short *)(header + 22);
	samplerate    = *(unsigned int   *)(header + 24);
	bitspersample = *(unsigned short *)(header + 34);
	wav_data_size      = *(unsigned int   *)(header + 40);

	//if (format != 1 || num_channels != 1 || bitspersample != 8)
    if (format != 1 || (num_channels != 1 && num_channels != 2)
        || bitspersample != 8)
	{
		puts("[ERROR] Only unsigned 8-bit mono/stereo PCM is supported!\n");
		fclose(soundFile);
		return -4;
	}

	wav_data = malloc(wav_data_size);

    if (!wav_data) {
        printf("[ERROR] Out of memory for %s (%u bytes)\n", filename, wav_data_size);
        free(wav_data);
        fclose(soundFile);
        return -5;
    }

	//fread(wav_data, 1, wav_data_size, soundFile);
    if (fread(wav_data, 1, wav_data_size, soundFile) != wav_data_size) {
        printf("[ERROR] Short read for %s\n", filename);
        free(wav_data);
        fclose(soundFile);
        return -6;
    }
	fclose(soundFile);

	//printf("[MEM] Allocated %d bytes for %s\n", wav_data_size, filename);
    printf("[MEM] Allocated %d bytes for %s (%d ch, %d Hz)\n",
           wav_data_size, filename, num_channels, samplerate);

    sound->data = wav_data;
    sound->sample_rate = samplerate;
    sound->size = wav_data_size;
    sound->channels = num_channels;

    return 0; // success
}

// A short test signal is sent to DSP
void test_sound_generator() {
    int i;
    
    // Filling the buffer with a test tone (440 Hz)
    int freq = 440;
    int sample_rate = SAMPLE_RATE;
    int samples = BUFFER_SIZE;

    printf("[TEST] Generating test tone...\n");
    
    for (i = 0; i < samples; i++) {
        float angle = 2.0f * 3.14159f * freq * i / sample_rate;
        int sample = (int)(128 + 127 * sin(angle));
        if (sample > 255) sample = 255;
        if (sample < 0) sample = 0;
        dma_buffers[0][i] = (unsigned char)sample;
    }
    
    // Configure DMA
    setup_dma_for_buffer(dma_buffers[0], 0);
    
    // Starting playback
    dsp_write(DSP_CMD_8BIT_DMA_SETUP);
    dsp_write(LOBYTE(BUFFER_SIZE - 1));
    dsp_write(HIBYTE(BUFFER_SIZE - 1));
    dsp_write(DSP_CMD_8BIT_AUTO_OUTPUT);
    dsp_write(DSP_MODE_UNSIGNED);
    
    first_run = 0;
    
    printf("[TEST] Test tone should be playing!\n");
}

void test_stereo() {
    int i;
    int sample_rate = SAMPLE_RATE;
    int frames = BUFFER_SIZE / 2;
    float angle;
    int s;
    unsigned char tc;
    int quarter = frames / 4;

    // Reset and initialization of DSP
    dsp_reset();
    dsp_version = get_dsp_version();
    dsp_write(DSP_CMD_SPEAKER_ON);
    delay(10);

    // Stereo on mixer output
    outp(sb_port + 0x04, 0x0E);
    outp(sb_port + 0x05, 0x03);
    delay(10);

    // Frequency
    tc = (unsigned char)(256.0f - (1000000.0f / (float)(sample_rate * 2)));
    dsp_write(DSP_CMD_SAMPLE_RATE);
    dsp_write(tc);
    delay(10);

    // Size of the block
    dsp_write(DSP_CMD_8BIT_DMA_SETUP);
    dsp_write(LOBYTE(BUFFER_SIZE - 1));
    dsp_write(HIBYTE(BUFFER_SIZE - 1));
    delay(10);

    // Filling the buffer with four different sections
    for (i = 0; i < frames; i++) {
        int section = i / quarter;   // 0, 1, 2, 3
        int L = 0x80, R = 0x80;

        switch (section) {
            case 0:   // L=440, R=silence
                angle = 2.0f * 3.14159f * 440 * i / sample_rate;
                s = (int)(128 + 100 * sin(angle));
                L = s;
                R = 0x80;
                break;
            case 1:   // L=silence, R=440
                angle = 2.0f * 3.14159f * 440 * i / sample_rate;
                s = (int)(128 + 100 * sin(angle));
                L = 0x80;
                R = s;
                break;
            case 2:   // L=440, R=880
                angle = 2.0f * 3.14159f * 440 * i / sample_rate;
                s = (int)(128 + 100 * sin(angle));
                L = s;
                angle = 2.0f * 3.14159f * 880 * i / sample_rate;
                s = (int)(128 + 100 * sin(angle));
                R = s;
                break;
            case 3:   // L=880, R=440
                angle = 2.0f * 3.14159f * 880 * i / sample_rate;
                s = (int)(128 + 100 * sin(angle));
                L = s;
                angle = 2.0f * 3.14159f * 440 * i / sample_rate;
                s = (int)(128 + 100 * sin(angle));
                R = s;
                break;
        }

        if (L > 255) L = 255;
        if (L < 0)   L = 0;
        if (R > 255) R = 255;
        if (R < 0)   R = 0;

        dma_buffers[0][i * 2]     = (unsigned char)L;
        dma_buffers[0][i * 2 + 1] = (unsigned char)R;
    }

    // --- Запускаем ---
    setup_dma_controller_only(dma_buffers[0]);
    dsp_write(DSP_CMD_8BIT_SINGLE_OUTPUT);   // 0xC0
    dsp_write(DSP_MODE_STEREO);              // 0x20

    printf("[TEST] Section 0: L=440, R=silence\n");
    printf("[TEST] Section 1: L=silence, R=440\n");
    printf("[TEST] Section 2: L=440, R=880\n");
    printf("[TEST] Section 3: L=880, R=440\n");

    // --- Циклически перезапускаем, чтобы блок играл непрерывно ---
    for (i = 0; i < 6; i++) {
        delay(500);
        setup_dma_controller_only(dma_buffers[0]);
        dsp_write(DSP_CMD_8BIT_SINGLE_OUTPUT);
        dsp_write(DSP_MODE_STEREO);
    }

    dsp_write(DSP_CMD_EXIT_AUTO_INIT);
    dsp_write(DSP_CMD_PAUSE_DMA);
    is_playing = 0;
}

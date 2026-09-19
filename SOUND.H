#ifndef SOUND_H
#define SOUND_H

#define MAX_STREAMS 8
#define BUFFER_SIZE 4096
#define NUM_BUFFERS 2
#define SAMPLE_RATE 22050

#define LOBYTE(w) ((unsigned char)((unsigned short)(w) & 0xFF))
#define HIBYTE(w) ((unsigned char)(((unsigned short)(w) >> 8)) & 0xFF)

#define DSP_RESET_PORT_OFFSET       0x06
#define DSP_READ_PORT_OFFSET        0x0A
#define DSP_WRITE_PORT_OFFSET       0x0C
#define DSP_DATA_AVAILABLE_OFFSET   0x0E

#define DSP_RESET      (sb_port + DSP_RESET_PORT_OFFSET)
#define DSP_READ       (sb_port + DSP_READ_PORT_OFFSET)
#define DSP_WRITE      (sb_port + DSP_WRITE_PORT_OFFSET)
#define DSP_STATUS     (sb_port + DSP_WRITE_PORT_OFFSET)
#define DSP_DATA_AVAIL (sb_port + DSP_DATA_AVAILABLE_OFFSET)

#define DMA_PAGE_REGISTER           0x83   // for channel 1
#define DMA_MASK_REGISTER           0x0A
#define DMA_MODE_REGISTER           0x0B
#define DMA_CLEAR_BYTE_POINTER      0x0C

#define DMA_MODE_SINGLE_CYCLE       0x48
#define DMA_MODE_AUTO_INIT          0x58

#define DSP_CMD_8BIT_SINGLE_OUTPUT           0xC0   // 8-bit single-cycle DAC output (mono)
#define DSP_CMD_8BIT_SINGLE_OUTPUT_HI        0xC1   // 8-bit single-cycle, high-speed (SB Pro)
#define DSP_CMD_8BIT_AUTO_OUTPUT             0xC6   // 8-bit auto-init DAC output (SB Pro)
#define DSP_CMD_8BIT_AUTO_OUTPUT_HI          0xC7   // 8-bit auto-init, high-speed

#define DSP_CMD_8BIT_DMA_SETUP               0x14   // Without this command, nothing worked for me
#define DSP_CMD_8BIT_AUTO_INIT_DAC           0x1C   // 8-bit auto-init DAC output (SB16, main)
#define DSP_CMD_8BIT_HIGH_SPEED_SINGLE       0x1E   // 8-bit high-speed single-cycle (rarely)

#define DSP_CMD_16BIT_SINGLE_OUTPUT          0xB0   // 16-bit single-cycle DAC output
#define DSP_CMD_16BIT_AUTO_OUTPUT            0xB4   // 16-bit single-cycle, FIFO
#define DSP_CMD_16BIT_AUTO_INIT_DAC          0xB6   // 16-bit auto-init DAC output
#define DSP_CMD_16BIT_HIGH_SPEED_SINGLE      0xB8   // 16-bit high-speed single-cycle

#define DSP_CMD_PAUSE_DMA                    0xD0   // Pause 8/16-bit DMA transfer
#define DSP_CMD_CONTINUE_DMA                 0xD4   // Continue DMA transfer after pause
#define DSP_CMD_EXIT_AUTO_INIT               0xDA   // Exit 8-bit auto-init mode
#define DSP_CMD_EXIT_16BIT_AUTO_INIT         0xD9   // Exit 16-bit auto-init mode

#define DSP_CMD_SPEAKER_ON                   0xD1   // Turn speaker ON
#define DSP_CMD_SPEAKER_OFF                  0xD3   // Turn speaker OFF

#define DSP_CMD_SAMPLE_RATE				     0x40   // SBPRO2 Sample Rate
#define DSP_CMD_SET_PLAYBACK_RATE            0x41   // SB16 Set playback sample rate (2 bytes after command)
#define DSP_CMD_SET_CAPTURE_RATE             0x42   // SB16 Set recording sample rate

#define DSP_CMD_GET_VERSION                  0xE1   // Get DSP version (returns major.minor)
#define DSP_CMD_RESET                        0xFF   // Reset via port (not a DSP command, separate port)

#define DSP_MODE_MONO                        0x00
#define DSP_MODE_UNSIGNED                    0x00
#define DSP_MODE_SIGNED                      0x10    // for 16-bit
#define DSP_MODE_STEREO                      0x20

typedef struct {
    unsigned char *data;        // Audio data
    unsigned int position;      // Current position
    unsigned int size;          // Size in bytes
    unsigned int sample_rate;   // Frequency
    int channels;               // 1 = mono, 2 = stereo
    int volume;                 // Volume (0-255)
    int volume_l;               // Left channel volume
    int volume_r;               // Right channel volume
    int playing;                // Active?
    int looping;                // Looped?
    int priority;               // Priority (0-high)
} SoundStream;

typedef struct {
    unsigned char *data;
    unsigned int sample_rate;   // Frequency
    int channels;               // 1 = mono, 2 = stereo
    unsigned int size;          // Size in bytes
} Sound;

extern SoundStream streams[MAX_STREAMS];
extern unsigned char mix_buffer[BUFFER_SIZE];

extern volatile int dma_block_finished_flag;
extern int playing_final_chunk;

extern volatile int counter;
extern volatile unsigned char irq_debug_last_status;
extern volatile unsigned char irq_debug_read_data;

extern unsigned char *dma_buffers[NUM_BUFFERS];
extern unsigned short dma_selectors[NUM_BUFFERS];
extern unsigned short dma_segments[NUM_BUFFERS];
extern unsigned int current_buffer;
extern unsigned int next_buffer;
extern unsigned int first_run;

// Function prototypes
void mixer_init();
void mixer_stop(int channel);
void mixer_free_channel(int channel);
void mixer_set_volume(int channel, int volume);
void mixer_mix(unsigned char *output, unsigned int samples);
void mixer_process();
int setup_sound_blaster();
void __interrupt __far irq_handler();
void setup_irq_handler();
void cleanup_irq_handler();
unsigned char *alloc_low_dos_memory(unsigned long bytes, unsigned short *selector, unsigned short *segment);
void free_low_dos_memory(unsigned short selector);
void dsp_reset();
void dsp_write(unsigned char cmd);
unsigned char dsp_read();
void set_sb_sample_rate(unsigned int hz);
unsigned int get_dsp_version();
void setup_dma_for_buffer(unsigned char *buffer, int buffer_index);
void setup_dma_controller_only(unsigned char *buffer);
int fill_buffer(unsigned char *buffer, int buffer_index);
void swap_buffers();
void cleanup_buffers();
void sb_stop_playback();
void show_progress();
int load_wav(
    const char *filename,
    Sound *sound);
void test_sound_generator();
void test_stereo();
int mixer_play_sound_ex(int channel, Sound *sound, int looping, int priority);
void mixer_set_pan(int channel, int pan);       // 0=left, 128=center, 255=right
void mixer_set_volume_stereo(int channel, int vol_l, int vol_r);
void dsp_set_stereo(int enable);

#endif

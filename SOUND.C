/******************************************************************************
*                                 SOUND.C                                     *
*                  Воспроизведение WAV-файла (22 KHz, 8 бит, моно, PCM)       *
*                             через Sound Blaster Pro 2                       *
*                  с использованием DMA auto-init и прерываний                *
******************************************************************************/

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

unsigned int dsp_version = 0;  // 0 = не определена

unsigned char *current_dma_target_ptr = NULL;

unsigned long phys;
unsigned int page, offset;

//const unsigned int dma_buffer_size = BUFFER_SIZE;

// Два буфера для двойной буферизации

// unsigned char *dma_buffer = NULL;
// unsigned short dma_selector = 0;
// unsigned short dma_segment = 0;
unsigned char *dma_buffers[NUM_BUFFERS] = {NULL, NULL};
unsigned short dma_selectors[NUM_BUFFERS] = {0, 0};
unsigned short dma_segments[NUM_BUFFERS] = {0, 0};

unsigned int bytes_to_play; // Количество байт для проигрывания

unsigned int current_buffer = 0;        // Буфер, который сейчас играет
unsigned int next_buffer = 1;           // Буфер, который заполняем
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
// Микширование разных звуков
// **************************************************************

// Инициализация
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
    
    // Стерео на выходе микшера
    outp(sb_port + 0x04, 0x0E);
    outp(sb_port + 0x05, 0x03);
    delay(10);


    
    // Запускаем воспроизведение
    // dsp_write(DSP_CMD_8BIT_DMA_SETUP);
    // dsp_write(LOBYTE(BUFFER_SIZE - 1));
    // dsp_write(HIBYTE(BUFFER_SIZE - 1));

    //dsp_write(DSP_CMD_8BIT_SINGLE_OUTPUT);
    
    //dsp_write(DSP_MODE_UNSIGNED);
    //dsp_write(DSP_MODE_MONO);

    //set_sb_sample_rate(22050);
    set_sb_sample_rate(SAMPLE_RATE);

    // Настраиваем DMA
    setup_dma_for_buffer(dma_buffers[0], 0);
    //setup_dma_controller_only(dma_buffers[0]);

    // Это идёт последовательно
    //dsp_write(DSP_CMD_8BIT_AUTO_OUTPUT);
    dsp_write(DSP_CMD_8BIT_SINGLE_OUTPUT);
    dsp_write(DSP_MODE_STEREO);

    // dsp_write(0xC0);
    // dsp_write(0x20);

    dsp_write(DSP_CMD_SPEAKER_ON);  // Включить колонки!
}

// Явная остановка канала
void mixer_stop(int channel) {
    if (channel < 0 || channel >= MAX_STREAMS) return;
    streams[channel].playing = 0;
    streams[channel].position = 0;
    streams[channel].data = NULL;
    streams[channel].size = 0;
    streams[channel].volume = 255;
}

// Полная очистка канала (для освобождения ресурсов)
void mixer_free_channel(int channel) {
    mixer_stop(channel);
}

// Поиск свободного или наименее приоритетного канала
int find_free_channel(int priority) {
    int i;
    // Если нет свободных, ищем с наименьшим приоритетом
    int lowest_priority = 999;
    int lowest_channel = -1;

    // Сначала ищем свободный
    for (i = 0; i < MAX_STREAMS; i++) {
        if (!streams[i].playing) return i;
    }
    
    for (i = 0; i < MAX_STREAMS; i++) {
        if (streams[i].priority < lowest_priority) {
            lowest_priority = streams[i].priority;
            lowest_channel = i;
        }
    }
    
    // Если новый звук важнее, заменяем
    if (lowest_channel != -1 && priority < streams[lowest_channel].priority) {
        streams[lowest_channel].playing = 0;
        return lowest_channel;
    }
    
    return -1; // Нет места
}

// Запуск звука
// int mixer_play_sound(int channel, const unsigned char *data, 
//                      unsigned int size, unsigned int sample_rate, 
//                      int looping, int priority) {
//     if (channel == -1) {
//         // Автоматический поиск канала
//         channel = find_free_channel(priority);
//         if (channel == -1) return -1;
//     }
//     if (channel >= MAX_STREAMS) return -1;
//     // Останавливаем текущий звук
//     streams[channel].playing = 0;
//     // Загружаем новый
//     streams[channel].data = (unsigned char*)data;
//     streams[channel].position = 0;
//     streams[channel].size = size;
//     streams[channel].sample_rate = sample_rate;
//     streams[channel].volume = 255;
//     streams[channel].playing = 1;
//     streams[channel].looping = looping;
//     streams[channel].priority = priority;
//     return channel;
// }

// Старая функция — моно по умолчанию:
int mixer_play_sound(int channel, const unsigned char *data,
                     unsigned int size, unsigned int sample_rate,
                     int looping, int priority)
{
    return mixer_play_sound_ex(channel, data, size, sample_rate,
                               1, looping, priority);
}

int mixer_play_sound_ex(int channel, const unsigned char *data,
                        unsigned int size, unsigned int sample_rate,
                        int channels, int looping, int priority)
{
    if (channel == -1) {
        channel = find_free_channel(priority);
        if (channel == -1) return -1;
    }
    if (channel >= MAX_STREAMS) return -1;

    streams[channel].playing     = 0;
    streams[channel].data        = (unsigned char*)data;
    streams[channel].position    = 0;
    streams[channel].size        = size;
    streams[channel].sample_rate = sample_rate;
    streams[channel].channels    = channels;
    streams[channel].volume      = 255;
    streams[channel].volume_l    = 255;
    streams[channel].volume_r    = 255;
    streams[channel].playing     = 1;
    streams[channel].looping     = looping;
    streams[channel].priority    = priority;
    return channel;
}

void mixer_set_pan(int channel, int pan)
{
    if (channel < 0 || channel >= MAX_STREAMS) return;
    if (pan < 0)   pan = 0;
    if (pan > 255) pan = 255;
    streams[channel].volume_l = 255 - pan;
    streams[channel].volume_r = pan;
}

void mixer_set_volume_stereo(int channel, int vol_l, int vol_r)
{
    if (channel < 0 || channel >= MAX_STREAMS) return;
    streams[channel].volume_l = vol_l & 0xFF;
    streams[channel].volume_r = vol_r & 0xFF;
}

// Микширование всех каналов
void mixer_mix(unsigned char *output, unsigned int samples) {
    // samples = количество БАЙТ в выходном буфере.
    // Выход всегда стерео (2 байта на кадр), как настроен DSP.

    int i, j;
    int active = 0;
    int sample, mixed;
    SoundStream *ch;
    int out_frames;

    out_frames = samples / 2;   // кадров в выходном буфере

    // Очищаем выходной буфер (тишина)
    memset(output, 0x80, samples);
    
    // Микшируем каждый активный канал
    for (i = 0; i < MAX_STREAMS; i++) {
        if (!streams[i].playing) continue;
        
        active++;
        ch = &streams[i];
        
        //for (j = 0; j < samples; j++) {
        for (j = 0; j < out_frames; j++) {
            int L, R, m, out_l, out_r;


            // Проверяем конец звука
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
            
            // Теперь стерео
            // мы работаем в «знаковой» (centered) арифметике: 
            // вычитаем 128 из сэмпла, суммируем, потом прибавляем 128 обратно;
            // клиппинг делаем до прибавления 128, в диапазоне [-128, 127];
            // выходной буфер считаем всегда стерео — так проще и совпадает с настройкой DSP.
            if (ch->channels == 2) {
                // Стерео: два байта на кадр
                L = (int)ch->data[ch->position    ] - 128;
                R = (int)ch->data[ch->position + 1] - 128;
                ch->position += 2;
            } else {
                // Моно: дублируем в оба канала
                m = (int)ch->data[ch->position] - 128;
                ch->position += 1;
                L = m;
                R = m;
            }

            // Применяем громкость каналов
            L = (L * ch->volume_l) / 256;
            R = (R * ch->volume_r) / 256;

            // Микшируем с клиппингом
            out_l = (int)output[j*2    ] - 128 + L;
            out_r = (int)output[j*2 + 1] - 128 + R;
            if (out_l >  127) out_l =  127;
            if (out_l < -128) out_l = -128;
            if (out_r >  127) out_r =  127;
            if (out_r < -128) out_r = -128;
            output[j*2    ] = (unsigned char)(out_l + 128);
            output[j*2 + 1] = (unsigned char)(out_r + 128);

            // // Получаем сэмпл и применяем громкость
            // sample = ch->data[ch->position++];
            // sample = ((sample - 128) * ch->volume) / 256 + 128;
            
            // // Микшируем с клиппингом
            // mixed = output[j] + sample - 128;
            // if (mixed > 255) mixed = 255;
            // if (mixed < 0) mixed = 0;
            // output[j] = (unsigned char)mixed;
        }
    }
    
    // Если нет активных каналов, останавливаем воспроизведение
    if (active == 0) {
        is_playing = 0;
    }
}

// Основная функция обработки звука - вызывается каждый кадр
void mixer_process() {

    int next;

    // if (free_buffer >= 0) {
    //     mixer_mix(dma_buffers[free_buffer], BUFFER_SIZE);
    //     free_buffer = -1;
    // }

    if (dma_block_finished_flag) {
        dma_block_finished_flag = 0;
        
        next = (current_buffer + 1) % NUM_BUFFERS;
        mixer_mix(dma_buffers[next], BUFFER_SIZE);

        //printf("[DEBUG] IRQ Fired: %d\n", counter);

        // if (is_playing) {
        //     // Микшируем в следующий буфер
        //     int next = (current_buffer + 1) % NUM_BUFFERS;
        //     mixer_mix(dma_buffers[next], BUFFER_SIZE);
        //     // Настраиваем DMA на следующий буфер
        //     //setup_dma_for_buffer(dma_buffers[next], next);
        //     setup_dma_for_buffer(dma_buffers[next], next);
        //     //setup_dma_controller_only(dma_buffers[next]);
        //     // Переключаем буфер
        //     current_buffer = next;
        // }
    }
            
    //         // dsp_write(0xC0);
    //         // dsp_write(0x20);

    //         // Запускаем только при первом вызове
    //         // if (first_run) {
    //         //     // setup_dma_for_buffer уже всё сделал!
    //         //     // Просто говорим DSP начать воспроизведение
    //         //     //dsp_write(DSP_CMD_8BIT_AUTO_OUTPUT);
    //         //     //dsp_write(DSP_MODE_UNSIGNED);
    //         //     first_run = 0;
    //         // }
    //         // // 1. Микшируем все звуки в буфер
    //         // //mixer_mix(mix_buffer, BUFFER_SIZE);
    //         // mixer_mix(dma_buffers[current_buffer], BUFFER_SIZE);
    //         // // 2. Настраиваем DMA на микшированный буфер
    //         // //setup_dma_for_buffer(mix_buffer, 0);
    //         // setup_dma_for_buffer(dma_buffers[current_buffer], current_buffer);
    //         // // Переключаем буфер
    //         // current_buffer = (current_buffer + 1) % NUM_BUFFERS;
    //         // // 3. Запускаем воспроизведение (если ещё не запущено)
    //         // if (first_run) {
    //         //     dsp_write(DSP_CMD_8BIT_DMA_SETUP);
    //         //     dsp_write(LOBYTE(BUFFER_SIZE - 1));
    //         //     dsp_write(HIBYTE(BUFFER_SIZE - 1));
    //         //     dsp_write(DSP_CMD_8BIT_AUTO_OUTPUT);
    //         //     dsp_write(DSP_MODE_UNSIGNED);
    //         //     first_run = 0;
    //         // }
    //     }
    // }
}

// **************************************************************
// Начальная настройка
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

    /* здесь можно проверить валидность, например */
    if (sb_port < 0x210 || sb_port > 0x280 || (sb_port & 0x0F) != 0) {
        printf("[ERROR] Wrong port or address!\n");
        return -1;
    }

    return 1;   /* успех */
}

// **************************************************************
// Работа с прерываниями
// **************************************************************
void __interrupt __far irq_handler() {

    int next;
    
    counter++;

    // 1. Пауза DMA (без этой и CONTINUE_DMA не работало в 86box)
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

    // 5. Возобновляем DMA — но DSP будет читать с НОВОГО адреса
    // Без этого не работало в 86box
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
// Низкоуровневые функции для управления куском памяти
// ниже 640 кб для работы DMA буфера
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
    regs.w.ax = 0x0101; // Устанавливаем номер DPMI-функции: 0x0101 — Free DOS Memory Block.
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
// Низкоуровневые функции для управления DSP Sound Blaster
// **************************************************************

// *** dsp_reset() ***
// Пытается сбросить DSP чип Sound Blaster и проверить, что он ответил 0xAA
// Возвращает: ничего, но выводит диагностику в консоль
void dsp_reset()
{
    int timeout;
    unsigned char resp;

    puts("[DSP] Reset start...\n");

    // Пишем единицу в регистр на 3 мс
    outp(DSP_RESET, 1);
    delay(3);

    // Пишем ноль в регистр на 100 мс
    outp(DSP_RESET, 0); delay(100);

    // Ждём ответ
    timeout = 1000;
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
        puts("[DSP] Version not detected, assuming SB16\n");
        dsp_version = 0x0400;  // fallback на SB16
    }

    printf("[DSP] Setting sample rate %u Hz (DSP v%d.%02d)\n",
           hz, (dsp_version >> 8), (dsp_version & 0xFF));

    if (dsp_version >= 0x0400) {  // SB16 и выше (DSP 4.xx)
        dsp_write(DSP_CMD_SET_PLAYBACK_RATE);
        dsp_write((unsigned char)(hz >> 8));   // high byte
        dsp_write((unsigned char)(hz & 0xFF)); // low byte
        printf("[SB16] Direct rate: %u Hz\n", hz);
    } else {  // SB Pro 2 и ниже (DSP 3.xx и раньше)
        // Time Constant = 256 - (1 000 000 / sample rate)
        unsigned int byte_rate = hz * 2;   // 2 байта на кадр
        unsigned char tc = (unsigned char)(256.0f - (1000000.0f / (float)byte_rate));
        dsp_write(DSP_CMD_SAMPLE_RATE);
        dsp_write(tc);
		//dsp_write(0xC1);
        printf("[SB Pro] Time Constant: 0x%02X (effective rate ~%u Hz)\n", tc, hz);

        // hz — частота кадров (например, 22050)
        // для стерео TC считается от удвоенной байтовой скорости
        
        
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
    // Порядок важен: сначала команда, потом её параметр
    dsp_write(enable ? DSP_MODE_STEREO : DSP_MODE_MONO);
    printf("[DSP] Stereo mode: %s\n", enable ? "ON" : "OFF");
}

// ************************************************
// Работа с буферами
// ************************************************

// Настраивает DMA для указанного буфера
// void setup_dma_for_buffer(unsigned char *buffer, int buffer_index)
// {
//     phys = (unsigned long)buffer;
//     page = phys >> 16;
//     offset = phys & 0xFFFF;
//     dsp_write(DSP_CMD_8BIT_DMA_SETUP);
//     dsp_write(LOBYTE(dma_buffer_size - 1));
//     dsp_write(HIBYTE(dma_buffer_size - 1));
//     _disable();
//     // Отключаем DMA
//     outp(DMA_MASK_REGISTER, 0x04 | sb_dma);
//     // Сбрасываем flip-flop
//     outp(DMA_CLEAR_BYTE_POINTER, 0x00);
//     // Устанавливаем режим auto-init
//     outp(DMA_MODE_REGISTER, DMA_MODE_AUTO_INIT | sb_dma);
//     // Устанавливаем адрес
//     outp((sb_dma << 1) + 0, LOBYTE(offset));
//     outp((sb_dma << 1) + 0, HIBYTE(offset));
//     // Устанавливаем страницу
//     outp(DMA_PAGE_REGISTER, page);
//     // Устанавливаем счетчик (на 1 меньше реального размера)
//     outp((sb_dma << 1) + 1, LOBYTE(dma_buffer_size - 1));
//     outp((sb_dma << 1) + 1, HIBYTE(dma_buffer_size - 1));
//     // Включаем DMA
//     outp(DMA_MASK_REGISTER, sb_dma);
//     _enable();
//     // // // Без этих строк почему-то не вызывается прерывание
//     // dsp_write(DSP_CMD_8BIT_DMA_SETUP);
//     // dsp_write(LOBYTE(dma_buffer_size - 1));
//     // dsp_write(HIBYTE(dma_buffer_size - 1));
// }

// Только программирование DMA-контроллера. Без DSP-команд.
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

// Полная настройка: DMA + DSP block size. Используется один раз.
void setup_dma_for_buffer(unsigned char *buffer, int buffer_index)
{
    setup_dma_controller_only(buffer);
    dsp_write(DSP_CMD_8BIT_DMA_SETUP);
    // dsp_write(LOBYTE(dma_buffer_size - 1));
    // dsp_write(HIBYTE(dma_buffer_size - 1));
    dsp_write(LOBYTE(BUFFER_SIZE - 1));
    dsp_write(HIBYTE(BUFFER_SIZE - 1));
}

// Заполняет указанный буфер данными из WAV
// Возвращает 1, если достигнут конец файла
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
        // Оставшуюся часть буфера заполняем тишиной (0x80 для 8-битного PCM)
        memset(buffer + part1, 0x80, to_copy - part1);
        current_playback_position = wav_data_size;
        is_final = 1;
        playing_final_chunk = 1;
    }

    return is_final;
}

// Переключает буферы при прерывании
void swap_buffers()
{
    // Заполняем следующий буфер
    int final = fill_buffer(dma_buffers[next_buffer], next_buffer);
    
    // Настраиваем DMA на следующий буфер
    setup_dma_for_buffer(dma_buffers[next_buffer], next_buffer);
    
    // Переключаем буферы
    current_buffer = next_buffer;
    next_buffer = (next_buffer + 1) % NUM_BUFFERS;
    
    // Показываем прогресс
    //show_progress();
    
    if (final) {
        is_playing = 0;
    }
}

// Выводит прогресс-бар
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

// Освобождает все буферы
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

// Копирует звуковые данные в кусок памяти DMA
// Сдвигает play_pos на величину буфера
// void sb_copy_buffer()
// {
// 	   unsigned long to_copy;
// 	   unsigned long part1;
//     unsigned int percent;
//     unsigned int filled;
//     unsigned int i;
//     unsigned int progress_len = 50;
//     if (!dma_buffer) {
//         printf("[ERROR] dma_buffer is not defined!\n");
//         return;
//     }
//     current_dma_target_ptr = dma_buffer;
//     to_copy = dma_buffer_size;
//     if (current_playback_position + to_copy <= wav_data_size) 
//     { // Начало файла
//         memcpy(current_dma_target_ptr, wav_data + current_playback_position, to_copy);
//         current_playback_position += to_copy;
//         //if (play_pos >= datasize) play_pos = 0;
//     }
//     else { // Подошли к концу файла
//         part1 = wav_data_size - current_playback_position;
//         memcpy(current_dma_target_ptr, wav_data + current_playback_position, part1);
//         //printf("Final part, %d bytes\n", part1);
//         playing_final_chunk = 1;
//         current_playback_position = wav_data_size;
//     }
//     //printf("\rplay_pos = %16lu / %16lu", play_pos, datasize);
//     percent = (unsigned int)((current_playback_position * 100UL) / wav_data_size);
//     filled  = (unsigned int)((current_playback_position * (unsigned long)progress_len) / wav_data_size);
//     printf("\r[");
//     for (i = 0; i < progress_len; i++) {
//         if (i < filled)
//             putchar(219); // ■ — полный блок (код 219)
//         else
//             putchar(177); // ░ — лёгкая штриховка (код 177)
//     }
//     printf("] %3u%%  %lu / %lu   ", percent, current_playback_position, wav_data_size);
//     fflush(stdout);
//     // Без этих строк почему-то не вызывается прерывание
//     dsp_write(DSP_CMD_8BIT_DMA_SETUP);
// 	dsp_write(LOBYTE(bytes_to_play));
//     dsp_write(HIBYTE(bytes_to_play));
// }

void sb_stop_playback(void)
{
    dsp_write(DSP_CMD_EXIT_AUTO_INIT);                     /* exit auto-init */
    dsp_write(DSP_CMD_PAUSE_DMA);                     /* pause */
    is_playing = 0;
}

// int sb_cleanup()
// {
//     sb_stop_playback();
//     free_low_dos_memory(dma_selector);
//     dma_buffer = NULL;
//     dma_selector = 0;
// 	free(wav_data);
//     return 0;
// }

// 
unsigned char* load_wav(
    const char *filename,
    unsigned int *size,
    unsigned int *rate,
    unsigned int *channels_out)
{
    FILE *soundFile;
    unsigned char header[44];
    unsigned short format, num_channels, bitspersample;
    unsigned int samplerate;
	unsigned int len;
    unsigned int i;
    unsigned char *wav_data;
    unsigned int wav_data_size;

    soundFile = fopen(filename, "rb");
	
	if (fread(header, 1, 44, soundFile) != 44 ||
		strncmp((char*)header,      "RIFF", 4) != 0 ||
		strncmp((char*)(header+8),  "WAVE", 4) != 0 ||
		strncmp((char*)(header+12), "fmt ", 4) != 0)
	{
		printf("[ERROR] This is not a WAV file: %s\n", filename);
		fclose(soundFile);
		return NULL;
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
		return NULL;
	}

	wav_data = malloc(wav_data_size);
	fread(wav_data, 1, wav_data_size, soundFile);
	fclose(soundFile);

	//printf("[MEM] Allocated %d bytes for %s\n", wav_data_size, filename);
    printf("[MEM] Allocated %d bytes for %s (%d ch, %d Hz)\n",
           wav_data_size, filename, num_channels, samplerate);

    *size = wav_data_size;
    *rate = samplerate;
    *channels_out = num_channels;

    return wav_data;
}

// Короткий тестовый сигнал отправляется в DSP
void test_sound_generator() {
    int i;
    
    // Заполняем буфер тестовым тоном (440 Гц)
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
    
    // Настраиваем DMA
    setup_dma_for_buffer(dma_buffers[0], 0);
    
    // Запускаем воспроизведение
    dsp_write(DSP_CMD_8BIT_DMA_SETUP);
    dsp_write(LOBYTE(BUFFER_SIZE - 1));
    dsp_write(HIBYTE(BUFFER_SIZE - 1));
    dsp_write(DSP_CMD_8BIT_AUTO_OUTPUT);
    dsp_write(DSP_MODE_UNSIGNED);
    
    is_playing = 1;
    first_run = 0;
    
    printf("[TEST] Test tone should be playing!\n");
}

// void test_stereo() {
//     int i;
//     int freq = 440;
//     int sample_rate = SAMPLE_RATE;
//     int frames = BUFFER_SIZE / 2;   // кадров (L+R пар)
//     float angle;
//     int s;
//     printf("[TEST] Generating stereo test tone (L=440Hz, R=silence)...\n");
//     // Сначала левый канал
//     for (i = 0; i < frames; i++) {
//         angle = 2.0f * 3.14159f * freq * i / sample_rate;
//         s = (int)(128 + 100 * sin(angle));
//         if (s > 255) s = 255;
//         if (s < 0) s = 0;
//         dma_buffers[0][i * 2]     = (unsigned char)s;   // L — тон
//         dma_buffers[0][i * 2 + 1] = 0x80;               // R — тишина
//     }
//     setup_dma_for_buffer(dma_buffers[0], 0);
//     // dsp_write(DSP_CMD_8BIT_DMA_SETUP);
//     // dsp_write(LOBYTE(BUFFER_SIZE - 1));
//     // dsp_write(HIBYTE(BUFFER_SIZE - 1));
//     //dsp_write(DSP_CMD_8BIT_AUTO_OUTPUT);
//     dsp_write(DSP_CMD_8BIT_SINGLE_OUTPUT);
//     dsp_write(DSP_MODE_STEREO);   // 0x20
//     delay(1000);
//     // Теперь правый
//     printf("[TEST] Generating stereo test tone (L=silence, R=440Hz)...\n");
//     for (i = 0; i < frames; i++) {
//         angle = 2.0f * 3.14159f * freq * i / sample_rate;
//         s = (int)(128 + 100 * sin(angle));
//         if (s > 255) s = 255;
//         if (s < 0) s = 0;
//         dma_buffers[0][i * 2]     = 0x80; // L — тишина
//         dma_buffers[0][i * 2 + 1] = (unsigned char)s; // R — тон
//     }
//     setup_dma_for_buffer(dma_buffers[0], 0);
//     // dsp_write(DSP_CMD_8BIT_DMA_SETUP);
//     // dsp_write(LOBYTE(BUFFER_SIZE - 1));
//     // dsp_write(HIBYTE(BUFFER_SIZE - 1));
//     //dsp_write(DSP_CMD_8BIT_AUTO_OUTPUT);
//     dsp_write(DSP_CMD_8BIT_SINGLE_OUTPUT);
//     dsp_write(DSP_MODE_STEREO);   // 0x20
//     is_playing = 1;
//     first_run = 0;
//     printf("[TEST] If tone ONLY in left ear — stereo works.\n");
//     printf("[TEST] If in both ears — DSP is in mono.\n");
//     delay(1000);
// }

// void test_stereo() {
//     int i;
//     int freq = 440;
//     int sample_rate = SAMPLE_RATE;       // 11025
//     int frames = BUFFER_SIZE / 2;        // 4096 кадров (L+R)
//     float angle;
//     int s;
//     unsigned int tc;
//     int loop;
//     printf("[TEST] Stereo test start. SAMPLE_RATE=%u\n", sample_rate);
//     // ------------------------------------------------------------------
//     // 1. Полный сброс DSP
//     // ------------------------------------------------------------------
//     dsp_reset();
//     dsp_version = get_dsp_version();
//     // Включить стерео на выходе микшера SB Pro 2
//     outp(sb_port + 0x04, 0x0E);   // выбрать регистр 0x0E
//     outp(sb_port + 0x05, 0x03);   // биты 0-1: 11 = stereo
//     // ------------------------------------------------------------------
//     // 2. Speaker ON — ДО всего остального
//     // ------------------------------------------------------------------
//     dsp_write(DSP_CMD_SPEAKER_ON);
//     delay(10);
//     // ------------------------------------------------------------------
//     // 3. Частота через Time Constant (для 8-бит всегда 0x40)
//     //    Для стерео 11025 кадров/с: byte_rate = 22050, TC = 0xD2
//     // ------------------------------------------------------------------
//     tc = (unsigned int)(256.0f - (1000000.0f / (float)(sample_rate * 2)));
//     dsp_write(DSP_CMD_SAMPLE_RATE);   // 0x40
//     dsp_write((unsigned char)tc);
//     printf("[TEST] TC = 0x%02X (для %u кадров/с стерео)\n", tc, sample_rate);
//     delay(10);
//     // ------------------------------------------------------------------
//     // 4. Устанавливаем размер DMA-блока для DSP (0x14) — ОДИН РАЗ
//     // ------------------------------------------------------------------
//     dsp_write(DSP_CMD_8BIT_DMA_SETUP);   // 0x14
//     dsp_write(LOBYTE(BUFFER_SIZE - 1));
//     dsp_write(HIBYTE(BUFFER_SIZE - 1));
//     delay(10);
//     // ------------------------------------------------------------------
//     // 5. Заполняем оба буфера: сначала L=тон, R=тишина,
//     //    потом L=тишина, R=тон — чтобы услышать оба канала.
//     //    Но для теста сделаем проще: в одном буфере L=тон, R=тон-наоборот.
//     // ------------------------------------------------------------------
//     printf("[TEST] Заполняем буфер: L=440Hz, R=тишина\n");
//     for (i = 0; i < frames; i++) {
//         angle = 2.0f * 3.14159f * freq * i / sample_rate;
//         s = (int)(128 + 100 * sin(angle));
//         if (s > 255) s = 255;
//         if (s < 0) s = 0;
//         dma_buffers[0][i * 2]     = (unsigned char)s;   // L — тон
//         dma_buffers[0][i * 2 + 1] = 0x80;               // R — тишина
//     }
//     // ------------------------------------------------------------------
//     // 6. Программируем DMA-контроллер на буфер 0 (БЕЗ DSP-команд!)
//     // ------------------------------------------------------------------
//     setup_dma_controller_only(dma_buffers[0]);
//     // ------------------------------------------------------------------
//     // 7. Запускаем single-cycle stereo
//     // ------------------------------------------------------------------
//     dsp_write(DSP_CMD_8BIT_SINGLE_OUTPUT);   // 0xC0
//     dsp_write(DSP_MODE_STEREO);              // 0x20
//     printf("[TEST] Слушаем 1.5 сек... (L=тон, R=тишина)\n");
//     for (loop = 0; loop < 3; loop++) {
//         delay(500);
//         // В single-cycle режиме IRQ приходит после каждого блока.
//         // Перезапускаем воспроизведение, чтобы не было пауз.
//         setup_dma_controller_only(dma_buffers[0]);
//         dsp_write(DSP_CMD_8BIT_SINGLE_OUTPUT);
//         dsp_write(DSP_MODE_STEREO);
//     }
//     delay(1000);
//     // ------------------------------------------------------------------
//     // 8. Теперь L=тишина, R=тон
//     // ------------------------------------------------------------------
//     printf("[TEST] Заполняем буфер: L=тишина, R=440Hz\n");
//     for (i = 0; i < frames; i++) {
//         angle = 2.0f * 3.14159f * freq * i / sample_rate;
//         s = (int)(128 + 100 * sin(angle));
//         if (s > 255) s = 255;
//         if (s < 0) s = 0;
//         dma_buffers[0][i * 2]     = 0x80;               // L — тишина
//         dma_buffers[0][i * 2 + 1] = (unsigned char)s;   // R — тон
//     }
//     setup_dma_controller_only(dma_buffers[0]);
//     dsp_write(DSP_CMD_8BIT_SINGLE_OUTPUT);
//     dsp_write(DSP_MODE_STEREO);
//     printf("[TEST] Слушаем 1.5 сек... (L=тишина, R=тон)\n");
//     for (loop = 0; loop < 3; loop++) {
//         delay(500);
//         setup_dma_controller_only(dma_buffers[0]);
//         dsp_write(DSP_CMD_8BIT_SINGLE_OUTPUT);
//         dsp_write(DSP_MODE_STEREO);
//     }
//     delay(1000);
//     printf("[TEST] Если в первом тесте тон был только в ЛЕВОМ ухе,\n");
//     printf("[TEST] а во втором — только в ПРАВОМ — стерео работает.\n");
//     // Останавливаем
//     dsp_write(DSP_CMD_EXIT_AUTO_INIT);
//     dsp_write(DSP_CMD_PAUSE_DMA);
//     is_playing = 0;
// }

void test_stereo() {
    int i;
    int sample_rate = SAMPLE_RATE;
    int frames = BUFFER_SIZE / 2;
    float angle;
    int s;
    unsigned char tc;
    int quarter = frames / 4;

    // --- Сброс и инициализация DSP ---
    dsp_reset();
    dsp_version = get_dsp_version();
    dsp_write(DSP_CMD_SPEAKER_ON);
    delay(10);

    // Стерео на выходе микшера
    outp(sb_port + 0x04, 0x0E);
    outp(sb_port + 0x05, 0x03);
    delay(10);

    // Частота
    tc = (unsigned char)(256.0f - (1000000.0f / (float)(sample_rate * 2)));
    dsp_write(DSP_CMD_SAMPLE_RATE);
    dsp_write(tc);
    delay(10);

    // Размер блока
    dsp_write(DSP_CMD_8BIT_DMA_SETUP);
    dsp_write(LOBYTE(BUFFER_SIZE - 1));
    dsp_write(HIBYTE(BUFFER_SIZE - 1));
    delay(10);

    // --- Заполняем буфер четырьмя разными секциями ---
    for (i = 0; i < frames; i++) {
        int section = i / quarter;   // 0, 1, 2, 3
        int L = 0x80, R = 0x80;

        switch (section) {
            case 0:   // L=440, R=тишина
                angle = 2.0f * 3.14159f * 440 * i / sample_rate;
                s = (int)(128 + 100 * sin(angle));
                L = s;
                R = 0x80;
                break;
            case 1:   // L=тишина, R=440
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

// int main(int argc, char *argv[])
// {
//     if (argc != 2)
//     {
//         printf("[FILE] File is not found!");
//         return 1;
//     }
//     while (1) {
//         // Прерывание срабатывает, когда DSP доходит до конца блока DMA
//         if (dma_block_finished_flag) {
//             sb_copy_buffer();
//             if (playing_final_chunk) {
//                 is_playing = 0;
//             }
//             dma_block_finished_flag = 0;
//         }
//         if (!is_playing) break; // Если звук доиграл до конца, выйти
//         if (kbhit() && getch() == 27) break; // Если был нажат Escape, выйти
//     }
//     return 0;
// }

// Старая версия
//int play_sound(const char *filename)
// {
//     FILE *soundFile;
//     unsigned char header[44];
//     unsigned short format, num_channels, bitspersample;
//     unsigned int samplerate;
// 	unsigned int len;
//     unsigned int i;
//     unsigned char *wav_data;
//     unsigned int wav_data_size;
//     soundFile = fopen(filename, "rb");
// 	if (fread(header, 1, 44, soundFile) != 44 ||
// 		strncmp((char*)header,      "RIFF", 4) != 0 ||
// 		strncmp((char*)(header+8),  "WAVE", 4) != 0 ||
// 		strncmp((char*)(header+12), "fmt ", 4) != 0)
// 	{
// 		printf("[ERROR] This is not a WAV file: %s\n", filename);
// 		fclose(soundFile);
// 		return -3;
// 	}
// 	format        = *(unsigned short *)(header + 20);
// 	num_channels      = *(unsigned short *)(header + 22);
// 	samplerate    = *(unsigned int   *)(header + 24);
// 	bitspersample = *(unsigned short *)(header + 34);
// 	wav_data_size      = *(unsigned int   *)(header + 40);
// 	if (format != 1 || num_channels != 1 || bitspersample != 8)
// 	{
// 		puts("[ERROR] Only unsigned 8-bit mono PCM is supported!\n");
// 		fclose(soundFile);
// 		return -4;
// 	}
// 	wav_data = malloc(wav_data_size);
// 	fread(wav_data, 1, wav_data_size, soundFile);
// 	fclose(soundFile);
// 	printf("[MEM] Allocated %d bytes for %s\n", wav_data_size, filename);
//     // Выделяем память для двух буферов
//     for (i = 0; i < NUM_BUFFERS; i++) {
//         dma_buffers[i] = alloc_low_dos_memory(dma_buffer_size, 
//                                                &dma_selectors[i], 
//                                                &dma_segments[i]);
//         if (dma_buffers[i] == NULL) {
//             printf("[MEM] Memory allocation error for dma_buffer %d\n", i);
//             cleanup_buffers();
//             return -1;
//         }
//     }
// 	// if (dma_buffer != NULL) {
//     //     printf("[SB] Primary Buffer already initialized\n");
//     //     return 0;
//     // }
//     // dma_buffer = alloc_low_dos_memory(dma_buffer_size, &dma_selector, &dma_segment);
//     // if (dma_buffer == NULL)
// 	// {
// 	// 	printf("[MEM] Memory allocation error for dma_buffer, dma_buffer == NULL\n");
// 	// 	return -1;
// 	// }
//     //Сначала забиваем буфер, и только потом запускаем проигрывание
//     //sb_copy_buffer();
//     dsp_reset();
// 	dsp_version = get_dsp_version();
//     set_sb_sample_rate(samplerate);
// // Заполняем оба буфера перед началом воспроизведения
//     current_playback_position = 0;
//     playing_final_chunk = 0;
//     // Заполняем первый буфер
//     fill_buffer(dma_buffers[0], 0);
//     // Заполняем второй буфер
//     fill_buffer(dma_buffers[1], 1);
//     // Настраиваем DMA на первый буфер
//     setup_dma_for_buffer(dma_buffers[0], 0);
//     current_buffer = 0;
//     next_buffer = 1;
//     // Запускаем воспроизведение
//     dsp_write(DSP_CMD_8BIT_DMA_SETUP);
//     dsp_write(LOBYTE(dma_buffer_size - 1));
//     dsp_write(HIBYTE(dma_buffer_size - 1));
//     dsp_write(DSP_CMD_8BIT_AUTO_OUTPUT);
//     //dsp_write(DSP_CMD_8BIT_AUTO_);
//     dsp_write(DSP_MODE_UNSIGNED);
//     // Показываем прогресс
//     //show_progress();
//     is_playing = 1;
//     first_run = 0;
//     return 0;
// }

#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include <cmath>
#include <vector>
#include <string.h>
#include <map>
#include "score_library.h"
#include <stdio.h>
#include "hardware/uart.h"
#include "hardware/watchdog.h"
//#include "pico/cyw43_arch.h"// 藍牙相關

#define DEBUG_PRINT(fmt, ...) \
    printf("[LINE %d] " fmt "\r\n", __LINE__, ##__VA_ARGS__)   //除錯用的函數，會印出該行

//hardware ID
int int_hardware_ID;

//目前的歌曲
int current_song;
enum Song_ID 
{
    //與Arduino Nano中的順序相同，只是沒有play
    IDLE = 0,
    YOUR_NAME_ENGRAVED_HEREIN,
    A_UNIQUE_FLOWER_IN_THE_WORLD,
    LETS_NOT_LEAVE_OUR_YOUTH_BLANK,
    DISNEY_OPENING,
    DETECTIVE_CONAN,
    TEST_HAND,
    DORAEMON,

    Song_ID_MAX,



};

// 訂定接腳
// 蜂鳴器
    //const uint BUZZER_PIN = 16;
// LED (但經實驗無效？）
    //const uint LED_PIN= 20;

// 定義硬體定址腳位
const uint ADDR_PINS[] = {7, 8, 9};
    // 定義 UART 設定
#define UART0_FROM_ARDUINO uart0



#define UART1_COM_PICO uart1
#define BAUD_RATE 9600
// #define UART0_TX_PIN 12  // 實體 Pin 16，負責發送給 Nano
// #define UART0_RX_PIN 13  // 實體 Pin 17，負責接收來自 Nano

// 定義節拍單位 (ms) 目前無法正確讀取，需修正
#define TEMPO // 一拍 500ms

// 使用 vector 替代固定陣列
//std::vector<Note> current_melody;

enum STATE_ORDER{

    STATE_IDLE,
    STATE_CHECK,
    STATE_READY,
    STATE_PLAY,
    STATE_TEST_HAND,
    STATE_STOP,


    STATE_MAX,
};

//要記得和enum State對上
std::vector<std::string> STATE_NAME={
    "STATE_IDLE",
    "STATE_CHECK",
    "STATE_READY",
    "STATE_PLAY",
    "STATE_TEST_HAND",
    "STATE_STOP",


    "STATE_MAX"




};

uint BUZZER_PIN   = 16;
uint LED_PIN      = 20;
uint UART0_TX_PIN = 12;
uint UART0_RX_PIN = 13;
uint UART1_TX_PIN = 0;
uint UART1_RX_PIN = 1;

/**
 * 計算音符頻率
 * @param noteName 音名 (例如 "C", "C#", "Db")
 * @param octave    八度 (例如 4)
 * @return 頻率 (Hz)
 */
int get_freq(std::string note_name, int octave)
{
    // 定義半音距離 (以 A 為 0)
    static std::map<std::string, int> semitones = {
        {"C", -9}, {"C#", -8}, {"Db", -8}, {"D", -7}, {"D#", -6}, {"Eb", -6}, {"E", -5}, {"F", -4}, {"F#", -3}, {"Gb", -3}, {"G", -2}, {"G#", -1}, {"Ab", -1}, {"A", 0}, {"A#", 1}, {"Bb", 1}, {"B", 2}};

    // 檢查音名是否存在於 map 中
    if (semitones.find(note_name) == semitones.end())
    {
        return 0;
    }

    // 計算與 A4 (440Hz) 的半音總距離 n
    // 公式: n = (octave - 4) * 12 + 偏移量
    int n = (octave - 4) * 12 + semitones[note_name];

    // 使用公式計算頻率: freq = 440 * (2^(n/12))
    double freq = 440.0 * std::pow(2.0, (double)n / 12.0);

    return (int)std::round(freq);
}

/**
 * 解析音符字串並返回頻率
 * @param note 音符字串 (例如 "C#4", "0")
 */
int parse_note(std::string note)
{
    if (note == "R")
        return 0;

    // 拆解字串，C#4" -> 音名為 "C#", 八度為 4
    std::string note_name = note.substr(0, note.length() - 1);
    int octave = note.back() - '0'; // 將 char 轉為 int

    return get_freq(note_name, octave);
}

// // 建立一個函式來快速填入旋律
// void load_melody(std::vector<std::string> notes, std::vector<float> beats)
// {
//     current_melody.clear(); 
//     for (size_t i = 0; i < notes.size(); i++)
//     {
//         // notes[i] 本身就是 string，這才符合 Note的定義
//         current_melody.push_back({notes[i], beats[i]}); 
//     }
// }

// 設定 PWM 頻率的函式
void set_buzzer_freq(uint pin, uint freq)
{
    if (freq == 0)
    {
        pwm_set_enabled(pwm_gpio_to_slice_num(pin), false);
        return;
    }
    uint slice_num = pwm_gpio_to_slice_num(pin);
    uint32_t clock_freq = 125000000; // RP2350 預設通常也是 125MHz
    uint32_t divider = clock_freq / (freq * 65536) + 1;
    uint32_t top = clock_freq / (divider * freq) - 1;

    pwm_set_clkdiv(slice_num, divider);
    pwm_set_wrap(slice_num, top);
    pwm_set_chan_level(slice_num, pwm_gpio_to_channel(pin), top / 2); // 50% 佔空比
    pwm_set_enabled(slice_num, true);
}



// // 播放旋律的通用函式
// void play_melody(uint BUZZER_PIN, uint LED_PIN, const std::vector<Note> &melody_to_play, uint tempo_ms)
// {
//     for (const auto &note : melody_to_play)
//     {
//         // 1. 點亮 LED (3V3 接法，給 0 亮)
//         //gpio_put(LED_PIN, 0);

//         // 2. 播放音符 (如果頻率為 0 則不發聲)
//         if (note.pitch != "R")
//         {
//             gpio_put(LED_PIN, 1);
//             set_buzzer_freq(BUZZER_PIN, parse_note(note.pitch));
//             // 3. 持續時間
//             sleep_ms(note.beats * tempo_ms*0.95);

//             gpio_put(LED_PIN, 0);
//             set_buzzer_freq(BUZZER_PIN, 0);
//             sleep_ms(note.beats * tempo_ms*0.05);
//         }
//         else
//         {
//             gpio_put(LED_PIN, 0);
//             set_buzzer_freq(BUZZER_PIN, 0);
//             // 3. 持續時間
//             sleep_ms(note.beats * tempo_ms);
//         }



//         // 4. 熄滅 LED 並停止聲音
//         gpio_put(LED_PIN, 1);
//         set_buzzer_freq(BUZZER_PIN, 0);

//         // 音符間的短暫停頓
//         sleep_ms(50);
//     }
// }

// 播放核心
void play_song_by_name(std::string name, uint BUZZER_PIN, uint LED_PIN) {
    for (const auto& song : SONG_LIBRARY) {
        if (song.name == name) {
            for (const auto& note : song.notes) {

                 printf("%s,%.3f\r\n",note.pitch.c_str(),note.beats);

                 if (note.pitch != "R")
                 {
                    gpio_put(LED_PIN, 1); //綠燈點亮
                    set_buzzer_freq(BUZZER_PIN, parse_note(note.pitch));


                    // 3. 持續時間
                    sleep_ms(note.beats * 60000/song.tempo * 0.95);

                    gpio_put(LED_PIN, 0); //綠燈熄滅
                    set_buzzer_freq(BUZZER_PIN, 0);
                    sleep_ms(note.beats * 60000/song.tempo *0.05);
                 }
                 else
                 {
                    gpio_put(LED_PIN, 0); //綠燈熄滅
                    set_buzzer_freq(BUZZER_PIN, 0);
                    sleep_ms(note.beats * 60000/song.tempo);
                 }

                 

                 // 4. 綠燈熄滅並停止聲音
                 gpio_put(LED_PIN, 0);
                 set_buzzer_freq(BUZZER_PIN, 0);

                 // 音符間的短暫停頓
                //sleep_ms(50);
                }
            return;
        }
    }
}

//取得pico的硬體定址
int get_hardware_id() {
    

    bool bit0 = gpio_get(ADDR_PINS[0]); //7
    bool bit1 = gpio_get(ADDR_PINS[1]); //8
    bool bit2 = gpio_get(ADDR_PINS[2]); //9

     // 檢查讀到的資訊
    printf("Raw bits: bit2=%d, bit1=%d, bit0=%d\n", bit2, bit1, bit0);

    // 組合二進位 
    return (bit2 << 2) | (bit1 << 1) | bit0;
    //return id;
}

// 根據 Hardware ID 決定腳位
void init_pins(uint &BUZZER_PIN, uint &LED_PIN, 
               uint &UART0_TX_PIN, uint &UART0_RX_PIN,
               uint &UART1_TX_PIN, uint &UART1_RX_PIN)
{
    switch(int_hardware_ID)
    {
        case 0: case 1: case 2: case 3:
            BUZZER_PIN    = 16;
            LED_PIN       = 20;
            UART0_TX_PIN  = 12;
            UART0_RX_PIN  = 13;
            UART1_TX_PIN  = 4;
            UART1_RX_PIN  = 5;
            break;

        case 4: case 5: case 6:
            BUZZER_PIN    = 15;
            LED_PIN       = 11;
            UART0_TX_PIN  = 16;
            UART0_RX_PIN  = 17;
            UART1_TX_PIN  = 4;
            UART1_RX_PIN  = 5;
            break;

        default:
            break;
    }
}

//GPIO與UART的初始化包裝起來
void init_hardware(uint BUZZER_PIN, uint LED_PIN,
                   uint UART0_TX_PIN, uint UART0_RX_PIN,
                   uint UART1_TX_PIN, uint UART1_RX_PIN)
{
    // 初始化硬體定址腳位
    gpio_init(ADDR_PINS[0]);
    gpio_init(ADDR_PINS[1]);
    gpio_init(ADDR_PINS[2]);
    gpio_set_dir(ADDR_PINS[0], GPIO_IN);
    gpio_set_dir(ADDR_PINS[1], GPIO_IN);
    gpio_set_dir(ADDR_PINS[2], GPIO_IN);
    gpio_pull_up(ADDR_PINS[0]);
    gpio_pull_up(ADDR_PINS[1]);
    gpio_pull_up(ADDR_PINS[2]);

    // 讀取 Hardware ID
    sleep_ms(500);
    int_hardware_ID = get_hardware_id();
    sleep_ms(500);

    // 初始化 GPIO
    gpio_init(LED_PIN);
    gpio_init(BUZZER_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);

    // 初始化 UART0
    uart_init(UART0_FROM_ARDUINO, BAUD_RATE);
    gpio_set_function(UART0_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART0_RX_PIN, GPIO_FUNC_UART);

    // 初始化 UART1
    uart_init(UART1_COM_PICO, BAUD_RATE);
    gpio_set_function(UART1_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART1_RX_PIN, GPIO_FUNC_UART);
}

void play_startup_song(uint BUZZER_PIN, uint LED_PIN)
{
    gpio_put(LED_PIN, 1);
    sleep_ms(500);

    switch(int_hardware_ID)
    {
        case 0: play_song_by_name("joy_to_the_world_T1", BUZZER_PIN, LED_PIN); break;
        case 1: play_song_by_name("joy_to_the_world_T2", BUZZER_PIN, LED_PIN); break;
        case 2: play_song_by_name("joy_to_the_world_B1", BUZZER_PIN, LED_PIN); break;
        case 3: play_song_by_name("joy_to_the_world_B2", BUZZER_PIN, LED_PIN); break;
        case 4: play_song_by_name("joy_to_the_world_P1", BUZZER_PIN, LED_PIN); break;
        case 5: play_song_by_name("joy_to_the_world_P2", BUZZER_PIN, LED_PIN); break;
        case 6: play_song_by_name("joy_to_the_world_P3", BUZZER_PIN, LED_PIN); break;
        default: play_song_by_name("mario_die", BUZZER_PIN, LED_PIN); break;
    }
}

void handle_idle(char* temp_buffer, size_t &idx, int &current_State, uint LED_PIN)
{
    DEBUG_PRINT("現在在%s", STATE_NAME[current_State].c_str());
    sleep_ms(2000);

    while (true) {
        if (uart_is_readable(UART0_FROM_ARDUINO)) {
            char c = uart_getc(UART0_FROM_ARDUINO);
            if (c == '\n' || c == '\r') {
                if (idx > 0) {
                    temp_buffer[idx] = '\0';
                    idx = 0;
                    printf("迴圈裡面的buffer是%s\r\n", temp_buffer);

                    if (strcmp(temp_buffer, "PLAY_FLOWER") == 0) {
                        memset(temp_buffer, 0, 128);
                        sleep_ms(2000);
                        current_song = A_UNIQUE_FLOWER_IN_THE_WORLD;
                        current_State = STATE_PLAY;
                    } else if (strcmp(temp_buffer, "PLAY_NAME_ENGRAVED") == 0) {
                        memset(temp_buffer, 0, 128);
                        sleep_ms(2000);
                        current_song = YOUR_NAME_ENGRAVED_HEREIN;
                        current_State = STATE_PLAY;
                    } else if (strcmp(temp_buffer, "PLAY_DISNEY") == 0) {
                        memset(temp_buffer, 0, 128);
                        sleep_ms(2000);
                        current_song = DISNEY_OPENING;
                        current_State = STATE_PLAY;
                    } else if (strcmp(temp_buffer, "PLAY_YOUTH_BLANK") == 0) {
                        memset(temp_buffer, 0, 128);
                        sleep_ms(2000);
                        current_song = LETS_NOT_LEAVE_OUR_YOUTH_BLANK;
                        current_State = STATE_PLAY;
                    } else if (strcmp(temp_buffer, "PLAY_DETECTIVE_CONAN") == 0) {
                        memset(temp_buffer, 0, 128);
                        sleep_ms(2000);
                        current_song = DETECTIVE_CONAN;
                        current_State = STATE_PLAY;
                    } else if (strcmp(temp_buffer, "DRIVE_CAR") == 0) {
                        memset(temp_buffer, 0, 128);
                        sleep_ms(500);
                        if (int_hardware_ID == 2) {
                            gpio_put(LED_PIN, 1);
                            sleep_ms(5000);
                            gpio_put(LED_PIN, 0);
                        }
                        sleep_ms(2000);
                        current_State = STATE_STOP;
                    }

                    idx = 0;
                    break;
                }
            } else {
                if (idx < 127) {
                    temp_buffer[idx++] = c;
                }
            }
        }
    }
}

void handle_play(uint BUZZER_PIN, uint LED_PIN, int &current_State)
{
    DEBUG_PRINT("現在在%s", STATE_NAME[current_State].c_str());
    sleep_ms(2000);

    switch(current_song)
    {
        case A_UNIQUE_FLOWER_IN_THE_WORLD:
            switch(int_hardware_ID) {
                case 0: case 6: play_song_by_name("A_UNIQUE_FLOWER_IN_THE_WORLD_T1", BUZZER_PIN, LED_PIN); break;
                case 1: case 2: case 4: play_song_by_name("A_UNIQUE_FLOWER_IN_THE_WORLD_T2", BUZZER_PIN, LED_PIN); break;
                case 3: case 5: play_song_by_name("A_UNIQUE_FLOWER_IN_THE_WORLD_B1", BUZZER_PIN, LED_PIN); break;
            }
            break;

        case YOUR_NAME_ENGRAVED_HEREIN:
            DEBUG_PRINT("現在準備播放刻在我心底的名字");
            switch(int_hardware_ID) {
                case 0: case 4: play_song_by_name("YOUR_NAME_ENGRAVED_HEREIN_00", BUZZER_PIN, LED_PIN); break;
                case 1: play_song_by_name("YOUR_NAME_ENGRAVED_HEREIN_01", BUZZER_PIN, LED_PIN); break;
                case 2: play_song_by_name("YOUR_NAME_ENGRAVED_HEREIN_02", BUZZER_PIN, LED_PIN); break;
                case 3: play_song_by_name("YOUR_NAME_ENGRAVED_HEREIN_03", BUZZER_PIN, LED_PIN); break;
                case 5: play_song_by_name("YOUR_NAME_ENGRAVED_HEREIN_04", BUZZER_PIN, LED_PIN); break;
                case 6: play_song_by_name("YOUR_NAME_ENGRAVED_HEREIN_00", BUZZER_PIN, LED_PIN); break;
            }
            break;

        case LETS_NOT_LEAVE_OUR_YOUTH_BLANK:
            switch(int_hardware_ID) {
                case 0: play_song_by_name("LETS_NOT_LEAVE_OUR_YOUTH_BLANK_00", BUZZER_PIN, LED_PIN); break;
                case 1: play_song_by_name("LETS_NOT_LEAVE_OUR_YOUTH_BLANK_01", BUZZER_PIN, LED_PIN); break;
                case 2: play_song_by_name("LETS_NOT_LEAVE_OUR_YOUTH_BLANK_02", BUZZER_PIN, LED_PIN); break;
                case 3: play_song_by_name("LETS_NOT_LEAVE_OUR_YOUTH_BLANK_03", BUZZER_PIN, LED_PIN); break;
                case 4: play_song_by_name("LETS_NOT_LEAVE_OUR_YOUTH_BLANK_04", BUZZER_PIN, LED_PIN); break;
                case 5: play_song_by_name("LETS_NOT_LEAVE_OUR_YOUTH_BLANK_05", BUZZER_PIN, LED_PIN); break;
                case 6: play_song_by_name("LETS_NOT_LEAVE_OUR_YOUTH_BLANK_06", BUZZER_PIN, LED_PIN); break;
            }
            break;

        case DISNEY_OPENING:
            DEBUG_PRINT("現在準備播放迪士尼之歌");
            switch(int_hardware_ID) {
                case 0: case 1: case 2: play_song_by_name("disney_star_T1", BUZZER_PIN, LED_PIN); break;
                case 3: play_song_by_name("disney_star_B1", BUZZER_PIN, LED_PIN); break;
                case 4: case 6: play_song_by_name("disney_star_T2", BUZZER_PIN, LED_PIN); break;
                case 5: play_song_by_name("disney_star_B2", BUZZER_PIN, LED_PIN); break;
            }
            break;

        case DETECTIVE_CONAN:
            DEBUG_PRINT("現在準備播放名偵探柯南");
            sleep_ms(500);
            switch(int_hardware_ID) {
                case 0: play_song_by_name("DETECTIVE_CONAN_00", BUZZER_PIN, LED_PIN); break;
                case 1: play_song_by_name("DETECTIVE_CONAN_01", BUZZER_PIN, LED_PIN); break;
            }
            break;

        case DORAEMON:
            play_song_by_name("doraemon", BUZZER_PIN, LED_PIN);
            break;

        default:
            play_song_by_name("mario_die", BUZZER_PIN, LED_PIN);
            break;
    }

    current_State = STATE_STOP;
}

void handle_stop(char* temp_buffer, size_t &idx, int &current_State)
{
    DEBUG_PRINT("現在在%s", STATE_NAME[current_State].c_str());
    sleep_ms(2000);

    memset(temp_buffer, 0, 128);
    idx = 0;

    if(int_hardware_ID == 0)
    {
        uart_puts(UART0_FROM_ARDUINO, "PLAY_DONE\n");
        DEBUG_PRINT("我送出訊號給Arduino囉！");
        DEBUG_PRINT("要回去STATE_IDLE了，再會～");
    }

    current_State = STATE_IDLE;
}

void init_addr_pins()
{
              // 6. 初始化硬體定址腳位
            // 因為有三片要逆向，必須用硬體定址的結果來決定腳位
            gpio_init(ADDR_PINS[0]);
            gpio_init(ADDR_PINS[1]);
            gpio_init(ADDR_PINS[2]);

            // 硬體定址腳位設定為輸入模式
            gpio_set_dir(ADDR_PINS[0], GPIO_IN);
            gpio_set_dir(ADDR_PINS[1], GPIO_IN);
            gpio_set_dir(ADDR_PINS[2], GPIO_IN);

            // 硬體定址腳位啟動內部上拉電阻 (Pull-up)
            // 當腳位沒接地時，會穩定讀到 1；接地時讀到 0
            gpio_pull_up(ADDR_PINS[0]);
            gpio_pull_up(ADDR_PINS[1]);
            gpio_pull_up(ADDR_PINS[2]);

            sleep_ms(500);
            int_hardware_ID = get_hardware_id();

            
            sleep_ms(500);



}

int main(){
    
    int current_State = STATE_IDLE; 

    
            stdio_init_all();

            init_addr_pins();

            init_hardware(BUZZER_PIN, LED_PIN, UART0_TX_PIN, UART0_RX_PIN, UART1_TX_PIN, UART1_RX_PIN);
            init_pins(BUZZER_PIN, LED_PIN, UART0_TX_PIN, UART0_RX_PIN, UART1_TX_PIN, UART1_RX_PIN);

            char temp_buffer[128];
            char temp_buffer2[10];
            size_t idx = 0;
            size_t idx2 = 0;

              sleep_ms(2000);
            

            
            
            gpio_put(LED_PIN, 1);
            sleep_ms(500);
            
            
            //sleep_ms(500); //要切換到介面需要一點時間


            
        
            //初始化成功的聲音
            play_startup_song(BUZZER_PIN, LED_PIN);

            bool bool_shake_hand;
            bool bReady_6=false;

            DEBUG_PRINT("現在要進入迴圈了");
            //play_song_by_name("totoro_main", BUZZER_PIN, LED_PIN);
            //play_song_by_name("disney_star_T1", BUZZER_PIN, LED_PIN);
                    while(1)
                    {
                        switch(current_State)
                        {
                         case STATE_IDLE:
                                  handle_idle(temp_buffer, idx, current_State, LED_PIN);
                            break;

                         case STATE_TEST_HAND:
                            // TODO: 單節點控制機制，尚未實作
                            // 預計功能：感應卡片後只觸發指定 ID 的 Pico
                            bool_shake_hand = true;

                            //從pico6開始
                            if(int_hardware_ID==6 && bReady_6 == false)
                            {
                                bReady_6 = true;
                                DEBUG_PRINT("我是6號\r\n");
                                uart_puts(UART1_COM_PICO, "PICO6 OK\n"); //輸出
                                DEBUG_PRINT("我送出訊號給pico5囉！\r\n");
                                DEBUG_PRINT("要去STATE_READY了，再會～\r\n");
                                current_song = DORAEMON;
                                current_State = STATE_READY;
                            }
                            else
                            {
                            while(bool_shake_hand){      // 檢查 UART 是否有資料可讀
                                    if (uart_is_readable(UART1_COM_PICO)) 
                                    {
                                            char c2 = uart_getc(UART1_COM_PICO);
                                            DEBUG_PRINT("現在的buffer是%s\r\n",temp_buffer2); 
                                            
                                            if (c2 == '\n' || c2 == '\r') {
                                                if (idx2 > 0) {
                                                    temp_buffer2[idx2] = '\0'; // 結束字串
                                                    
                                                    
                                                    idx2 = 0;     

                                                }
                                                    idx2 = 0; // 不管比對成功與否，收到換行，index就要歸零
                                                    
                                                    //break;
                                                    
                                            }
                                            
                                            else {
                                                if (idx2 < sizeof(temp_buffer2) - 1) {
                                                    temp_buffer2[idx2++] = c2;
                                                }
                                            }

                                    }
                                }
                            }
                            //sleep_ms(2000);
                                

                            break;

                            
                        case STATE_READY:
                            DEBUG_PRINT("現在在%s\r\n",STATE_NAME[current_State].c_str());
                            current_State = STATE_PLAY;
                            break;

                        case STATE_PLAY:
                            handle_play(BUZZER_PIN, LED_PIN, current_State);
                            break;
                       
                        case STATE_STOP:

                            handle_stop(temp_buffer, idx, current_State);
                            break;

                        default:

                            printf("程式錯誤\r\n");
                        break;


                    }
            
                }

    
    return 0;
}

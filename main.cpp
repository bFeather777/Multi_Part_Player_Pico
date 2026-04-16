#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include <cmath>
#include <vector>
#include <string.h>
#include <map>
#include "score_library.h"
#include <stdio.h>
#include "hardware/uart.h"


//hardware ID
int int_hardware_ID;

//目前的歌曲
std::string current_song;

// 訂定接腳
// 蜂鳴器
    //const uint BUZZER_PIN = 16;
// LED (但經實驗無效？）
    //const uint LED_PIN= 20;

// 定義硬體定址腳位
const uint ADDR_PINS[] = {7, 8, 9};
    // 定義 UART 設定
#define UART_ID uart0
#define BAUD_RATE 9600
#define UART_TX_PIN 12  // 實體 Pin 16，負責發送給 Nano
#define UART_RX_PIN 13  // 實體 Pin 17，負責接收來自 Nano

// 定義節拍單位 (ms) 目前無法正確讀取，需修正
#define TEMPO // 一拍 500ms

// 使用 vector 替代固定陣列
//std::vector<Note> current_melody;

enum State{

    STATE_IDLE,
    STATE_PLAY,
    STATE_STOP,


    STATE_MAX,
};

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
    int id = 0;

    bool bit0 = gpio_get(ADDR_PINS[0]); //7
    bool bit1 = gpio_get(ADDR_PINS[1]); //8
    bool bit2 = gpio_get(ADDR_PINS[2]); //9

     // 檢查讀到的資訊
    printf("Raw bits: bit2=%d, bit1=%d, bit0=%d\n", bit2, bit1, bit0);

    // 組合二進位 
    return (bit2 << 2) | (bit1 << 1) | bit0;
    //return id;
}

int main(){
    
    int int_repeat_times = 2;
    int int_repeat_count = 0;
    State current_State = STATE_IDLE;

    
            stdio_init_all();

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
            // 這樣當腳位沒接地時，會穩定讀到 1；接地時讀到 0
            gpio_pull_up(ADDR_PINS[0]);
            gpio_pull_up(ADDR_PINS[1]);
            gpio_pull_up(ADDR_PINS[2]);

            sleep_ms(500);
            int_hardware_ID = get_hardware_id();

            //printf("這一個pico的hard id是:%d\r\n",int_hardware_ID);
            sleep_ms(500);

            //先作設定，接下來的switch會再修正
            uint BUZZER_PIN = 16;
            uint LED_PIN= 20;

            switch(int_hardware_ID)
            {
                case 0:
                case 1:
                case 2:
                case 3:
                    // 蜂鳴器
                    BUZZER_PIN = 16;
                    // LED 
                    LED_PIN= 20;

                break;

                case 4:
                case 5:
                case 6:
                    // 蜂鳴器
                    BUZZER_PIN = 0;
                    // LED 
                    LED_PIN= 4;

                break;

                default:

                break;

            }

            // 2. 初始化 GPIO
            gpio_init(LED_PIN);
            gpio_init(BUZZER_PIN);

            // 3. 設定方向 (LED 是輸出)
            gpio_set_dir(LED_PIN, GPIO_OUT);
            
            // 4. 設定 PWM 功能 (蜂鳴器腳位要切換成 PWM 模式)
            gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);

            

            // 5. 初始化 UART
            uart_init(UART_ID, BAUD_RATE);
            gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
            gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);


             //sleep_ms(2000);
            

            //printf("Pico 接收端已就緒，等待 Arduino 訊號...\n");

            char buffer[128];
            int idx = 0;

              sleep_ms(2000);
            

            
            //printf("LED要亮起來了唷\r\n");
            gpio_put(LED_PIN, 1);
            sleep_ms(500);
            
            //printf("要開始播音樂了\r\n");
            //sleep_ms(500); //要切換到介面需要一點時間


            //printf("初始化完成的音樂\r\n");
        
            //初始化成功的聲音
            switch(int_hardware_ID)
            {
                case 0:
                    play_song_by_name("joy_to_the_world_T1", BUZZER_PIN, LED_PIN);

                    break;
                
                case 1:
                    play_song_by_name("joy_to_the_world_T2", BUZZER_PIN, LED_PIN);

                    break;

                case 2:
                    play_song_by_name("joy_to_the_world_B1", BUZZER_PIN, LED_PIN);

                    break;

                case 3:
                    play_song_by_name("joy_to_the_world_B2", BUZZER_PIN, LED_PIN);

                    break;

                default:
                    play_song_by_name("mario_die", BUZZER_PIN, LED_PIN);

                    break;
            }

            //play_song_by_name("totoro_main", BUZZER_PIN, LED_PIN);
            //play_song_by_name("disney_star_T1", BUZZER_PIN, LED_PIN);
                    while(1)
                    {
                        switch(current_State)
                        {
                            case STATE_IDLE:

                                //printf("現在在%d\r\n",static_cast<int>(current_State));
                                sleep_ms(2000);

                                while (true) {
                                        // 檢查 UART 是否有資料可讀
                                        if (uart_is_readable(UART_ID)) {
                                            char c = uart_getc(UART_ID);
                                            //下面這一行可以監控由Arduino Nano送來的編號
                                            //printf("現在的buffer是%s\r\n",buffer); 
                                            // 判斷是否為換行符號 (表示指令結束)
                                            if (c == '\n' || c == '\r') {
                                                if (idx > 0) {
                                                    buffer[idx] = '\0'; // 結束字串
                                                    //printf(">>> Pico 成功接收指令: [%s]\n", buffer);
                                                    int current_idx = idx; 
                                                    idx = 0;
                                                    //下面這一行可以監控由Arduino Nano送來的編號
                                                    //printf("迴圈裡面的buffer是%s\r\n",buffer);       
                                                    if(strcmp(buffer, "PLAY_A_UNIQUE_FLOWER_IN_THE_WORLD") == 0)
                                                    {
                                                        printf("要去STATE_PLAY啦～\r\n");
                                                        memset(buffer, 0, sizeof(buffer)); 
                                                        sleep_ms(2000);
                                                        current_song = "A_UNIQUE_FLOWER_IN_THE_WORLD";
                                                        current_State = STATE_PLAY;
                                                    }
                                                    else if (strcmp(buffer, "PLAY_YOUR_NAME_ENGRAVED_HEREIN") == 0)
                                                    {
                                                        /* code */
                                                        printf("要去STATE_PLAY啦～\r\n");
                                                        memset(buffer, 0, sizeof(buffer)); 
                                                        sleep_ms(2000);
                                                        current_song = "YOUR_NAME_ENGRAVED_HEREIN";
                                                        current_State = STATE_PLAY;
                                                    }
                                                    
                                                    idx = 0;
                                                    
                                                    break;
                                                    
                                                }
                                            } else {
                                                if (idx < sizeof(buffer) - 1) {
                                                    buffer[idx++] = c;
                                                }
                                            }
                                        }
                                }


                        
                            break;

                        case STATE_PLAY:

                                        
                            //play_song_by_name("joy_to_the_world_B1", BUZZER_PIN, LED_PIN);

                            printf("現在在%d\r\n",static_cast<int>(current_State));
                             switch(int_hardware_ID)
                            {
                                case 0:
                                    //play_song_by_name("disney_star_T1", BUZZER_PIN, LED_PIN);
                                    play_song_by_name("A_UNIQUE_FLOWER_IN_THE_WORLD_T1", BUZZER_PIN, LED_PIN);

                                    break;
                                
                                case 1:
                                    //play_song_by_name("disney_star_T2", BUZZER_PIN, LED_PIN);
                                    play_song_by_name("A_UNIQUE_FLOWER_IN_THE_WORLD_T2", BUZZER_PIN, LED_PIN);

                                    break;

                                case 2:
                                    //play_song_by_name("disney_star_B1", BUZZER_PIN, LED_PIN);
                                    play_song_by_name("A_UNIQUE_FLOWER_IN_THE_WORLD_T2", BUZZER_PIN, LED_PIN);

                                    break;

                                case 3:
                                    //play_song_by_name("disney_star_B2", BUZZER_PIN, LED_PIN);
                                    play_song_by_name("A_UNIQUE_FLOWER_IN_THE_WORLD_B1", BUZZER_PIN, LED_PIN);

                                    break;

                                default:
                                    play_song_by_name("mario_die", BUZZER_PIN, LED_PIN);

                                    break;
                            }
                            
                            //sleep_ms(2000);
                            printf("要去STATE_STOP啦～\r\n");
                            //sleep_ms(2000);
                            current_State = STATE_STOP;
                            

                            break;

                        case STATE_STOP:

                            // if(int_repeat_count<int_repeat_times)
                            // {
                                printf("現在在%d\r\n",static_cast<int>(current_State));
                                //sleep_ms(2000);
                                printf("要去STATE_IDLE啦～\r\n");
                                //sleep_ms(2000);
                                memset(buffer, 0, sizeof(buffer)); // 將 buffer 全部填 0
                                idx = 0;                          // 重置索引指標
                                current_State = STATE_IDLE;
                            // }
                            // else
                            // {
                            //     sleep_ms(2000);
                            //     printf("已達到重播次數啦～再見囉\r\n");
                            //     return 0;

                            // }

                        break;

                        default:

                        printf("程式錯誤\r\n");
                        break;


                    }
            
                }

    
    return 0;
}

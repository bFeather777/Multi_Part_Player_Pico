#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include <cmath>
#include <vector>
#include <string>
#include <map>
#include "score_library.h"

struct Note;

int parse_note(std::string note,int pitch);
void load_melody(std::vector<std::string> notes, std::vector<float> beats);
void set_buzzer_freq(uint pin, uint freq);
int get_freq(std::string note_name, int octave);
void play_melody(uint buzzer_pin, uint led_pin, const std::vector<Note> &melody_to_play, uint tempo_ms);
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include <cmath>
#include <vector>
#include <string>
#include <map>
#include "score_library.h"

int get_freq(std::string note_name, int octave);
int parse_note(std::string note);
void set_buzzer_freq(uint pin, uint freq);
void play_melody(uint BUZZER_PIN, uint LED_PIN, const std::vector<Note> &melody_to_play, uint tempo_ms);
void play_song_by_name(std::string name, uint BUZZER_PIN, uint LED_PIN);
int get_hardware_id();
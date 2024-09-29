

#ifndef BUZZER_DRIVER_H
#define BUZZER_DRIVER_H


void init_buzzer();
void stop_buzzer();
void launch_buzz_pattern(const int* pattern, int patternLength, int repeatCount);
void beep(int ms_duration);
void incremental_buzz_pattern(int duration_ms);


#endif /* BUZZER_DRIVER_H */
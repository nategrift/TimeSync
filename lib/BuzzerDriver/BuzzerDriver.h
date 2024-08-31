

#ifndef BUZZER_DRIVER_H
#define BUZZER_DRIVER_H


void init_buzzer();
void stop_buzzer();
void launch_buzz_pattern(const int* pattern, int patternLength, int repeatCount);


#endif /* BUZZER_DRIVER_H */
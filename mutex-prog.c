// How to compile: $ gcc mutex-prog.c -o mutex -lpthread

#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>

#define USLEEP_TIME 350000

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

int turn_leds_on = 1;
int speed = 100;
char *led_color = "G";

// https://stackoverflow.com/questions/13460934/strcpy-using-pointers
// Copying char* to another char*
void str_cpy(char *dst, const char *src) {
   while (*src != '\0') {
      *dst++ = *src++; 
   }
   *dst = '\0';
}

void update_leds(int step, int led_number) {
    char *aux_led_color = malloc(sizeof(char)*(strlen(led_color) + 1));
    
    if (led_number < step)
        str_cpy(aux_led_color, "O"); // turn led off because time has passed for this led
    else
        str_cpy(aux_led_color, led_color);

    printf("L_%d_%s ", led_number, aux_led_color);
    free (aux_led_color);
}

void* change_speed(void* arg) {
    for(;;) {
        while (turn_leds_on == 1) {
            pthread_mutex_lock(&lock);
            for(int step = 1; step <=17; step++) {
                for(int led_number = 1; led_number <=16; led_number++) {
                    update_leds(step, led_number);
                }
                printf("\n");
                usleep(USLEEP_TIME);
            }
            pthread_mutex_unlock(&lock);
            turn_leds_on = 0;
        }
    }
}

// https://stackoverflow.com/questions/9547868/is-there-a-way-to-get-user-input-without-pressing-the-enter-key
void* keyboard_listener(void* arg) {
    struct termios old_tio, new_tio;
    unsigned char c;

    /* get the terminal settings for stdin */
    tcgetattr(STDIN_FILENO,&old_tio);
    /* we want to keep the old setting to restore them a the end */
    new_tio=old_tio;
    /* disable canonical mode (buffered i/o) and local echo */
    new_tio.c_lflag &=(~ICANON & ~ECHO);
    /* set the new settings immediately */
    tcsetattr(STDIN_FILENO,TCSANOW,&new_tio);
    
    for (;;) {
        c = getchar();
        if (c == 'S') {
            /* The pthread_mutex_trylock() function shall be equivalent to pthread_mutex_lock(), 
             * except that if the mutex object referenced by mutex is currently locked (by any 
             * thread, including the current thread), the call shall return immediately. */
            if (pthread_mutex_trylock(&lock) != 0) {
                led_color = "R";
                printf("...I can't change speed! MUTEX BLOCK! speed is %d :(\n", speed);
            }
            else {
                led_color = "G";
                speed++;
                turn_leds_on = 1;
                printf("...I CAN change speed. MUTEX UNBLOCK! speed is %d :)\n", speed);                              
                pthread_mutex_unlock(&lock);
            }
        }
        else if (c == 's') {                                                                             
            if (pthread_mutex_trylock(&lock) != 0) {                                                
                led_color = "R";                                                                    
                printf("...I can't change speed! MUTEX BLOCK! speed is %d :(\n", speed);
            }                                                                                       
            else {                                                                                  
                led_color = "G";                                                                    
                speed--;                                                                            
                turn_leds_on = 1;                                                                           
                printf("...I CAN change speed. MUTEX UNBLOCK! speed is %d :)\n", speed);                              
                pthread_mutex_unlock(&lock);                                                        
            }                                                                                       
        }
        else {
            printf("Error: command not recognized\n");
        }
    }
}

int main() {

    pthread_t speed_thread;
    pthread_t keyboard_listener_thread;

    pthread_create(&speed_thread, NULL, change_speed, NULL);
    pthread_create(&keyboard_listener_thread, NULL, keyboard_listener, NULL);

    // wait for all threads to finish
    pthread_join((pthread_t)speed_thread, NULL);
    pthread_join((pthread_t)keyboard_listener_thread, NULL);

    return 1;
}

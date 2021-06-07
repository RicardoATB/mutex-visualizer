// How to compile: $ gcc mutex-prog.c -o mutex -lpthread

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <linux/spi/spidev.h>
#include <pthread.h>
#include <time.h>
#include <string.h>
#include <termios.h>
/*------------------------------------------------------------------------------------------------*/
#define SPI_PATH "/dev/spidev1.0"

/* Slave Addresses */
#define	GREEN_ADDR	0b01000000
#define RED_ADDR 	0b01000010
#define OSC_ADDR 	0b01000100

/* Register Addresses */
#define IODIRA		0x00 // enable I/Os as either inputs or outputs
#define IODIRB		0x01 // enable I/Os as either inputs or outputs
#define GPIOA		0x12 // bank A
#define GPIOB		0x13 // bank B
#define IOCON_BANK0	0x0A // used to enable HAEN ("H"ardware "A"ddress "EN"able)

/* GPIO mask */
#define MASK_ALL_OUTPUT	0x00 // this bitmask will set all I/O pins of one bank to output (0)
#define USLEEP_TIME 350000
/*------------------------------------------------------------------------------------------------*/
static uint16_t spi_delay_usecs = 0;
static uint32_t spi_speed_hz = 1000000;
static uint16_t spi_bits_per_word = 8;
char *led_color = "G";
int spi_fd;
int status;
int turn_leds_on = 1;
int speed = 100;

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
/*------------------------------------------------------------------------------------------------*/
void write_spi(uint8_t slave_addr, uint8_t reg, uint8_t data) {
    	struct spi_ioc_transfer spi;
	uint8_t spi_buffer[3];

	spi_buffer[0] = slave_addr;
	spi_buffer[1] = reg;
	spi_buffer[2] = data;

	spi.tx_buf = (unsigned long) spi_buffer;
	spi.len = 3;
	spi.delay_usecs = spi_delay_usecs;
	spi.speed_hz = spi_speed_hz;
	spi.bits_per_word = spi_bits_per_word;

   	ioctl(spi_fd, SPI_IOC_MESSAGE(1), &spi);
}
/*------------------------------------------------------------------------------------------------*/
// BeagleBone Black pins: P9_17 (~CS), P9_18 (MOSI), P9_22(CLK)
int config_SPI_pins_BBB()
{
    int ret;
    status = system("config-pin p9_17 spi_cs; \
	  	     config-pin p9_18 spi; \
		     config-pin p9_22 spi_sclk");
    
    ret = WEXITSTATUS(status); // if no errors occur, WEXITSTATUS returns 0
    return ret;
}
/*------------------------------------------------------------------------------------------------*/
// https://stackoverflow.com/questions/13460934/strcpy-using-pointers
// Copying char* to another char*
void str_cpy(char *dst, const char *src) {
   while (*src != '\0') {
      *dst++ = *src++; 
   }
   *dst = '\0';
}
/*------------------------------------------------------------------------------------------------*/
void update_leds(int step, int led_number) {
    char *aux_led_color = malloc(sizeof(char)*(strlen(led_color) + 1));
    
    if (led_number < step)
        str_cpy(aux_led_color, "O"); // turn led off because time has passed for this led
    else
        str_cpy(aux_led_color, led_color);

    printf("L_%d_%s ", led_number, aux_led_color);
    free (aux_led_color);
}
/*------------------------------------------------------------------------------------------------*/
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
/*------------------------------------------------------------------------------------------------*/
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
/**************************************************************************************************/
int main() {

   int fd, i;					// file handle and loop counter
   unsigned char null=0x00;		// sending only a single char
   //uint8_t mode = 3;			// SPI mode 3
   uint8_t mode = 0;

   // The following calls set up the SPI bus properties
   if ((fd = open(SPI_PATH, O_RDWR))<0){
      perror("SPI Error: Can't open device.");
      return -1;
   }
   if (ioctl(fd, SPI_IOC_WR_MODE, &mode)==-1){
      perror("SPI: Can't set SPI mode.");
      return -1;
   }
   if (ioctl(fd, SPI_IOC_RD_MODE, &mode)==-1){
      perror("SPI: Can't get SPI mode.");
      return -1;
   }
   printf("SPI Mode is: %d\n", mode);
   
   // The following calls set up the SPI bus properties
   if ((spi_fd = open(SPI_PATH, O_RDWR))<0){
      perror("SPI Error: Can't open device");
      return -1;
   }
   if (ioctl(spi_fd, SPI_IOC_WR_MODE, &mode)==-1){
      perror("SPI: Can't set SPI mode");
      return -1;
   }
   if (ioctl(spi_fd, SPI_IOC_RD_MODE, &mode)==-1){
      perror("SPI: Can't get SPI mode");
      return -1;
   }
   printf("SPI Mode is: %d\n", mode);


   	if (config_SPI_pins_BBB() != 0) {
        	perror("SPI Error: Can't configure SPI pins");
		return status;
	}
	
	
   	write_spi(GREEN_ADDR, IOCON_BANK0, 0x08); // set HAEN bit to allow multiple ICs (slave addr)

	write_spi(GREEN_ADDR, IODIRA, 0x00);      // GREEN_LED_DRIVER IC: port_A ==> output
	write_spi(GREEN_ADDR, IODIRB, 0x00);      //		          port_B ==> output
	
	write_spi(RED_ADDR, IODIRA, 0x00);        // RED_LED_DRIVER IC: port_A ==> output
	write_spi(RED_ADDR, IODIRB, 0x00);        //		        port_B ==> output
	
	write_spi(OSC_ADDR, IODIRA, 0x00);        // OSC_DRIVER IC: port_A ==> output
	write_spi(OSC_ADDR, IODIRB, 0x00);        //		    port_B ==> output
	
	
	write_spi(GREEN_ADDR, GPIOA, 0xFF);
	write_spi(GREEN_ADDR, GPIOB, 0x00);
	
	write_spi(RED_ADDR, GPIOA, 0x00);
	write_spi(RED_ADDR, GPIOB, 0x00);

	write_spi(OSC_ADDR, GPIOA, 0xFF);
	write_spi(OSC_ADDR, GPIOB, 0x00);
	
	close(spi_fd);
	/***************************************************************************/
    pthread_t speed_thread;
    pthread_t keyboard_listener_thread;

    pthread_create(&speed_thread, NULL, change_speed, NULL);
    pthread_create(&keyboard_listener_thread, NULL, keyboard_listener, NULL);

    // wait for all threads to finish
    pthread_join((pthread_t)speed_thread, NULL);
    pthread_join((pthread_t)keyboard_listener_thread, NULL);

    return 1;
}

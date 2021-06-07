/**  SPI C Seven-Segment Display Example, Written by Derek Molloy (www.derekmolloy.ie)
*    for the book Exploring BeagelBone. Based on the spidev_test.c code
*    example at www.kernel.org  */


#include <stdlib.h> // used for "system" call
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <linux/spi/spidev.h>
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

static uint16_t spi_delay_usecs = 0;
static uint32_t spi_speed_hz = 1000000;
static uint16_t spi_bits_per_word = 8;
int spi_fd;
int status;

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

/*--------------------------------------------------------------------------------*/
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
/**********************************************************************************/
int main(){
   int fd, i;                              // file handle and loop counter
   unsigned char null=0x00;                // sending only a single char
   //uint8_t mode = 3;                       // SPI mode 3
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
	write_spi(GREEN_ADDR, GPIOB, 0xFF);
	
	write_spi(RED_ADDR, GPIOA, 0xFF);
	write_spi(RED_ADDR, GPIOB, 0xFF);

	write_spi(OSC_ADDR, GPIOA, 0xFF);
	write_spi(OSC_ADDR, GPIOB, 0xFF);
	
	close(spi_fd);

	return 0;
}

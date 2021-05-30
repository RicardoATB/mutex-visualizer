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

// The binary data that describes the LED state for each symbol
// A(top)         B(top right) C(bottom right)  D(bottom)
// E(bottom left) F(top left)  G(middle)        H(dot)
const unsigned char symbols[16] = {                   //(msb) HGFEDCBA (lsb)
     0b00111111, 0b00000110, 0b01011011, 0b01001111,  // 0123
     0b01100110, 0b01101101, 0b01111101, 0b00000111,  // 4567
     0b01111111, 0b01100111, 0b01110111, 0b01111100,  // 89Ab
     0b00111001, 0b01011110, 0b01111001, 0b01110001   // CdEF
};

const unsigned char ratb_symbols[10] = { GREEN_ADDR,
					 IODIRA, MASK_ALL_OUTPUT, 
					 GREEN_ADDR,
					 IODIRB, MASK_ALL_OUTPUT,
					 GREEN_ADDR,
					 GPIOA, 0xFF, 0xFF };

int transfer(int fd, unsigned char send[], unsigned char rec[], int len){
   struct spi_ioc_transfer transfer;        //the transfer structure
   transfer.tx_buf = (unsigned long) send;  //the buffer for sending data
   transfer.rx_buf = (unsigned long) rec;   //the buffer for receiving data
   transfer.len = len;                      //the length of buffer
   transfer.speed_hz = 1000000;             //the speed in Hz
   transfer.bits_per_word = 8;              //bits per word
   transfer.delay_usecs = 0;                //delay in us
   transfer.cs_change = 0;       // affects chip select after transfer
   transfer.tx_nbits = 0;        // no. bits for writing (default 0)
   transfer.rx_nbits = 0;        // no. bits for reading (default 0)
   transfer.pad = 0;             // interbyte delay - check version

   // send the SPI message (all of the above fields, inc. buffers)
   int status = ioctl(fd, SPI_IOC_MESSAGE(1), &transfer);
   if (status < 0) {
      perror("SPI: SPI_IOC_MESSAGE Failed");
      return -1;
   }
   return status;
}


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

/**********************************************************************************/
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
/**********************************************************************************/
   /*
   for (i=0; i<=15; i++)
   {
      // This function can send and receive data, just sending now
      if (transfer(fd, (unsigned char*) &symbols[i], &null, 1)==-1){
         perror("Failed to update the display");
         return -1;
      }
      printf("%4d\r", i);   // print the number in the terminal window
      fflush(stdout);       // need to flush the output, no \n
      usleep(2000000);
   }
   close(fd);               // close the file
   */

/**********************************************************************************/
/*
   for (i = 0; i <= 9; i++)
   {
   	// This function can send and receive data, just sending now
   	if (transfer(fd, (unsigned char*) &ratb_symbols[i], &null, 1)==-1) {
   		perror("Failed to update the display");
        	return -1;
	}
   }
   
   close(fd);               // close the file
   */
/**********************************************************************************/
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
	write_spi(RED_ADDR, GPIOB, 0xFF);

	write_spi(OSC_ADDR, GPIOA, 0xFF);
	write_spi(OSC_ADDR, GPIOB, 0x00);
	
	close(spi_fd);

	return 0;
}

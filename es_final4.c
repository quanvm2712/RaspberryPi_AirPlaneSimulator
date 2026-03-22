#include <stdio.h>
#include <stdlib.h>
#include <wiringPi.h>
#include <wiringPiSPI.h>
#include <stdint.h>
#include <mysql/mysql.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <fcntl.h>
#include <unistd.h>

#define ADXL345_I2C_ADDR 0x53
#define BUTTON_LEFT      28
#define BUTTON_RIGHT     27
#define BUTTON_FLAP 	 29

static char *host = "localhost";
static char *user = "quan";
static char *pass  = "27122001";
static char *dbname = "flight_sim";
unsigned int port = 3306;
static char *unix_socket = NULL;
unsigned int flag = 0;

int fd;
uint8_t left=0, right=0, flap=0;
int adc;
int x,y;
MYSQL* connect;

int selectDevice(int fd, int addr, char *name)
{
   int s;
   char str[128];

    s = ioctl(fd, I2C_SLAVE, addr);

    if (s == -1)
    {
       sprintf(str, "selectDevice for %s", name);
       perror(str);
    }

    return s;
}

int writeToDevice(int fd, int reg, int val)
{
   int s;
   char buf[2];

   buf[0]=reg; buf[1]=val;

   s = write(fd, buf, 2);

   if (s == -1)
   {
      perror("writeToDevice");
   }
   else if (s != 2)
   {
      fprintf(stderr, "short write to device\n");
   }
}

void i2c_connect(){
    unsigned char buf[16];
   sprintf(buf, "/dev/i2c-%d", 1);
   if ((fd = open(buf, O_RDWR)) < 0)
   {
      // Open port for reading and writing

      fprintf(stderr, "Failed to open i2c bus /dev/i2c-1\n");

      exit(1);
   }	
}

void adxl345_init(){
   writeToDevice(fd, 0x2d, 0);  /* POWER_CTL reset */
   writeToDevice(fd, 0x2d, 8);  /* POWER_CTL measure */
   writeToDevice(fd, 0x31, 0);  /* DATA_FORMAT reset */
   writeToDevice(fd, 0x31, 11); /* DATA_FORMAT full res +/- 16g */	
}

void getData(uint8_t buffer[]){
	buffer[0] = 0x01;
	buffer[1] = 0xa0;
	buffer[2] = 0;
	
	wiringPiSPIDataRW(0, buffer, 3);
	
}

void mysql_writeData(MYSQL* conn,int x, int y, int pot, int l, int r, int flap){
	const char* formatstring = "INSERT INTO table1(x_coor, y_coor, pot, le, ri, up) VALUES (%d,%d,%d,%d,%d,%d)";
	char buffer[256];
	if(snprintf(buffer, sizeof(buffer), formatstring, x, y, pot, l, r, flap) >= sizeof(buffer)){
		exit(-1);
	}
	if(mysql_query(conn,buffer)){
		 fprintf(stderr, "%s\n", mysql_error(conn));
		 exit (-1);
	}
}

void adxl_getData(unsigned char buf[], int data[]){
	   short x, y, z;
	   float xa, ya, za;
	   
	   
      buf[0] = 0x32;
   
      if ((write(fd, buf, 1)) != 1)
      {
         // Send the register to read from

         fprintf(stderr, "Error writing to ADXL345\n");
      }
   
      if (read(fd, buf, 6) != 6)
      {
         //  X, Y, Z accelerations

         fprintf(stderr, "Error reading from ADXL345\n");
      }
      else
      {
         x = buf[1]<<8| buf[0];
         y = buf[3]<<8| buf[2];
         z = buf[5]<<8| buf[4];
         xa = (90.0 / 256.0) * (float) x;
         ya = (90.0 / 256.0) * (float) y;
         za = (90.0 / 256.0) * (float) z;

         
         data[0] = (int)xa;
         data[1] = (int)ya;
      }
}

void button_left(){
    left=1;
   // mysql_writeData(connect, x, y, adc, left, right);
    left=0;
    printf("ok\n");
    
}

void button_right(){
    right=1;
    //mysql_writeData(connect, x, y, adc, left, right);
    right=0;
    printf("ok\n");
    
}

int main(){

      int adxl_data[2];
      unsigned char buf[16];
      uint8_t buffer[3];

      wiringPiSetup();
      if(wiringPiSPISetup(0,1000000) < 0) return -1;

      //config button
      pinMode(BUTTON_LEFT, INPUT);
      pinMode(BUTTON_RIGHT, INPUT);
      pinMode(BUTTON_FLAP, INPUT);
     // wiringPiISR(BUTTON_LEFT, INT_EDGE_RISING, &button_left);
     // wiringPiISR(BUTTON_RIGHT, INT_EDGE_RISING, &button_right);
      pullUpDnControl(BUTTON_LEFT, PUD_DOWN);
      pullUpDnControl(BUTTON_RIGHT, PUD_DOWN);
      pullUpDnControl(BUTTON_FLAP, PUD_DOWN);
      
      //init adxl345
      i2c_connect();
      selectDevice(fd, ADXL345_I2C_ADDR, "ADXL345");
      adxl345_init();


      //mysql connect
      connect = mysql_init(NULL);

      int connect_check = mysql_real_connect(connect,host, user,pass, dbname,port,unix_socket, flag);
      if(!connect_check){
	      fprintf(stderr, "\n Error :%s [%d] \n", mysql_error(connect),mysql_errno(connect) );
	      exit(1);
      }
      printf("Connect successfully\n");

      while(1){
	      
	      adxl_getData(buf, adxl_data);
	      x = adxl_data[0];
	      y = adxl_data[1];
	      printf("x:%d, y:%d\n", x,y);
	      
				      
	      getData(buffer);
	      adc =(buffer[2] | ((buffer[1] & 0x0F) << 8));
	      printf("buf1:%d, buf2:%d\n",buffer[1],buffer[2]);
	      printf("adc:%d\n",adc);	
	      
	     mysql_writeData(connect, x, y, adc, left, right,flap);
	       
	      if(digitalRead(BUTTON_LEFT)==1){
		       left=1;
			mysql_writeData(connect, x, y, adc, left, right,flap);
	       }
	       left=0;
	     if(digitalRead(BUTTON_RIGHT)==1){
		  right=1;
		  mysql_writeData(connect, x, y, adc, left, right,flap);
	       }
	       right=0;
	       if(digitalRead(BUTTON_FLAP)==1){
		  flap=1;
		  mysql_writeData(connect, x, y, adc, left, right,flap);
		 delay(200);
	       }
	       flap=0;

	       
	      
	      printf("\n");
	      delay(400);	
      }

		
}

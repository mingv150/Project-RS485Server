#include <modbus.h>

/* compiling : gcc slave-example.c -o slave-example -lmodbus */

void answer_sended(int function, int address, int length)
{
   printf("packet sended : function 0x%x \t address 0x%x \t length %d\n",function,address,length);
}

int main()
{
   int device;
   Mbs_data=(int *)malloc(255*sizeof(int));				/* allocate the modbus database */

   device=Mb_open_device("/dev/ttySAC1",9600,0,8,1);	   /* open device */

   Mb_verbose=1;													/* print debugging informations */

   Mb_slave(device,1,NULL,NULL,answer_sended);			/* start slave thread with slave number=1*/
   getchar();														/* hit <return> to stop program */
   Mb_slave_stop();												/* kill slave thread */
   Mb_close_device(device);									/* close device */
   return 0;
}


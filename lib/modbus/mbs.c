#include <modbus.h>

unsigned char Mbs_slave;				/* slave number */
pthread_t Mbs_thread;					/* slave thread */
int Mbs_device;							/* slave device */

/***********************************************************************************
   	   Mbs_read : read one data and call pointer function if !=NULL
************************************************************************************
input :
-------
unsigned char c : pointer of the char         

no output
***********************************************************************************/
void Mbs_read(unsigned char *c)
{
   read(Mbs_device,c,1);
   if(Mb_ptr_rcv_data!=NULL)
      (*Mb_ptr_rcv_data)(*c);
}

/***********************************************************************************
   	   Mbs_write: write packet and call pointer function if !=NULL
************************************************************************************
input :
-------
trame 	: packet to send
longeur	: length of the packet

no output
***********************************************************************************/
void Mbs_write(unsigned char trame[256], int longeur)
{
   int i;
   for(i=0;i<longeur;i++)
   {
      write(Mbs_device,&trame[i],1);
      if(Mb_ptr_snd_data!=NULL)
         (*Mb_ptr_snd_data)(trame[i]);
   }
}
      

/**********************************************************************************
   	   Mbs : Main slave function 
***********************************************************************************
input not used
no output
**********************************************************************************/
void Mbs(void *ptr)
{
	unsigned char fonction,c,c1,c2,trame[256];
   int adresse,longueur,data,i;
   int data_to_write[255];
	do {
		Mbs_read(&c);
		if (c==Mbs_slave)
		{
         if (Mb_verbose) fprintf(stderr,"Master call me !\n");
			Mbs_read(&fonction);
			if (Mb_verbose) fprintf(stderr,"function 0x%x \n",c);
         trame[0]=Mbs_slave;
         trame[1]=fonction;
			switch (fonction)
   		{
            /*********************************************************************/
      		case 0x03:
      		case 0x04:
         		/* read n byte */
               /* get adress */
               Mbs_read(&c1);
               Mbs_read(&c2);
               adresse=(c1<<8)+c2;
               trame[2]=c1;
               trame[3]=c2;
               if (Mb_verbose) fprintf(stderr,"adress %d (%x %x) \n",adresse,c1,c2);

               /* get length */
               Mbs_read(&c1);
               Mbs_read(&c2);
               longueur=(c1<<8)+c2;
               if (Mb_verbose) fprintf(stderr,"lenght %d \n",longueur);
               trame[4]=c1;
               trame[5]=c2;
               

               /* get crc16 */
               Mbs_read(&c1);
               Mbs_read(&c2);
               trame[6]=c1;
               trame[7]=c2;
               if (Mb_verbose) fprintf(stderr,"crc  %x%x \n",c1,c2);

               /* check crc16 */
               if (Mb_test_crc(trame,6))
               {
                  if (Mb_verbose) fprintf(stderr,"crc error\n");
                  if(Mb_ptr_end_slve!=NULL)
                     (*Mb_ptr_end_slve)(-1,0,0);
                  break;
               }
               if (Mb_verbose) 
                  for(i=0;i<=7;i++)
                     fprintf(stderr,"sended packet[%d] = %0x\n",i,trame[i]);

               /* comput answer packet */
               trame[2]=longueur*2;
               for (i=0;i<longueur;i++)
               {
                  if (Mb_verbose) 
                     fprintf(stderr,"Mbs_data[%d] = %0x\n",adresse+i,Mbs_data[adresse+i]);
                  trame[3+i*2]=Mbs_data[adresse+i]>>8;
                  trame[4+i*2]=Mbs_data[adresse+i]&0xff;
               }

               Mb_calcul_crc(trame,(longueur*2)+3);

               if (Mb_verbose)
               {
                  fprintf(stderr,"answer :\n");
                  for(i=0;i<longueur*2+5;i++)
                     fprintf(stderr,"answer packet[%d] = %0x\n",i,trame[i]);
               }

               Mbs_write(trame,(longueur*2)+5);
               if(Mb_ptr_end_slve!=NULL)
                  (*Mb_ptr_end_slve)(fonction,adresse,longueur);

         		break;
               /*********************************************************************/
      		case 0x06:
         		/* write on byte */
               /* get adress */
               Mbs_read(&c1);
               Mbs_read(&c2);
               adresse=(c1<<8)+c2;
               trame[2]=c1;
               trame[3]=c2;
               if (Mb_verbose) fprintf(stderr,"adress %d (%x %x) \n",adresse,c1,c2);

               /* get data */
               Mbs_read(&c1);
               Mbs_read(&c2);
               data=(c1<<8)+c2;
               if (Mb_verbose) fprintf(stderr,"data %d \n",data);
               trame[4]=c1;
               trame[5]=c2;
               
               /* get crc16 */
               Mbs_read(&c1);
               Mbs_read(&c2);
               trame[6]=c1;
               trame[7]=c2;
               if (Mb_verbose) fprintf(stderr,"crc  %x%x \n",c1,c2);
               /* check crc16 */
               if (Mb_test_crc(trame,6))
               {
                  if (Mb_verbose) fprintf(stderr,"crc error\n");
                  if(Mb_ptr_end_slve!=NULL)
                     (*Mb_ptr_end_slve)(-1,0,0);
                  break;
               }
               if (Mb_verbose) 
                  for(i=0;i<=7;i++)
                     fprintf(stderr,"sended packet[%d] = %0x\n",i,trame[i]);
               
               /* store data */
               Mbs_data[adresse]=data;
               if (Mb_verbose) 
                  fprintf(stderr,"data %d stored at : %d %x\n",data,adresse,adresse);
               
               /* answer trame is the same ;-)*/
               Mbs_write(trame,8);
               if(Mb_ptr_end_slve!=NULL)
                  (*Mb_ptr_end_slve)(fonction,adresse,1);
               
         		break;

            /*********************************************************************/
      		case 0x07:
         		/* read status */
               /* get crc16 */
               Mbs_read(&c1);
               Mbs_read(&c2);
               trame[2]=c1;
               trame[3]=c2;
               if (Mb_verbose) fprintf(stderr,"crc  %x%x \n",c1,c2);
               /* check crc16 */
               if (Mb_test_crc(trame,2))
               {
                  if (Mb_verbose) fprintf(stderr,"crc error\n");
                  if(Mb_ptr_end_slve!=NULL)
                     (*Mb_ptr_end_slve)(-1,0,0);
                  break;
               }
               if (Mb_verbose) 
                  for(i=0;i<=3;i++)
                     fprintf(stderr,"sended packet[%d] = %0x\n",i,trame[i]);

               /* comput answer packet */
               trame[2]=Mb_status;

               Mb_calcul_crc(trame,3);

               if (Mb_verbose)
               {
                  fprintf(stderr,"answer :\n");
                  for(i=0;i<=4;i++)
                     fprintf(stderr,"answer packet[%d] = %0x\n",i,trame[i]);
               }

               Mbs_write(trame,5);
               if(Mb_ptr_end_slve!=NULL)
                  (*Mb_ptr_end_slve)(fonction,0,0);

         		break;
         
            /*********************************************************************/
				case 0x08:
         		/* line test */
               for (i=0;i<4;i++)
               {
                  Mbs_read(&c);
                  if (c!=0) break;
                  trame[i+2]=c;
               }
               if (c!=0) break;

               /* get crc16 */
               Mbs_read(&c1);
               Mbs_read(&c2);
               trame[6]=c1;
               trame[7]=c2;
               if (Mb_verbose) fprintf(stderr,"crc  %x%x \n",c1,c2);
               /* check crc16 */
               if (Mb_test_crc(trame,6))
               {
                  if (Mb_verbose) fprintf(stderr,"crc error\n");
                  if(Mb_ptr_end_slve!=NULL)
                     (*Mb_ptr_end_slve)(-1,0,0);
                  break;
               }
               if (Mb_verbose) 
                  for(i=0;i<=7;i++)
                     fprintf(stderr,"sended packet[%d] = %0x\n",i,trame[i]);

               /* return the same packet, it's cool */
               Mbs_write(trame,8);
               if(Mb_ptr_end_slve!=NULL)
                  (*Mb_ptr_end_slve)(fonction,0,0);
         		break;
         
            /*********************************************************************/
      		case 0x10:
         		/* write n byte  */
               /* get adress */
               Mbs_read(&c1);
               Mbs_read(&c2);
               adresse=(c1<<8)+c2;
               trame[2]=c1;
               trame[3]=c2;
               if (Mb_verbose) fprintf(stderr,"adress %d 0x%x%x \n",adresse,c1,c2);

               /* get length */
               Mbs_read(&c1);
               Mbs_read(&c2);
               longueur=(c1<<8)+c2;
               if (Mb_verbose) fprintf(stderr,"length %d \n",longueur);
               trame[4]=c1;
               trame[5]=c2;
               
               /* read for nothing */
               Mbs_read(&c);
               trame[6]=c;
               
               /* read data to write */
               for (i=0;i<longueur;i++)
               {
                  Mbs_read(&c1);
                  Mbs_read(&c2);
                  data_to_write[i]=(c1<<8)+c2;
                  trame[7+(i*2)]=c1;
                  trame[8+(i*2)]=c2;
               }
               
               /* get crc16 */
               Mbs_read(&c1);
               Mbs_read(&c2);
               trame[7+longueur*2]=c1;
               trame[8+longueur*2]=c2;
               if (Mb_verbose) fprintf(stderr,"crc  %x%x \n",c1,c2);

               /* check crc16 */
               if (Mb_test_crc(trame,7+longueur*2))
               {
                  if (Mb_verbose) fprintf(stderr,"crc error\n");
                  if(Mb_ptr_end_slve!=NULL)
                     (*Mb_ptr_end_slve)(-1,0,0);
                  break;
               }
               if (Mb_verbose) 
                  for(i=0;i<=8+longueur*2;i++)
                     fprintf(stderr,"sended packet[%d] = %0x\n",i,trame[i]);


               /* comput answer packet */
               Mb_calcul_crc(trame,6);

               if (Mb_verbose)
               {
                  fprintf(stderr,"answer :\n");
                  for(i=0;i<8;i++)
                     fprintf(stderr,"answer packet[%d] = %0x\n",i,trame[i]);
               }

               Mbs_write(trame,8);
               
               /* copy data to modbus data*/
               for(i=0;i<longueur;i++)
               {
                  Mbs_data[adresse+i]=data_to_write[i];
                  if (Mb_verbose)
                     fprintf(stderr,"data[%x] = %x\n",adresse+i,data_to_write[i]);
               }
               if(Mb_ptr_end_slve!=NULL)
                  (*Mb_ptr_end_slve)(fonction,adresse,longueur);
         		break;
      		default:
         		break;
         }
   	}
	} while (1);
}


/***********************************************************************************
   	   Mb_slave : start slave thread
************************************************************************************
input :
-------
mbsdevice  : device to use
msslave    : slave number
ptrfoncsnd : function to call when slave send data (can be NULL if you don't whant to use it)
ptrfoncrcv : function to call when slave receive data (can be NULL if you don't whant to use it)
ptrfoncend : function to call when slave finish to send answer (can be NULL if you don't whant to use it)

no output
***********************************************************************************/
void Mb_slave(int mbsdevice,int mbsslave, void *ptrfoncsnd, void *ptrfoncrcv, void *ptrfoncend)
{
   Mbs_device=mbsdevice;
	Mbs_slave=mbsslave;
   Mb_ptr_snd_data=ptrfoncsnd;
   Mb_ptr_rcv_data=ptrfoncrcv;
   Mb_ptr_end_slve=ptrfoncend;
   pthread_create(&Mbs_thread, NULL,(void*)&Mbs,NULL);
}


/***********************************************************************************
   	   Mb_slave_stop : stop slave thread
************************************************************************************
no input
no output
***********************************************************************************/
void Mb_slave_stop()
{
   pthread_cancel(Mbs_thread);
}

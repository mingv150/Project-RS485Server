#include <modbus.h>
#include <signal.h>
#include <ctype.h>

int Mb_device;              /* device tu use */
int Mbm_Pid_Child;      /* PID child used to read the slave answer */
int Mbm_Pid_Sleep;      /* PID use to wait the end of the timeout */
byte *Mbm_result;           /* byte readed on the serial port : answer of the slave */


/************************************************************************************
        Mbd_get_data : thread reading data on the serial port
*************************************************************************************
input :
-------
len :   number of data to read;

no output
************************************************************************************/
void Mbd_get_data(int *len )
{
   int i;
   byte read_data;

   Mbm_Pid_Child=getpid();


   if (Mb_verbose)
      fprintf(stderr,"starting receiving data, total length : %d \n",*len);

   do {
      read(Mb_device,&read_data,1);
   } while (iscntrl(read_data));



   for(i=0;i<(*len);i++)
   {
      if(iscntrl(read_data))
         break;

      /* store data to the slave answer packet */
      Mbm_result[i]=toupper(read_data);
      
      /* call the pointer function if exist */
      if(Mb_ptr_rcv_data!=NULL)
         (*Mb_ptr_rcv_data)(read_data);
      if (Mb_verbose)
         fprintf(stderr,"receiving byte :0x%x %d (%d)\n",read_data,read_data,Mbm_result[i]);

      /* read data */
      read(Mb_device,&read_data,1);
   }

   Mbm_result[i]='\0';

   if (Mb_verbose)
      fprintf(stderr,"receiving data done\n");

   Mbm_Pid_Child=0;

}


/************************************************************************************
        Mbd_import : read data on the serial port
*************************************************************************************
input :
-------
port    :   port to read
string  :   string to store data
len :   max lenght to read
timeout :   timeout

output

0   : success
-1  : timeout

************************************************************************************/
int Mbd_import(int port, char *string, int len, int timeout)
{

   int i;
   byte read_data;

   time_t t;
                                                                                                                              
   t = (time(NULL) + timeout);

   if (Mb_verbose)
      fprintf(stderr,"starting receiving data, total length : %d \n",len);

   do {
      while(read(Mb_device,&read_data,1) == 0){
         if(t < time(NULL))
            return(-1);
//         printf("t =  %d \t clock = %d\n", t, time(NULL));
      }

   } while (iscntrl(read_data));



   for(i=0;i<(len);i++)
   {
      if(iscntrl(read_data))
         break;

      if(t < time(NULL))
         return(-1);
// printf("t =  %d \t clock = %d\n", t, time(NULL));

      /* store data to the slave answer packet */
      string[i]=toupper(read_data);
      
      if (Mb_verbose)
         fprintf(stderr,"receiving byte :0x%x %d (%d)\n",read_data,read_data,string[i]);

      /* read data */
      while(read(Mb_device,&read_data,1) == 0){
         if(t < time(NULL))
            return(-1);
//         printf("t =  %d \t clock = %d\n", t, time(NULL));
      }
   }

   string[i]='\0';

   if (Mb_verbose)
      fprintf(stderr,"receiving data done\n");

   return(0);
}




/************************************************************************************
        Mbd_matches : match data on the serial port
*************************************************************************************
input :
-------
port        :   port to read
matString   :   string to match
matTime     :   timeout

output

0   : success
-1  : timeout

************************************************************************************/
int Mbd_matchs (int port, char *matString, int matTime){
                                                                                                                              
   char modemRes[64];
   int result;
   Mb_tio.c_cc[VMIN]=0;
   Mb_tio.c_cc[VTIME]=1;

   if (tcsetattr(port,TCSANOW,&Mb_tio) <0) {
      perror("Can't set terminal parameters ");
      return -1 ;
   }
   result = -1;
                                                                                                                              
   while(Mbd_import(port, modemRes, 64, matTime) == 0){
      if (Mb_verbose)
         fprintf(stderr, "%s\n", modemRes);
      if(strstr(modemRes, matString)){
         result = 0;
         break;
      }
   }


   Mb_tio.c_cc[VMIN]=1;
   Mb_tio.c_cc[VTIME]=0;

   if (tcsetattr(port,TCSANOW,&Mb_tio) <0) {
      perror("Can't set terminal parameters ");
      return -1 ;
   }
   return(result);
}

/************************************************************************************
        Mbd_sleep : thread wait timeout
*************************************************************************************
input :
-------
timeout : duduration of the timeout in ms

no output
************************************************************************************/
void Mbd_sleep(int *timeout)
{
   Mbm_Pid_Sleep=getpid();
    if (Mb_verbose)
      fprintf(stderr,"\n sleeping %d ms\n",*timeout);

   usleep(*timeout*1000);

   if (Mb_verbose)
      fprintf(stderr,"\n !!!!! Done Sleeping %d ms !!!!!!\n",*timeout);

   Mbm_Pid_Sleep=0;

}


/************************************************************************************
        Mbd_export : send data
*************************************************************************************
input :
-------
port      : port to use
trame     : packet to send
timeout   : duduration of the timeout in ms
long_emit : length of the packet to send

answer :
--------
0         : write ok
-1        : failure
************************************************************************************/
int Mbd_export(int port, byte trame[], int timeout, int long_emit)
{
   int i;

   /* clean port */
   tcflush(port, TCIFLUSH);

   if (Mb_verbose)
      fprintf(stderr,"start writing \n");

   for(i=0;i<long_emit;i++)
   {
      /* send data */
      if(write(port,&trame[i],1) != 1)
         return(-1);
   }

  if (Mb_verbose)
      fprintf(stderr,"write ok\n");
  return(0);
}





/************************************************************************************
        Mbd_send_and_get_result : send data, and wait the answer of the modem
*************************************************************************************
input :
-------
trame     : packet to send
timeout   : duduration of the timeout in ms
long_emit : length of the packet to send
longueur  : length of the packet to read

answer :
--------
0         : timeout failure
1         : answer ok
************************************************************************************/
int Mbd_send_and_get_result(byte trame[], int timeout, int long_emit, int longueur)
{
   int i,stat1=-1,stat2=-1;

   pthread_t thread1,thread2;

   Mbm_result = (unsigned char *) malloc(longueur*sizeof(unsigned char));

   /* clean port */
   tcflush(Mb_device, TCIFLUSH);

   /* create 2 threads for read data and to wait end of timeout*/
   pthread_create(&thread2, NULL,(void*)&Mbd_sleep,&timeout);
   pthread_create(&thread1, NULL,(void*)&Mbd_get_data,&longueur);

   if (Mb_verbose)
      fprintf(stderr,"start writing \n");
   for(i=0;i<long_emit;i++)
   {
      /* send data */
      write(Mb_device,&trame[i],1);
      /* call pointer function if exist */
      if(Mb_ptr_snd_data!=NULL)
         (*Mb_ptr_snd_data)(trame[i]);
   }

  if (Mb_verbose)
      fprintf(stderr,"write ok\n");

   do {
      if (Mbm_Pid_Child!=0)
         /* kill return 0 if the pid is running or -1 if the pid don't exist */
         stat1=0;
      else
         stat1=-1;

      if (Mbm_Pid_Sleep!=0)
         stat2=0;
      else
         stat2=-1;

//printf("PID_Child = %d \tPID_Sleep = %d \tstat1 = %d \tstat2 = %d\n", Mbm_Pid_Child, Mbm_Pid_Sleep, stat1, stat2);

      /* answer of the slave terminate or and of the timeout */
      if (stat1==-1 || stat2==-1) 
         break;
      usleep(timeout);

   } while(1); 
   if (Mb_verbose)
   {
      fprintf(stderr,"pid reading %d return %d\n",Mbm_Pid_Child,stat1);
      fprintf(stderr,"pid timeout %d return %d\n",Mbm_Pid_Sleep,stat2);
   }

   /* stop both childs */
   Mbm_Pid_Child=0;
   Mbm_Pid_Sleep=0;
   pthread_cancel(thread1);
   pthread_cancel(thread2);
   /* error : timeout fealure */
   if (stat1==0)
   {
      return 0;
   }
   /* ok : store the answer packet in the data trame */
   for (i=0;i<=longueur;i++)
      trame[i]=Mbm_result[i];

   return 1;
}
      


/************************************************************************************
                Mbd_dial : Dial a phone number and wait for connect
*************************************************************************************
input :
-------
phonenumber   : Phone Number of the slave
timeout           : timeout duration in ms
*************************************************************************************
answer :
--------
0 : OK Connected
-1 : No DialTone
-2 : Busy
-3 : No Answer
-4 : No Carrier
-5 : Timeout
*************************************************************************************/
int Mbd_dial(int device, char *phonenumber, int timeout)
{
   byte trame[256];

   Mb_ptr_snd_data=0;
   Mb_ptr_rcv_data=0;

   Mb_device = device;

   sprintf(trame, "ATDT %s\r", phonenumber);

   Mbd_send_and_get_result(trame, timeout, strlen(trame), 256);

   if (Mb_verbose)
      fprintf(stderr,"trame = %s\n",trame);

   if(strstr(trame, "NO DIALTONE"))
      return(-1);

   if(strstr(trame, "BUSY"))
      return(-2);
   if(strstr(trame, "NO ANSWER"))
      return(-3);
   if(strstr(trame, "NO CARRIER"))
      return(-4);
   if(strstr(trame, "CONNECT"))
      return(0);
   return(-5);
}


/************************************************************************************
            Mbd_setup : send the modem a setup string and get response
*************************************************************************************
input :
-------
setup         : Modem Setup string
timeout           : timeout duration in ms
*************************************************************************************
answer :
--------
0 : OK setup
-1 : Timeout
-2 : Error
*************************************************************************************/
int Mbd_setup(int device, char *setup, int timeout)
{
   byte trame[256];

   Mb_ptr_snd_data=0;
   Mb_ptr_rcv_data=0;

   Mb_device = device;

   sprintf(trame, "%s\r", setup);

   if(Mbd_export(device, trame, timeout, strlen(trame)) < 0)
      return(-2);
   

   return(Mbd_matchs (device, "OK", timeout));

}

/************************************************************************************
            Mbd_hangup : hangup the modem
*************************************************************************************
input :
-------
device        : device
timeout           : timeout duration in ms
*************************************************************************************
answer :
--------
0 : OK hangup
-1 : Timeout
-2 : Error
*************************************************************************************/
int Mbd_hangup(int device, int timeout)
{
   byte trame[256];

   Mb_ptr_snd_data=0;
   Mb_ptr_rcv_data=0;

   Mb_device = device;

   sprintf(trame, "+++");

   if(Mbd_export(device, trame, timeout, strlen(trame)) < 0)
      return(-2);
   Mbd_matchs (device, "OK", timeout);

   sprintf(trame, "ath\r");

   if(Mbd_export(device, trame, timeout, strlen(trame)) < 0)
      return(-2);
   

   return(Mbd_matchs (device, "OK", timeout));

}

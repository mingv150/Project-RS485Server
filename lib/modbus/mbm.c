#include <modbus.h>
#include <signal.h>

int Mb_device;				/* device tu use */
int Mbm_Pid_Child;		/* PID child used to read the slave answer */
int Mbm_Pid_Sleep;		/* PID use to wait the end of the timeout */
byte *Mbm_result;			/* byte readed on the serial port : answer of the slave */


/************************************************************************************
		Mbm_get_data : thread reading data on the serial port
*************************************************************************************
input :
-------
len	:	number of data to read;

no output
************************************************************************************/
void Mbm_get_data(int *len )
{
	int i;
	byte read_data;

	Mbm_Pid_Child=getpid();


		if (Mb_verbose)
			fprintf(stderr,"starting receiving data, total length : %d \n",*len);
	for(i=0;i<(*len);i++)
	{
		/* read data */
		read(Mb_device,&read_data,1);

		/* store data to the slave answer packet */
		Mbm_result[i]=read_data;
		
		/* call the pointer function if exist */
		if(Mb_ptr_rcv_data!=NULL)
			(*Mb_ptr_rcv_data)(read_data);
		if (Mb_verbose)
			fprintf(stderr,"receiving byte :0x%x %d (%d)\n",read_data,read_data,Mbm_result[i]);
	}
	if (Mb_verbose)
		fprintf(stderr,"receiving data done\n");

	Mbm_Pid_Child=0;

}

int Csm_get_data(int len, int timeout)
{
	int i;
	byte read_data;
	time_t t;

	if (Mb_verbose)
		fprintf(stderr,"in get data\n");
	
	t = (time(NULL) + ((timeout * 2)/1000));

	for(i=0;i<(len);i++)
	{
		if(t < time(NULL))
			return(0);

		/* read data */
		while(read(Mb_device,&read_data,1) == 0){
			if(t < time(NULL))
				return(0);
		}
		/* store data to the slave answer packet */
		Mbm_result[i]=read_data;
		
		if (Mb_verbose)
			fprintf(stderr,"receiving byte :0x%x %d (%d)\n",read_data,read_data,Mbm_result[i]);
	  
	}
	if (Mb_verbose)
		fprintf(stderr,"receiving data done\n");
	return(1);
}


/************************************************************************************
		Mbm_sleep : thread wait timeout
*************************************************************************************
input :
-------
timeout : duduration of the timeout in ms

no output
************************************************************************************/
void Mbm_sleep(int *timeout)
{
	Mbm_Pid_Sleep=getpid();
	if (Mb_verbose)
		fprintf(stderr,"sleeping %d ms\n",*timeout);

	usleep(*timeout*1000);

	Mbm_Pid_Sleep=0;
	if (Mb_verbose)
		fprintf(stderr,"Done sleeping %d ms\n",*timeout);

}

/************************************************************************************
		Mbm_send_and_get_result : send data, and wait the answer of the slave
*************************************************************************************
input :
-------
trame	  : packet to send
timeout	: duduration of the timeout in ms
long_emit : length of the packet to send
longueur  : length of the packet to read

answer :
--------
0			: timeout failure
1			: answer ok
************************************************************************************/
int Mbm_send_and_get_result(byte trame[], int timeout, int long_emit, int longueur)
{
	int i,stat1=-1,stat2=-1;

	pthread_t thread1,thread2;
	Mbm_result = (unsigned char *) malloc(longueur*sizeof(unsigned char));

	/* clean port */
	tcflush(Mb_device, TCIFLUSH);

	/* create 2 threads for read data and to wait end of timeout*/
	pthread_create(&thread2, NULL,(void*)&Mbm_sleep,&timeout);
	pthread_detach(thread2);
	pthread_create(&thread1, NULL,(void*)&Mbm_get_data,&longueur);
	pthread_detach(thread1);

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
		free(Mbm_result);
		return 0;
	}
	/* ok : store the answer packet in the data trame */
	for (i=0;i<=longueur;i++)
		trame[i]=Mbm_result[i];
	
	free(Mbm_result);
	return 1;
}
		
int Csm_send_and_get_result(unsigned char trame[], int timeout, int long_emit, int longueur)
{
	int i;
	int ret;

	Mbm_result = trame;
	
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
	Mb_tio.c_cc[VMIN]=0;
	Mb_tio.c_cc[VTIME]=1;

	if (tcsetattr(Mb_device,TCSANOW,&Mb_tio) <0) {
		perror("Can't set terminal parameters ");
		return 0;
	}
  
	ret = Csm_get_data(longueur, timeout);

	Mb_tio.c_cc[VMIN]=1;
	Mb_tio.c_cc[VTIME]=0;

	if (tcsetattr(Mb_device,TCSANOW,&Mb_tio) <0) {
		perror("Can't set terminal parameters ");
		return 0 ;
	}
	
	return ret;
}


/************************************************************************************
					Mbm_master : comput and send a master packet
*************************************************************************************
input :
-------
Mb_trame	  : struct describing the packet to comput
						device		: device descriptor
						slave 		: slave number to call
						function 	: modbus function
						address		: address of the slave to read or write
						length		: lenght of data to send
data_in	  : data to send to the slave
data_out	  : data to read from the slave
timeout	  : timeout duration in ms
ptrfoncsnd : function to call when master send data (can be NULL if you don't whant to use it)
ptrfoncrcv : function to call when master receive data (can be NULL if you don't whant to use it)
*************************************************************************************
answer :
--------
0 : OK
-1 : unknow modbus function
-2 : CRC error in the slave answer
-3 : timeout error
-4 : answer come from an other slave
*************************************************************************************/
int Mb_master(Mbm_trame Mbtrame,int data_in[], int data_out[],void *ptrfoncsnd, void *ptrfoncrcv)
{
	int i,longueur,long_emit;
	int slave, function, adresse, nbre;
	byte trame[256];

	Mb_device=Mbtrame.device;
	slave=Mbtrame.slave;
	function=Mbtrame.function;
	adresse=Mbtrame.address;
	nbre=Mbtrame.length;
	Mb_ptr_snd_data=ptrfoncsnd;
	Mb_ptr_rcv_data=ptrfoncrcv;
		
	switch (function)
	{
		case 0x03:
		case 0x04:
			/* read n byte */
			trame[0]=slave;
			trame[1]=function;
			trame[2]=adresse>>8;
			trame[3]=adresse&0xFF;
			trame[4]=nbre>>8;
			trame[5]=nbre&0xFF;
			/* comput crc */
			Mb_calcul_crc(trame,6);
			/* comput length of the packet to send */
			long_emit=8;
			break;
		
		case 0x05: //write a single coil
			trame[0]=slave;
			trame[1]=function;
			trame[2]=adresse>>8;
			trame[3]=adresse&0xFF;
			trame[4]=data_in[0]>>8;
			trame[5]=data_in[0]&0xFF;
			/* comput crc */
			Mb_calcul_crc(trame,6);
			/* comput length of the packet to send */
			long_emit=8;
			break;
			
		case 0x06:
			/* write one byte */
			trame[0]=slave;
			trame[1]=function;
			trame[2]=adresse>>8;
			trame[3]=adresse&0xFF;
			trame[4]=data_in[0]>>8;
			trame[5]=data_in[0]&0xFF;
			/* comput crc */
			Mb_calcul_crc(trame,6);
			/* comput length of the packet to send */
			long_emit=8;
			break;

		case 0x07:
			/* read status */
			trame[0]=slave;
			trame[1]=function;
			/* comput crc */
			Mb_calcul_crc(trame,2);
			/* comput length of the packet to send */
			long_emit=4;
			break;
			
		case 0x08:
			/* line test */
			trame[0]=slave;
			trame[1]=0x08;
			trame[2]=0;
			trame[3]=0;
			trame[4]=0;
			trame[5]=0;
			Mb_calcul_crc(trame,6);
			/* comput length of the packet to send */
			long_emit=8;
			break;
			
		case 0x10:
			/* write n byte  */
			trame[0]=slave;
			trame[1]=0x10;
			trame[2]=adresse>>8;
			trame[3]=adresse&0xFF;
			trame[4]=nbre>>8;
			trame[5]=nbre&0xFF;
			trame[6]=nbre*2;
			for (i=0;i<nbre;i++)
			{
				trame[7+i*2]=data_in[i]>>8;
				trame[8+i*2]=data_in[i]&0xFF;
			}
			/* comput crc */
			Mb_calcul_crc(trame,7+nbre*2);
			/* comput length of the packet to send */
			long_emit=(nbre*2)+9;
			break;
		default:
			return -1;
	}
	if (Mb_verbose) 
	{
		fprintf(stderr,"send packet length %d\n",long_emit);
		for(i=0;i<long_emit;i++)
			fprintf(stderr,"send packet[%d] = %0x\n",i,trame[i]);
	}
	
	/* comput length of the slave answer */
	switch (function)
	{
		case 0x03:
		case 0x04:
			longueur=5+(nbre*2);
			break;
		
		case 0x05:
		case 0x06:
		case 0x08:
		case 0x10:
		longueur=8;
			break;

		case 0x07:
		longueur=5;
			break;

		default:
			return -1;
			break;
	}

	/* send packet & read answer of the slave
		answer is stored in trame[] */


	for(i = 0;i < 4; i++){
		if(Csm_send_and_get_result(trame,Mbtrame.timeout,long_emit,longueur)){
			i = 1;
			break;
		}
	}
	if(i != 1) 
		return -3;	/* timeout error */




  	if (Mb_verbose)
	{
		fprintf(stderr,"answer :\n");
		for(i=0;i<longueur;i++)
			fprintf(stderr,"answer packet[%d] = %0x\n",i,trame[i]);
	}
	
	if (trame[0]!=slave)
		return -4;	/* this is not the right slave */

	switch (function)
	{
		case 0x03:
		case 0x04:
			/* test received data */
			if (trame[1]!=0x03 && trame[1]!=0x04)
				return -2;
			if (Mb_test_crc(trame,3+(nbre*2)))
				return -2;
			/* data are ok */
			if (Mb_verbose)
				fprintf(stderr,"Reader data \n");
			for (i=0;i<nbre;i++)
			{
				data_out[i]=(trame[3+(i*2)]<<8)+trame[4+i*2];
				if (Mb_verbose)
					fprintf(stderr,"data %d = %0x\n",i,data_out[i]);
			}
			break;
			
		case 0x05: //write a single coil
			/* test received data */
			if (trame[1]!=0x05)
				return -2;
			if (Mb_test_crc(trame,6))
				return -2;
			/* data are ok */
			if (Mb_verbose)
				fprintf(stderr,"data stored succesfull !\n");
			break;
		
		case 0x06:
			/* test received data */
			if (trame[1]!=0x06)
				return -2;
			if (Mb_test_crc(trame,6))
				return -2;
			/* data are ok */
			if (Mb_verbose)
				fprintf(stderr,"data stored succesfull !\n");
			break;

		case 0x07:
			/* test received data */
			if (trame[1]!=0x07)
				return -2;
			if (Mb_test_crc(trame,3))
				return -2;
			/* data are ok */
			data_out[0]=trame[2];	/* store status in data_out[0] */
			if (Mb_verbose)
				fprintf(stderr,"data  = %0x\n",data_out[0]);
			break;

		case 0x08:
			/* test received data */
			if (trame[1]!=0x08)
				return -2;
			if (Mb_test_crc(trame,6))
				return -2;
			/* data are ok */
			if (Mb_verbose)
				fprintf(stderr,"Loopback test ok \n");
			break;

		case 0x10:
			/* test received data */
			if (trame[1]!=0x10)
				return -2;
			if (Mb_test_crc(trame,6))
				return -2;
			/* data are ok */
			if (Mb_verbose)
				fprintf(stderr,"%d setpoint stored succesfull\n",(trame[4]<<8)+trame[5]);
			break;

		default:
			return -1;
			break;
	}
	return 0;
}

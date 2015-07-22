#include <modbus.h>

/* compiling : gcc master-example.c -o master-example -lmodbus */

int main()
{
    int device;
    Mbm_trame trame;
    int result;
    int data_in[256];
    int data_out[256];
    char data[256];

    /* open device */
    device=Mb_open_device("/dev/ttySAC2",9600,0,8,1);

    /* print debugging informations */
    Mb_verbose=1;
    
    /* setup master packet */
    trame.device=device;
    trame.function=16;           /* send data */
    trame.slave=1;                   /* slave number*/ 
//   trame.address=0xa24 -1;
    trame.address=2596;
    trame.length=4;
    trame.timeout=10000;
    
    /* setup data to send */
    data_in[0]=0;
    data_in[1]=0;
/*   data_in[2]=3;
    data_in[3]=4;
    data_in[4]=5;
    data_in[5]=5;
    data_in[6]=5;
    data_in[7]=5;
    data_in[8]=5;
*/

    result=Mb_master(trame,data_in,data_out,NULL,NULL);
    /* return 0 if ok */
    if (result<0)
    {
        if (result==-1) printf("error : unknow function\n");
        if (result==-2) printf("crc error\n");
        if (result==-3) printf("timeout error\n");
        if (result==-4) printf("error : bad slave answer\n");
    }
    else
        printf("ok\n");

    trame.function=3;            /* send data */
    trame.slave=1;                   /* slave number*/ 
//   trame.address=0xa28;
    trame.address=1043;
    trame.length=11;
    trame.timeout=10000;


    result=Mb_master(trame,data_in,data_out,NULL,NULL);
    /* return 0 if ok */
    if (result<0)
    {
        if (result==-1) printf("error : unknow function\n");
        if (result==-2) printf("crc error\n");
        if (result==-3) printf("timeout error\n");
        if (result==-4) printf("error : bad slave answer\n");
    }
    else
        printf("ok\n");

    data[0] = (char) ((data_out[0] & 0xff00) >> 8);
    data[1] = (char) (data_out[0] & 0x00ff);
    data[2] = (char) ((data_out[1] & 0xff00) >> 8);
    data[3] = (char) (data_out[1] & 0x00ff);
    data[4] = (char) ((data_out[2] & 0xff00) >> 8);
    data[5] = (char) (data_out[2] & 0x00ff);
    data[6] = (char) ((data_out[3] & 0xff00) >> 8);
    data[7] = (char) (data_out[3] & 0x00ff);
    data[8] = (char) ((data_out[4] & 0xff00) >> 8);
    data[9] = (char) (data_out[4] & 0x00ff);
    data[10] = (char) ((data_out[5] & 0xff00) >> 8);
    data[11] = (char) (data_out[5] & 0x00ff);
    data[12] = (char) ((data_out[6] & 0xff00) >> 8);
    data[13] = (char) (data_out[6] & 0x00ff);
    data[14] = (char) ((data_out[7] & 0xff00) >> 8);
    data[15] = (char) (data_out[7] & 0x00ff);
    data[16] = (char) ((data_out[8] & 0xff00) >> 8);
    data[17] = (char) (data_out[8] & 0x00ff);
    data[18] = (char) ((data_out[9] & 0xff00) >> 8);
    data[19] = (char) (data_out[9] & 0x00ff);
    data[20] = (char) ((data_out[10] & 0xff00) >> 8);
    data[21] = (char) (data_out[10] & 0x00ff);
    data[22] = (char) ((data_out[11] & 0xff00) >> 8);
    data[23] = '\0';

    printf("string = .%s.\n", data);

    Mb_verbose=1;
    trame.function=3;                    /* send data */
    trame.slave=1;                                       /* slave number*/
    trame.address=2584;
    trame.length=1;
    trame.timeout=10000;
                                                                                                                                                
                                                                                                                                                
    result=Mb_master(trame,data_in,data_out,NULL,NULL);
    /* return 0 if ok */
    if (result<0)
    {
        if (result==-1) printf("error : unknow function\n");
        if (result==-2) printf("crc error\n");
        if (result==-3) printf("timeout error\n");
        if (result==-4) printf("error : bad slave answer\n");
    }
    else
        printf("ok\n");
    Mb_verbose=0;



    trame.function=3;            /* send data */
    trame.slave=1;                   /* slave number*/ 
    trame.address=2600;
    trame.length=21;
    trame.timeout=10000;


    result=Mb_master(trame,data_in,data_out,NULL,NULL);
    /* return 0 if ok */
    if (result<0)
    {
        if (result==-1) printf("error : unknow function\n");
        if (result==-2) printf("crc error\n");
        if (result==-3) printf("timeout error\n");
        if (result==-4) printf("error : bad slave answer\n");
    }
    else
        printf("ok\n");


    data[0] = (char) ((data_out[2] & 0xff00) >> 8);
    data[1] = (char) (data_out[2] & 0x00ff);
    data[2] = (char) ((data_out[3] & 0xff00) >> 8);
    data[3] = (char) (data_out[3] & 0x00ff);
    data[4] = (char) ((data_out[4] & 0xff00) >> 8);
    data[5] = (char) (data_out[4] & 0x00ff);
    data[6] = (char) ((data_out[5] & 0xff00) >> 8);
    data[7] = (char) (data_out[5] & 0x00ff);
    data[8] = (char) ((data_out[6] & 0xff00) >> 8);
    data[9] = (char) (data_out[6] & 0x00ff);
    data[10] = (char) ((data_out[7] & 0xff00) >> 8);
    data[11] = (char) (data_out[7] & 0x00ff);
    data[12] = (char) ((data_out[8] & 0xff00) >> 8);
    data[13] = (char) (data_out[8] & 0x00ff);
    data[14] = (char) ((data_out[9] & 0xff00) >> 8);
    data[15] = (char) (data_out[9] & 0x00ff);
    data[16] = (char) ((data_out[10] & 0xff00) >> 8);
    data[17] = (char) (data_out[10] & 0x00ff);
    data[18] = (char) ((data_out[11] & 0xff00) >> 8);
    data[19] = (char) (data_out[11] & 0x00ff);
    data[20] = '\0';

    printf("string = .%s.\n", data);


    Mb_close_device(device);

    return 0;
}


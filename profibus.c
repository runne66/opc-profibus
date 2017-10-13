#include <stdio.h>  
#include <string.h>  
#include <stdlib.h>  
#include <signal.h>  
#include <fcntl.h>  
#include <unistd.h>
#include <sys/socket.h>  
#include <termios.h> //set baud rate  
#include <netinet/in.h>
#include <sys/select.h>  
#include <sys/time.h>  
#include <sys/types.h>  
#include <errno.h>  
#include <sys/stat.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <pthread.h>

#include "open62541.h"  
  
//#define rec_buf_wait_2s 2  
#define buffLen 1024  
#define rcvTimeOut 2  

unsigned char send_buff[512] = {0x03,0x07,0x44,0x00,0x00,0x00,0x84,0xc7};
unsigned char recv_buff[512]; 
unsigned char ask_buff[512] = {0x03,0x04,0xff,0xff,0xff,0xff,0xde,0x29};
unsigned char res_buff[10];
#define OPCUA_SERVER_PORT 16664
UA_Server *server;

typedef struct _DATA_SOURCE{
    char* name;
    float data;
}DATA_SOURCE;

DATA_SOURCE source[]=
{
    {"DP_1",0.0},
    {"DP_2",0.0},
    {"PA_1",0.0},
    {"PA_2",0.0}
};


UA_Boolean running = true;
static void stopHandler(int sig) {
    running = false;
}

/*通过名字读数据*/
void* nodeIdFindData(const UA_NodeId nodeId)
{
    int i;
    for(i=0;i<sizeof(source)/sizeof(DATA_SOURCE);i++) {
        if(strncmp((char*)nodeId.identifier.string.data, source[i].name, strlen(source[i].name)) == 0) 
            return &source[i].data;
        }           
    printf("not find:%s!\n",nodeId.identifier.string.data);
    return NULL;
}

static UA_StatusCode
readFloatDataSource(void *handle, const UA_NodeId nodeId, UA_Boolean sourceTimeStamp,
                const UA_NumericRange *range, UA_DataValue *value) {
    if(range) {
        value->hasStatus = true;
        value->status = UA_STATUSCODE_BADINDEXRANGEINVALID;
        return UA_STATUSCODE_GOOD;
    }       
    UA_Float currentFloat;

    if(nodeIdFindData(nodeId) != NULL)
        currentFloat = *(UA_Float*)nodeIdFindData(nodeId);
    else 
        currentFloat = -1;
    value->sourceTimestamp = UA_DateTime_now();
    value->hasSourceTimestamp = true;
    UA_Variant_setScalarCopy(&value->value, &currentFloat, &UA_TYPES[UA_TYPES_FLOAT]);
    value->hasValue = true;
    return UA_STATUSCODE_GOOD;
}

void add_dataSource_to_opcServer()
{
    int i;
    for(i=0;i<sizeof(source)/sizeof(DATA_SOURCE);i++) {
            UA_DataSource dateDataSource = (UA_DataSource) {.handle = NULL, .read = readFloatDataSource, 
                .write = NULL};
            UA_VariableAttributes *attr_float = UA_VariableAttributes_new();
            UA_VariableAttributes_init(attr_float);
            
            attr_float->description = UA_LOCALIZEDTEXT("en_US",source[i].name);
            attr_float->displayName = UA_LOCALIZEDTEXT("en_US",source[i].name);
            attr_float->accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
            UA_NodeId myIntegerNodeId = UA_NODEID_STRING(1, source[i].name);
            UA_QualifiedName myIntegerName = UA_QUALIFIEDNAME(1, source[i].name);
            UA_NodeId parentNodeId = UA_NODEID_STRING(1, "profibus");
            UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
            UA_Server_addDataSourceVariableNode(server, myIntegerNodeId,parentNodeId,
                                                    parentReferenceNodeId, myIntegerName,
                                                UA_NODEID_NULL, *attr_float, dateDataSource, NULL);     
    }   
}

void handle_opcua_server(){

    UA_ServerConfig config = UA_ServerConfig_standard;
    UA_ServerNetworkLayer nl = UA_ServerNetworkLayerTCP(UA_ConnectionConfig_standard, OPCUA_SERVER_PORT);
    config.networkLayers = &nl;
    config.networkLayersSize = 1;
    server = UA_Server_new(config);

    UA_ObjectAttributes object_attr;
    UA_ObjectAttributes_init(&object_attr);
    object_attr.description = UA_LOCALIZEDTEXT("en_US","profibus");
    object_attr.displayName = UA_LOCALIZEDTEXT("en_US","profibus");
    UA_Server_addObjectNode(server, UA_NODEID_STRING(1,"profibus"),
    UA_NODEID_NUMERIC(0, 2253),
    UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), UA_QUALIFIEDNAME(1, "profibus"),
    UA_NODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE), object_attr, NULL, NULL);


    add_dataSource_to_opcServer();
    
    UA_Server_run(server, &running);
        
    UA_Server_delete(server);
    nl.deleteMembers(&nl);  
}


/*************Linux and Serial Port *********************/  
int openPort(int fd, int comport)  
{  
  
    if (comport == 1)  
    {  
        fd = open("/dev/ttyS0", O_RDWR | O_NOCTTY | O_NDELAY);  
        if (-1 == fd)  
        {  
            perror("Can't Open Serial Port");  
            return(-1);  
        }  
        else  
        {  
            printf("open ttyS0 .....\n");  
        }  
    }  
    else if (comport == 2)  
    {  
        fd = open("/dev/ttyS1", O_RDWR | O_NOCTTY | O_NDELAY);  
        if (-1 == fd)  
        {  
            perror("Can't Open Serial Port");  
            return(-1);  
        }  
        else  
        {  
            printf("open ttyS1 .....\n");  
        }  
    }  
    else if (comport == 3)  
    {  
        fd = open("/dev/ttyS2", O_RDWR | O_NOCTTY | O_NDELAY);  
        if (-1 == fd)  
        {  
            perror("Can't Open Serial Port");  
            return(-1);  
        }  
        else  
        {  
            printf("open ttyS2 .....\n");  
        }  
    }  
    /*************************************************/  
    else if (comport == 4)  
    {  
        fd = open("/dev/ttyUSB0", O_RDWR | O_NOCTTY | O_NDELAY);  
        if (-1 == fd)  
        {  
            perror("Can't Open Serial Port");  
            return(-1);  
        }  
        else  
        {  
            printf("open ttyUSB0 .....\n");  
        }  
    }  
  
    if (fcntl(fd, F_SETFL, 0)<0)  
    {  
        printf("fcntl failed!\n");  
    }  
    else  
    {  
        printf("fcntl=%d\n", fcntl(fd, F_SETFL, 0));  
    }  
    if (isatty(STDIN_FILENO) == 0)  
    {  
        printf("standard input is not a terminal device\n");  
    }  
    else  
    {  
        printf("is a tty success!\n");  
    }  
    printf("fd-open=%d\n", fd);  
    return fd;  
}  
  
int setOpt(int fd, int nSpeed, int nBits, char nEvent, int nStop)  
{  
    struct termios newtio, oldtio;  
    if (tcgetattr(fd, &oldtio) != 0)  
    {  
        perror("SetupSerial 1");  
        return -1;  
    }  
    bzero(&newtio, sizeof(newtio));  
    newtio.c_cflag |= CLOCAL | CREAD;  
    newtio.c_cflag &= ~CSIZE;  
  
    switch (nBits)  
    {  
    case 7:  
        newtio.c_cflag |= CS7;  
        break;  
    case 8:  
        newtio.c_cflag |= CS8;  
        break;  
    }  
  
    switch (nEvent)  
    {  
    case 'O':                     //奇校验  
        newtio.c_cflag |= PARENB;  
        newtio.c_cflag |= PARODD;  
        newtio.c_iflag |= (INPCK | ISTRIP);  
        break;  
    case 'E':                     //偶校验  
        newtio.c_iflag |= (INPCK | ISTRIP);  
        newtio.c_cflag |= PARENB;  
        newtio.c_cflag &= ~PARODD;  
        break;  
    case 'N':                    //无校验  
        newtio.c_cflag &= ~PARENB;  
        break;  
    }  
  
    switch (nSpeed)  
    {  
    case 2400:  
        cfsetispeed(&newtio, B2400);  
        cfsetospeed(&newtio, B2400);  
        break;  
    case 4800:  
        cfsetispeed(&newtio, B4800);  
        cfsetospeed(&newtio, B4800);  
        break;  
    case 9600:  
        cfsetispeed(&newtio, B9600);  
        cfsetospeed(&newtio, B9600);  
        break;  
    case 115200:  
        cfsetispeed(&newtio, B115200);  
        cfsetospeed(&newtio, B115200);  
        break;  
    default:  
        cfsetispeed(&newtio, B9600);  
        cfsetospeed(&newtio, B9600);  
        break;  
    }  
    if (nStop == 1)  
    {  
        newtio.c_cflag &= ~CSTOPB;  
    }  
    else if (nStop == 2)  
    {  
        newtio.c_cflag |= CSTOPB;  
    }  
    newtio.c_cc[VTIME] = 0;  
    newtio.c_cc[VMIN] = 0;  
    tcflush(fd, TCIFLUSH);  
    if ((tcsetattr(fd, TCSANOW, &newtio)) != 0)  
    {  
        perror("com set error");  
        return -1;  
    }  
    printf("set done!\n");  
    return 0;  
}  

int UART_Recv(int fd, char *rcv_buf,int data_len)
{
    int len,fs_sel;
    fd_set fs_read;
    
    struct timeval time;
    
    FD_ZERO(&fs_read);
    FD_SET(fd,&fs_read);
    
    time.tv_sec = 10;
    time.tv_usec = 0;
    
    //使用select实现串口的多路通信
    fs_sel = select(fd+1,&fs_read,NULL,NULL,&time);
    if(fs_sel){
     len = read(fd,rcv_buf,data_len);    
     return len;
        } else {
        return -1;
    }    
}

int sendDataTty(int fd, unsigned char *send_buf, int Len)  
{  
    ssize_t ret;  
  
    ret = write(fd, send_buf, Len);  
    if (ret == -1)  
    {  
        printf("write device error\n");  
        return -1;  
    }  
  
    return 1;  
} 

float Readini()
{
    FILE *fp;
    char str[128];
    float a;
    if ((fp = fopen("./configini", "r")) == NULL) {
        printf("cannot open file/n");
    }
    fscanf(fp,"%s %f;", str,&a);
    //printf("%f\n",a);
    fclose(fp);//feof监测文件结束符，如果未结束返回非0值，结束返回0值；
    return a;
}


  
int main()  
{  
    handle_opcua_server();

    int iSetOpt = 0;//SetOpt 的增量i  
    float n = 0 ;//人工减少误差！
    
  
    //serialInit();  
    //send_data_tty(SerFd, "hello series\n", sizeof("hello series\n"));  
  
    int fdSerial = 0;  
  
    //openPort  
    if ((fdSerial = openPort(fdSerial, 1))<0)//1--"/dev/ttyS0",2--"/dev/ttyS1",3--"/dev/ttyS2",4--"/dev/ttyUSB0" 小电脑上是2--"/dev/ttyS1"  
    {  
        perror("open_port error");  
        return -1;  
    }  
    //setOpt(fdSerial, 9600, 8, 'N', 1)  
    if ((iSetOpt = setOpt(fdSerial, 115200, 8, 'N', 1))<0)  
    {  
        perror("set_opt error");  
        return -1;  
    }  
    printf("Serial fdSerial=%d\n", fdSerial);  
  
    tcflush(fdSerial, TCIOFLUSH);//清掉串口缓存  
    fcntl(fdSerial, F_SETFL, 0);  
    int i,ret,a=0;
  
    int readDataNum = 0;
    unsigned char compare_buff[512] = {0x03,0x04,0x2b,0xff,0x40,0xdc};
    unsigned char compare_buff1[512] = {0x00};
     
    sendDataTty(fdSerial, ask_buff, 8);
    UART_Recv(fdSerial, res_buff,6); 
    for(i=0;i<6;i++)
    {
        if(res_buff[i] != compare_buff[i])
        {
            printf("The DP moudle is not online!");
             return -1;
        }
    }
    
    while(1)
    { 
        sendDataTty(fdSerial,send_buff, 8);
        printf("%x,%x,%x,%x,%x,%x,%x,%x\n",send_buff[0],send_buff[1],send_buff[2],send_buff[3],send_buff[4],send_buff[5],send_buff[6],send_buff[7]); 
        ret=UART_Recv(fdSerial, recv_buff,200);  
        printf("%x,%x,%x,%x,%x,%x,%x,%x\n",recv_buff[0],recv_buff[1],recv_buff[2],recv_buff[3],recv_buff[4],recv_buff[5],recv_buff[6],recv_buff[7]); 
        n = Readini();
        if(ret == -1)
        {
            printf("Recv is incorrect");
        }

        a = 0;

        for(i=0;i<25;i++)
        {
            if(recv_buff[i] == compare_buff1[i])
            {
                a++;
            }

            if(a>=20)
            {
                printf("Something is wrong when reading the data!\n");
                return -1;
            }
        }


        unsigned char pa1[] = {recv_buff[7],recv_buff[6],recv_buff[5],recv_buff[4]};
        float *PA1 = (float*)pa1;
         //printf("%f\r\n", *PA1);
        source[2].data = *PA1;
        unsigned char pa2[] = {recv_buff[12],recv_buff[11],recv_buff[10],recv_buff[9]}; 
        float *PA2 = (float*)pa2; 
        source[3].data = (*PA2) - n;
        unsigned char dp1[] = {recv_buff[17],recv_buff[16],recv_buff[15],recv_buff[14]}; 
        float *DP1 = (float*)dp1; 
        source[0].data = *DP1; 
        unsigned char dp2[] = {recv_buff[22],recv_buff[21],recv_buff[20],recv_buff[19]}; 
        float *DP2 = (float*)dp2; 
        source[1].data = *DP2; 
        memset(recv_buff,0,sizeof(recv_buff));     
        sleep(1);
        }

 }      

#include"rfid.h"
#include"string.h"
#include"time.h"
#include"pthread.h"
#include"stddef.h"
#include"stdio.h"

#define MoveReadPointer(pStart,n)    do{ pRead = (pStart) + (n);if(pRead >= (RxBuffer+RX_BUFFER_SIZE)){pRead -= RX_BUFFER_SIZE;}}while(0) //


int DEBUGE0 = 0;
int DEBUGE1 = 0;
int DEBUGE2 = 0;
int fd = 0;
pthread_t thread1;

unsigned char RxBuffer[RX_BUFFER_SIZE+512];
unsigned char *pRead;
unsigned char *pWrite;
unsigned long int RxTotalBytes = 0;
unsigned long int RxTotalFrameCnt = 0;
unsigned long int RxCorrectFrameCnt = 0;
unsigned long int RxErrorFrameCnt = 0;

unsigned long int ReadTotalCnt = 0;
unsigned long int ReadSucceedCnt = 0;
unsigned long int ReadErrorCnt = 0;

unsigned long int WriteTotalCnt = 0;
unsigned long int WriteSucceedCnt = 0;
unsigned long int WriteErrorCnt = 0;
unsigned long int SelectCnt = 0;
unsigned long int SelectSucceedCnt = 0;

bool isGetInfo = false;
bool isGetNewData = false;
unsigned char user_data_bytes[16];
tag_t Tag[TAG_BUFFER_SIZE]; // Buffer for tags.
unsigned int TagCnt = 0;    // The sum of different tag.

void delay(clock_t ms);
void *analysis_thread(void * arg);
static void read_data(unsigned char *pHead);
static void bytes_to_int64(unsigned char *buffer,long int * result);
static void int64_to_bytes(unsigned char *bytes, long int data);
static void set_select_mode(unsigned char mode);
static void update_epc(unsigned char *epc, unsigned int epcBytes);
static void single_inventory(void);
static void get_info(void);

bool isSelect = true;// ture: select tag to read. false:read every tags
long int user_data = 0;
unsigned char epc[12] = {0xAA,0xBB,0xCC,0xDD,0xEE,0xFF,0x00,0x00,0x00,0x00,0x00,0x00};
/////////////////////////////////////////////////////////////////////////
////////////////////////                /////////////////////////////////
////////////////////////      App       /////////////////////////////////
////////////////////////                /////////////////////////////////
/////////////////////////////////////////////////////////////////////////
void rfid_init(void)
{
    pRead = RxBuffer;
    pWrite = RxBuffer;
    pthread_create(&thread1, NULL, analysis_thread, (void*)NULL);
    char port[4];
    for(int i=0;i<9;i++){
        sprintf(port,"USB%d",i);
        fd = serialport_init(port, B115200);
        get_info();
        delay(100);
        if(isGetInfo){
            break;
        }
    }
    //fd = serialport_init("USB0", B115200);
    return;
}


void read_user_data(void)
{   
    unsigned char ptr[4]={0x00,0x00,0x00,0x20};
    unsigned char ap[4] = {0x00, 0x00 ,0x00 ,0x00};
    while(1)
    {
        if(isSelect){
            select_tag(0x01,ptr,96,0x00,epc);
        }
        else{
            set_select_mode(0x01);
        }
        delay(50);
        read_tag(ap, 0x03, 0, 4);
        bytes_to_int64(user_data_bytes, &user_data);
        printf("Result: %ld\r\n",user_data);
    }
}

// Write data to user block memeory
// para:
//   data: The data write to tag
//   ap:   Assces password
//   n  : Read n times
//   intervalms: Write interval. unit :ms
bool write_user_data(long int data,unsigned char *ap,unsigned int n, unsigned int intervalms)
{
    unsigned long int cnt = WriteSucceedCnt;
    unsigned char buf[8];
    int64_to_bytes(buf, data);
    while( (WriteSucceedCnt <= cnt) && (n--))
    {
        write_tag(ap, 0x03, 0, 4, buf);
        delay(intervalms);
    }
    if(WriteSucceedCnt > cnt)
    {// Write succeed
        return true;
    }
    return false;
}



void delay(clock_t ms)
{
    clock_t s = clock();
    while(clock() - s < ms*1000){};
}


void get_info(void)
{
    unsigned char para[1] = {0x00};
    send_command(fd, CMD_GET_MODULE_INFO, 1, para);
    return;
}


void restart(void)
{
    unsigned char para[1] = {0x00};
    send_command(fd, CMD_RESTART, 0, para);
    return;
}

void single_inventory(void)
{
    unsigned char para[1] = {0x00};
    send_command(fd, CMD_SINGLE_ID, 0, para);
    return;
}

void muti_inventory(unsigned int cnt)
{
    unsigned char cnt_h = (cnt>>8)&0xFF;
    unsigned char cnt_l = (cnt>>0)&0xFF;
    unsigned char para[3] = {0x22,cnt_h,cnt_l};
    send_command(fd, CMD_MULTI_ID, 3, para);
    return;
}

void stop_muti_inventory(void)
{
    unsigned char para[1] = {0x00};
    send_command(fd, CMD_STOP_MULTI, 1, para);
    return;
}

void get_select_para()
{
    unsigned char para[1] = {0x00};
    send_command(fd, CMD_GET_SELECT_PARA, 0, para);
    return;
}

// selParam : select witch memory  // 0x00:reserve   0x01:epc   0x02:tid   0x03:user
// ptr:                            // 0x00 0x00 0x00 0x20
// maskLen: length of mask         // 96bits
// isTrunc: is trunck              // 0x00 0x01
// mask: the tag's epc             //
void select_tag(unsigned char selParam, unsigned char *ptr, unsigned int maskBits, unsigned char isTrunc, unsigned char *mask)
{    
    //selParam |= 0X80;
    unsigned char maskLen = (unsigned char)maskBits;
    unsigned char para[30] = {selParam,*(ptr),*(ptr+1),*(ptr+2),*(ptr+3),maskLen,isTrunc};
    memcpy(para+7, mask, maskBits/8);
    send_command(fd, CMD_SET_SELECT_PARA, 7+maskBits/8, para);
    SelectCnt++;
    return;
}


void set_select_mode(unsigned char mode)
{
    unsigned char para[1] = {mode};
    send_command(fd, CMD_SET_INV_MODE, 1, para);
    return;
}

// ap :access pass
// memBank : 0x00:RESVER  0x01:EPC 0x02:TID 0x03:User
// offWords : addr offfset (unit: word = 2 bytes)
// datlenWords : data length  (unit: word = 2 bytes)
void read_tag(unsigned char *accessPasswords, unsigned char memBank, unsigned int offWords, unsigned int datlenWords)
{
    unsigned char off_h = (offWords>>8)&0xFF;
    unsigned char off_l = (offWords>>0)&0xFF;
    unsigned char dl_h = (datlenWords>>8)&0xFF;
    unsigned char dl_l = (datlenWords>>0)&0xFF;
    unsigned char para[9] = {*accessPasswords,*(accessPasswords+1),*(accessPasswords+2),*(accessPasswords+3),memBank,off_h,off_l,dl_h,dl_l};
    send_command(fd, CMD_READ_DATA, 9, para);
    ReadTotalCnt ++;
    return;
}


void write_tag(unsigned char *accessPasswords, unsigned char memBank, unsigned int offWords, unsigned int datlenWords, unsigned char *data)
{
    unsigned char off_h = (offWords>>8)&0xFF;
    unsigned char off_l = (offWords>>0)&0xFF;
    unsigned char dl_h = (datlenWords>>8)&0xFF;
    unsigned char dl_l = (datlenWords>>0)&0xFF;
    unsigned char para[64] = {*accessPasswords,*(accessPasswords+1),*(accessPasswords+2),*(accessPasswords+3),memBank,off_h,off_l,dl_h,dl_l};
    memcpy(para+9, data, datlenWords*2);
    send_command(fd, CMD_WRITE_DATA, 9+2*datlenWords, para);
    WriteTotalCnt ++;
    return;
}

/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
////////////////////////      Driver    /////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
// cmd : command
// pl  : length of parameter
// para: parameter
void send_command(int fd, cmd_code cmd, unsigned int pl, unsigned char *para)
{
    unsigned char buf[64];

    // load data to send
    buf[0] = 0xBB; // Heade
    buf[1] = 0x00; // Type:command
    buf[2] = cmd;  // Command
    buf[3] = (pl>>8) & 0xFF ;// PL(MSB)
    buf[4] = (pl>>0) & 0xFF ;// PL(LSB)

    for(unsigned int i=0; i<pl; i++)
    {
        buf[i+5] = para[i];
    }

    // Calculate check sum
    unsigned char checkSum = 0;
    for(unsigned int i=1; i<5+pl; i++)
    {
        checkSum += buf[i];
    }
    buf[5+pl] = checkSum; // Check sum
    buf[6+pl] = 0x7E;     // End

    write(fd, buf, 7+pl);
    if(DEBUGE1)
    {
        printf("send:\r\n");
        for(unsigned int i=0; i<7+pl; i++)
        {
            printf("%02X ", buf[i]);
        }
        printf("\r\n");
    }

    return;
}


// return the bytes of new data
static unsigned int update_data(void)
{
    if(fd != 0){
        int n = read(fd, pWrite,30);
        if(n > 0)
        {
            RxTotalBytes += (unsigned long int)n;
            pWrite += n;
            if(pWrite >= RxBuffer + RX_BUFFER_SIZE)
            {
                pWrite -= RX_BUFFER_SIZE;
            }
        }
        static bool isOverFlow = false;
        /*有新数据，未溢出*/
        if( (pWrite - pRead) > 0) //写指针未溢出时有新数据
        {
            isOverFlow = false;
            return pWrite - pRead;
        }
        /*有新数据，溢出*/
        else if( (pWrite - pRead) < 0 )
        {
            /*尾部数据*/
            if(!isOverFlow)
            {
                isOverFlow = true;
                memcpy(RxBuffer,RxBuffer+RX_BUFFER_SIZE,pWrite - RxBuffer);
            }
            memcpy(RxBuffer+RX_BUFFER_SIZE,RxBuffer,pWrite - RxBuffer);
            return pWrite + RX_BUFFER_SIZE - 1 - pRead;
         }
         /*无新数据*/
         else
        {
            isOverFlow = false;
            return 0;
        }
    }else{
        return 0;
    }
}

void analysis(void)
{
    int step = 0;
    unsigned int newDataLen = 0;
    unsigned char *pHead = 0;
    unsigned char *pEnd = 0;

    newDataLen = update_data();
    if(newDataLen > 0)
    {// data exist        
       while(newDataLen > 0)
       {
           pHead = 0;
           pEnd = 0;
           unsigned char *pHeadTemp[20];
           int pHeadCnt = 0;
           memset(pHeadTemp,0,20);
           for(unsigned int i=0; i<newDataLen; i++)
           {
               unsigned int pl = 0;              
               if( (*(pRead+i) == 0xBB) && (pHead==0))
               {// Header found
                   //pHead = pRead+i;
                   pHeadTemp[pHeadCnt++] = pRead+i;
                   if(pHeadCnt >= 20)
                   {
                       pHeadCnt = 20;
                   }
               }
               if( *(pRead+i) == 0x7E)
               {// Tail found                   
                   pEnd = pRead+i;
                   for(int j=0; j<pHeadCnt; j++)
                   {
                       pl = (unsigned int)((*(pHeadTemp[j]+3)<<8)|(*(pHeadTemp[j]+4)));
                       if(7+pl == (pEnd-pHeadTemp[j])+1)
                       {
                           pHead = pHeadTemp[j];
                           break;
                       }
                       else
                       {
                           pHead = 0;
                       }
                   }// end for(int j=0; j<pHeadCnt; j++)
               }
               if((pEnd!=0) && (pHead!=0))
               {// probable find one frame
                    RxTotalFrameCnt ++;
                    unsigned char checkSum = 0;                                        
                    unsigned int len= pEnd-pHead; //7+pl==len+1
                    for(unsigned int j=1; j<len-1; j++)
                    {// cal check sum
                        checkSum += *(pHead+j);
                    }
                    if(DEBUGE0)printf("\r\n");
                    if(DEBUGE0)printf("pRead : %ld\r\n",pRead-RxBuffer);
                    if(DEBUGE0)printf("pWrite: %ld\r\n",pWrite-RxBuffer);
                    if(DEBUGE0)printf("pHead : %ld\r\n",pHead-RxBuffer);
                    if(DEBUGE0)printf("pEnd  : %ld\r\n",pEnd-RxBuffer);
                    if(DEBUGE0)printf("pl : %u\r\n",pl);

                    if( checkSum == *(pEnd-1) )
                    {// data correct
                        RxCorrectFrameCnt ++;
                        read_data(pHead);
                        if(DEBUGE1)
                        {
                             printf("Recv:\r\n");
                             for(unsigned int k=0; k<len+1; k++)
                             {
                                 printf("%02X ", *(pRead+k));
                             }
                             printf("\r\n");
                        }
                    }
                    else
                    {// check sum error
                         RxErrorFrameCnt ++;
                         if(DEBUGE0)
                         {
                             printf("%d Check sum error!!!\r\n",step++);
                             for(unsigned int k=0; k<30; k++)
                             {
                                 printf("%02X ", *(pHead+k));
                             }
                             printf("\r\n");
                         };

                    }
                    int l = pEnd - pRead + 1;
                    MoveReadPointer(pRead, l);
                    newDataLen -= l;
                    if(DEBUGE0)printf("RxTotalFrameCnt   : %lu\r\n",RxTotalFrameCnt);
                    if(DEBUGE0)printf("RxCorrectFrameCnt : %lu\r\n",RxCorrectFrameCnt);
                    if(DEBUGE0)printf("RxErrorFrameCnt   : %lu\r\n",RxErrorFrameCnt);
                    break;
                }// end if((pEnd!=0) && (pHead!=0))
           }// end for
           if((pHead==0) || (pEnd==0))
           {// can find header or tail,but not need to update pRead
               newDataLen = 0;
           }
       }// end while
    }// end if    
}


void *analysis_thread(void * arg)
{
    arg ++;
    while(1)
    {
         analysis();
    }
}

// little endian
static void bytes_to_int64(unsigned char *buffer,long int *result)
{
    *result = *((long int *)buffer);
     memset(buffer,0,16);
    return;
}

static void int64_to_bytes(unsigned char *bytes, long int data)
{
    unsigned char *buf;
    buf = (unsigned char *)(&data);
    bytes[0] = buf[0];
    bytes[1] = buf[1];
    bytes[2] = buf[2];
    bytes[3] = buf[3];
    bytes[4] = buf[4];
    bytes[5] = buf[5];
    bytes[6] = buf[6];
    bytes[7] = buf[7];
    return;
}

long int test_data = 0;
// read data from origin data
static void read_data(unsigned char *pHead)
{
    unsigned int bytes = 0;
    if(*(pHead+1) == 0x01)
    {//
        switch( *(pHead+2))
        {//command tyes
            case 0x39:// read succeed
                ReadSucceedCnt ++;
                bytes = (unsigned int)((*(pHead+3)<<8)|(*(pHead+4))) ;
                for(unsigned int j=20+bytes-15-1; j>=20; j--)
                {// little endian
                    user_data_bytes[j-20] = *(pHead+j);
                }               
                break;            
            case 0x49:// write succeed
                WriteSucceedCnt ++;
                break;
            case 0x0C:// select succeed
                if(*(pHead+5) == 0x00)
                {
                    SelectSucceedCnt++;
                }
            case 0xFF:
                if(*(pHead+5) == 0x09)
                {// read failed
                   isGetNewData = false;
                }
            break;
            case 0x03:
                isGetInfo = true;break;
            default:break;
        }      
    }//end if
    else if(*(pHead+1) == 0x02)
    {
        switch( *(pHead+2))
        {
            case 0x22:
                update_epc(pHead+8, 12);
                break;
            default:break;
        }
    }
    if(DEBUGE2)printf("ReadTotalCnt  : %ld\r\n",ReadTotalCnt);
    if(DEBUGE2)printf("ReadSucceedCnt: %ld\r\n\r\n",ReadSucceedCnt);
    //if(DEBUGE2)printf("ReadErrorCnt   : %ld\r\n\r\n",ReadErrorCnt);

    if(DEBUGE2)printf("WriteTotalCnt  : %ld\r\n",WriteTotalCnt);
    if(DEBUGE2)printf("WriteSucceedCnt: %ld\r\n\r\n",WriteSucceedCnt);
}

// Update tag's buffer, add new tag into buffer.
static void update_epc(unsigned char *epc, unsigned int epcBytes)
{
    for(int i=0; i<TAG_BUFFER_SIZE; i++)
    {
        if( 0 == memcmp(Tag[i].EPC, epc, epcBytes) )
        {// Already exist
           break;
        }

        if(i == TAG_BUFFER_SIZE-1)
        {// Different from all tags, add tag to buffer.
            memcpy(Tag[TagCnt].EPC, epc, epcBytes);
            TagCnt ++;
            TagCnt %= TAG_BUFFER_SIZE;
        }
    }
}

void clear_epc_buffer(void)
{
    for(int i=0; i<TAG_BUFFER_SIZE; i++)
    {
        memset(Tag[i].EPC,0,12);
    }
    TagCnt = 0;
    return;
}

void list_epc(void)
{
    printf("\r\n");
    for(unsigned int i=0;i<TagCnt;i++)
    {
        for(int j=0;j<12;j++)
        {
            printf("%02X ",Tag[i].EPC[j]);
        }        
        printf("\tTag %u\r\n",i);
    }
     printf("\r\n");
}
//////////////////////////////////////////////////////////////////////////
////////////////////////                  ////////////////////////////////
////////////////////////      DEBUGE      ////////////////////////////////
////////////////////////                  ////////////////////////////////
//////////////////////////////////////////////////////////////////////////

void debuge(void)
{
    while(1)
    {
        unsigned char str[64];
        memset(str, 0, 64);
        scanf("%s",str);

       if(0==strcmp((const char*)str, "q"))
        {// quit
             break;
        }
        else if(0==strcmp((const char*)str, "rs"))
        {// restart
            restart();
        }
        else if(0==strncmp((const char*)str, "ms",2))
        {// muti start
            unsigned int len = 0;
            sscanf((const char *)(str+2),"%d:",&len);
            muti_inventory(len);
        }
        else if(0==strcmp((const char*)str, "me"))
        {// muti end
            stop_muti_inventory();
        }
        else if(0==strcmp((const char*)str, "ss"))
        {// single start
            single_inventory();
        }
        else if(0==strcmp((const char*)str, "info"))
        {
            get_info();
        }
        else if(0==strcmp((const char*)str, "hide"))
        {
            DEBUGE0 = 0;
            DEBUGE1 = 0;
            DEBUGE2 = 0;
        }
        else if(0==strcmp((const char*)str, "show"))
        {
            DEBUGE0 = 1;
            DEBUGE1 = 1;
            DEBUGE2 = 1;
        }
        else if(0==strncmp((const char*)str, "show", 4))
        {
            int n = 0;
            sscanf((const char*)(str+4), "%d", &n);
            if(n == 0){DEBUGE0 = 1;}
            else if(n == 1){DEBUGE1 = 1;}
            else if(n == 2){DEBUGE2 = 1;}
        }
        else if(0==strcmp((const char*)str, "getsel"))
        {// get sel para
            get_select_para();
        }
        else if(0==strcmp((const char*)(str), "read"))
        {// read dafta
            char c[2];
            printf("Select? y/n ");
            scanf("%s",c);
            if(c[0] == 'y' || c[0] == 'Y'){
                isSelect = true;
            }else{
                isSelect = false;
            }
            read_user_data();
        }
        else if(0==strcmp((const char*)(str), "write"))
        {// write data
            long int cnt = 0;
            printf("Write tiems: ");
            scanf("%ld:",&cnt);            
            unsigned char ap[4] = {0x00, 0x00 ,0x00 ,0x00};
            printf("Data: ");
            long int data = 0;
            scanf("%ld:",&data);
            if(true == write_user_data(data, ap,cnt,100))
            {
                printf("Write succeed!\r\n");
            }
            else
            {
                printf("Write filed!\r\n");
            }
        }
        else if(0==strncmp((const char*)str, "sel",3))
        {// select
            unsigned char ptr[4]={0x00,0x00,0x00,0x20};
          //unsigned char epc[12] = {0x30,0x08,0x33,0xB2,0xDD,0xD9,0x01,0x40,0x00,0x00,0x00,0x00};
            unsigned char epc1[12] = {0x30,0x08,0x33,0xB2,0xDD,0xD9,0x01,0x40,0x12,0x34,0x00,0x00};
            if(TagCnt != 0)
            {
                list_epc();
                printf("Select tag: ");
                int index = 0;
                scanf("%d",&index);
                memcpy(epc1, Tag[index].EPC, 12);
            }
            select_tag(0x03,ptr,96,0x00,epc1);
        }
        else if(0==strcmp((const char*)str, "re"))
        {// read epc
            printf("Read EPC\r\n");
            unsigned char ap[4] = {0x00, 0x00, 0x00, 0x00};
            read_tag(ap, 0x01, 2, 6);
        }
        else if(0==strcmp((const char*)str, "we"))
        {// write epc
            printf("RWrite EPC\r\n");
            unsigned char ap[4] = {0x00, 0x00, 0x00, 0x00};
            unsigned char data[12] = {0xDD,0xBB,0xCC,0xDD,0xEE,0xFF,0x00,0x00,0x00,0x00,0x00,0x00};
            write_tag(ap, 0x01, 2, 6, data);
        }
        else if(0==strcmp((const char*)str, "list"))
        {// list epc
           list_epc();
        }
        else
        {

        }
    }
    tcflush(fd, TCIOFLUSH);
    close(fd);
    printf("COM closed!\r\n");
}








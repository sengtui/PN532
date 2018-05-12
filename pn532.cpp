#include "pn532.hpp"
#define DEBUG

using namespace std;

void printHEX(const char* topic, char* str, int len)
{

  fprintf(stderr, "[%s %d] ",topic, len);
  if(len > 255) {
    fprintf(stderr, "Length > 256, set to 256\n");
    len = 256;
  }
  for(int i = 0; i< len; i++) fprintf(stderr, "%02X ", (unsigned char)str[i]);
  fprintf(stderr, "\n");
}


PN532::PN532(const char* ttyS){

    dev = new mraa::Uart(ttyS);
    bzero(rxStr, 256);
    bzero(txStr, 256);

    dev->setBaudRate(115200);
    dev->setMode(8, mraa::UART_PARITY_NONE,1);
    dev->setFlowcontrol(false,false);
    dev->setTimeout(44,44,44);
}

PN532::~PN532(){
    delete dev;
}

// Wake up sequence: 0x55 0x55 + lot of 0x00 until it awake.
// Then send 0x00 0x00 0xff 0x03 0xfd to notify 3 bytes command:
// 0xD4 0x14 0x01  (SamConfigue: Normal mode, timeout=50ms)
// Then [DCS][00] 0x17 0x00 
// Where [DCS]+ 0xD4 + 0x14 + 0x01 = 0x00
bool PN532::wakeUp()
{
    int ret;
    
    char WAKEUP[] ={
        0x55, 0x55, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0xff, 0x03, 0xfd, 0xd4,
        0x14, 0x01, 0x17, 0x00
    };

    dev->write(WAKEUP, 24);
    usleep(200000);

    return ret;
}

bool PN532::RFConfiguration(void)
{
    int ret;
    char RFCFG[]={ 0xD4, 0x32, 0x05, 0x02, 0x01, 0x40 };

    cout<<"Configure RF parameters..." <<endl;
    ret = Query(RFCFG, 6, false);
    return true;
}
// GerFirmwareVersion: 0xD4 0x02 
// Then [DCS] 0x2A 
// Where [DCS] = 0x100 - 0xD4 - 0x02
bool PN532::GetFirmwareVersion(void)
{
    int ret;
    
    char cmd[] ={ 0xD4, 0x02 };
    cout<<"GetFirmwareVersion..."<<endl;
    ret = Query(cmd, 2, false);
    info_IC = Reply[2];
    info_Ver= Reply[3];
    info_Rev= Reply[4];
    info_Support = Reply[5];
    fprintf(stderr, "IC: %02X, Ver: %d, Rev: %d, Support: %02X\n", info_IC, info_Ver, info_Rev, info_Support);
    if(info_Support|1) cout << "Supported: ISO/IEC 14443 TypeA\n";
    if(info_Support|2) cout << "Supported: ISO/IEC 14443 TypeB\n";
    if(info_Support|4) cout << "Supported: ISO18092\n";
    return true;
}
// GerGeneralStatus: 0xD4 0x04 
// Where [DCS] = 0x100 - 0xD4 - 0x04
bool PN532::GetGeneralStatus(void)
{
    int ret;
    
    char cmd[] ={ 0xD4, 0x04 };
    cout<<"GetGeneralStatus..."<<endl;
    ret = Query(cmd, 2, true);
    printHEX("Status", Reply, ret);
 
    return true;
}

bool PN532::auth(void)
{
    int ret;
    
    char cmd[] ={ 0xD4, 0x40, 0x01, 0x60, 0x07, 
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00,0x00,0x00,0x00 };
    cout<<"Authencate..."<<endl;
    for(int i=0;i<4;i++) cmd[i+11]=UID[i];

    ret = Query(cmd, 15, true);
    printHEX("Status", Reply, ret);
 
    return true;
}

// ListPassiveTarget: 0xD4 0x4A 0X02 0X00
// Where [DCS] = 0x100 - 0xD4 - 0x04
bool PN532::ListPassiveTarget(void)
{
    int ret;
    int NbTg;
    
    char cmd[] ={
        0xD4, 0x4A, 0x01, 0x00
    };
    
    cout<<"ListPassiveTarget..."<<endl;
    ret = Query(cmd, 4, true);
    NbTg = rxStr[7];
    if((NbTg == 0) || (ret==0)) {
        cout<<"No card listed"<<endl;
        for(int i=0;i<12;i++) UID[i]=0;
    } 
    else{
        lenUID = rxStr[12];
        if(lenUID<12){
            memcpy(UID, rxStr+13, lenUID);
            printHEX("UID", UID, lenUID);
        }
    }

    return true;
}
/* Query wraps the PN532 command to txString in following spec:
   0x00 0x00 0xFF LEN LCS CMD VAR1 VAR2 ....VARn CHK 0x00
   head = 0x00 0x00 0xFF
   LEN = Bytes counts from CMD to VARn (which is n+1)
   LCS = ~LEN+1  (LCS+LEN = 0x100)
   CHK = ~SUM(CMD, VAR1, ... VARn)+1
   Total lenght transmit = cmdLen + 7

*/
int PN532::Query(char* cmd, int cmdLen, bool isLog)
{
    unsigned char txLEN, txLCS, txCHK;
    char c;
    int ret;

    c =0;
    for(int i=0;i<cmdLen;i++){
        c+= cmd[i];
    }
    txCHK = ~c+1;
    txLEN = (unsigned char) cmdLen;
    txLCS = ~txLEN+1;

    txStr[0]= 0x00;
    txStr[1]= 0x00;
    txStr[2]= 0xFF;
    txStr[3]= txLEN;
    txStr[4]= txLCS;
    memcpy(txStr+5, cmd, cmdLen);
    txStr[5+cmdLen]=txCHK;
    txStr[6+cmdLen]=0x00;
    return(rawCommand(7+cmdLen, isLog));

}

/* rawCommand send raw data to PN532, read ACK, then wait for reply to read and read them.
    1. send cmd
    2. wait 5mS, this is important to keep a gap to earlier communication otherwise PN532 go fucked up.
    3. Read ACK, if can not read 6 chars, read the rest again.
    4. Wait until uart is ready to read, this can be replaced by an IRQ
    5. Read a long line, twice. Don't ask me. PN532 always prefer to answer question in two paragraph, no reason.
    6. Start to analysis the buffer, locate the first 0xFF (which means 0x00 0x00 0xFF <- pointer stop here)
    7. Check next two bytes if they are 2's compliment (LEN = ~LCS+1), otherwise try to find next 0xFF.
    8. Check if the length of read buffer match definition of LEN ( Output= 0x00 0x00 0xFF LEN LCS [DATA LEN] CHK 0x00), 
    if not, we have a problem here.... Let's send ACK to reset PN532's answer, (should send NACK to ask PN532 to send again... but not working so far)
    9. make a copy of [DATA LEN] to Reply[], and return the Reply LEN. 
*/
int PN532::rawCommand(int txLen, bool isLog)
{
    int ret, remain;
    int LEN, LCS, CMD;
    
    char NACK[] ={0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00};
    char ACK[]={0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00};
 
    try{
    // Send Command
        if(isLog) printHEX("TX", (char*)txStr, txLen);
        dev->write((char*)txStr, txLen);
    // Wait for PN532 to settle down, >5mS , very very important, otherwise PN532 go crazy and start to lost sync of reply...
        usleep(10000);
    // Wait for data from serial line, timeout 22ms
        if(!dev->dataAvailable(22)){
            cout<<"[ACK] Reply timeout 1" << endl;
            return false;
        }
        ret = dev->read(rxStr,6);   // Read ACK 6 chars 
        if(ret<6) ret+= dev->read(rxStr+ret, 6-ret); // Buffer run out?? read the rest!
        if(isLog) printHEX("ACK", rxStr, ret);

        if(!dev->dataAvailable(5000)){
            cout<<"[RX] Reply timeout" << endl;
            dev->write(ACK,6); // Ask PN532 to give up, reset all pending job, wait for next command.
            usleep(100000);
            return false;
        }

// Read heading: 0x00 0x00 0xFF LEN SUM

        ret = dev->read(rxStr,100);
        if(ret<6) 
            if(dev->dataAvailable(100))
                ret+= dev->read(rxStr+ret, 100);
    
        //locate 0xFF
        for(int i=0;i<ret-3;i++){
            if((unsigned char)rxStr[i]==255){
                LEN = rxStr[i+1];
                LCS = rxStr[i+2];
                if((LEN+LCS)==0){
                    CMD=i+3;
                    if(ret<CMD+LEN+2){
                        fprintf(stderr,"Length %d, expected %d (CMD %d + LEN %d + 2)\n", ret, (CMD+LEN+2), CMD, LEN);
                        if(dev->dataAvailable(100)) ret+=dev->read(rxStr+ret, (CMD+LEN+2-ret));
                    }
                    break;
                }
            }
        }
        if((LEN+LCS)!=0){
            fprintf(stderr, "Paragraph LEN %d does not match to LCS %d, reset peer\n", LEN, LCS);
            dev->write(ACK, 6);
            return 0;
        }
        
        if(isLog) printHEX("HEAD", rxStr, ret);

        if(isLog) cout<< "LEN: " << LEN <<" LCS: "<< LCS << endl;

        if((LEN+LCS)!=0){
            cout << "[RX] Read header error" << LEN+LCS << endl;
            printHEX("RX", rxStr, ret);
        }
        memcpy(Reply, rxStr+CMD, LEN);



    } catch (std::exception& e)
    {
        std::cout<<e.what() << "Read / Write Error" << std::endl;
    }
    // Send ACK to acknowledge the receipt of reply, or to reset the reply
 //   dev->write(ACK,6);
 
    return LEN;
}


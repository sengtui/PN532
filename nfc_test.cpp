#include "pn532.hpp"

main()
{
    PN532* reader;
    int cc,len;
    char cmd[256],buf[256];
    int bytes=0;

    int i=0;
    reader = new PN532("/dev/ttyS1");
    reader->wakeUp();
    reader->RFConfiguration(false);
    reader->GetFirmwareVersion(false);
    reader->GetGeneralStatus(false);
          
        if(reader->ListPassiveTarget(false)){
            reader->auth(false);
        }
        fprintf(stderr,"Type your command:\n");
    do{
        bytes=0;
         do{
            scanf("%s", buf);
            len = strlen(buf);
            buf[len]=0;
            if(len>2) {
                sscanf(buf, "%x", &cc);
                cmd[bytes]=(char)cc;
                bytes++;
            }
        } while (len>2);
        printHEX("COMMAND", cmd, bytes);
        reader->Query(cmd, bytes, true);   
        usleep(100000);
    } while (i++<1000);
    delete reader;

}
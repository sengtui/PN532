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
    reader->RFConfiguration();
    reader->GetFirmwareVersion();
    reader->GetGeneralStatus();
          
        if(reader->ListPassiveTarget()){
            reader->auth();
        }
        fprintf(stderr,"<<<<%d>>>>\n",i);
    do{
        bytes=0;
         do{
            scanf("%s", buf);
            fprintf(stderr, "Get: [%s]\n", buf);
            len = strlen(buf);
            buf[len]=0;
            if(len>2) {
                sscanf(buf, "%x", &cc);
                cmd[bytes]=(char)cc;
                bytes++;
            }
        } while (len>2);
        printHEX("BYTES", cmd, bytes);
        reader->Query(cmd, bytes, true);   
        usleep(100000);
    } while (i++<1000);
    delete reader;

}
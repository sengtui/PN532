#ifndef PN532_HPP
#define PN532_HPP

#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <mraa.hpp>



void printHEX(const char* item, char* str, int len);

class PN532 {
  public:
      PN532(const char* ttyS);
      bool wakeUp();
      bool auth();
      bool RFConfiguration(void);
      bool GetFirmwareVersion(void);
      bool GetGeneralStatus(void);
      bool ListPassiveTarget(void);
      int Query(char* cmd, int txLen, bool isLog);
      
    
      ~PN532();
  private:
    mraa::Uart* dev;
    unsigned char txStr[256];
    char rxStr[256];
    char Reply[256];
    int rawCommand(int txLen, bool isLog);
    char UID[12];
    int lenUID;
    int info_IC, info_Ver, info_Rev, info_Support;
 
};

#endif

#ifndef SIM800_TCALL_H
#define SIM800_TCALL_H

#include "Arduino.h"

/*
    Adapted from https://github.com/vittorioexp/Sim800L-Arduino-Library-revised
*/

class Sim800
{
public:
    Sim800(HardwareSerial &serial, int rxPin, int txPin, int resetPin, int powerPin, int pwrKey);
    void setDebugSerial(HardwareSerial *debugSerial);
    void begin();
    bool reset(String pinCode = "");

    bool debugATLoop();

    // AT commands
    bool setPIN(String pin);
    String getProductInfo();
    String getOperatorsList();
    String getOperator();
    bool sendSms(String number, String text);
    bool prepareForSmsReceive();

    /*
        If several sms are received at the same time,
        This function gets only the first
        The others will be lost.
    */
    uint8_t checkForSMS();

    bool getSms(uint8_t index, String *number, String *text);
    bool delAllSms();

    bool enableSleep();
    bool disableSleep();

private:
    HardwareSerial &_serial;
    HardwareSerial *_debugSerial = nullptr;
    int _rxPin;
    int _txPin;
    int _resetPin;
    int _powerPin;
    int _pwrKey;

    String _readSerial(time_t timeout = 5000);
    void _printSerial(String text);
    void _printSerial(char character);
    void _printSerial(uint8_t number);
};

#endif
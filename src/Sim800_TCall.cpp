#include "Sim800_TCall.h"

#include <driver/uart.h>

Sim800::Sim800(HardwareSerial &serial, int rxPin, int txPin, int resetPin, int powerPin, int pwrKey)
    : _serial(serial),
      _rxPin(rxPin),
      _txPin(txPin),
      _resetPin(resetPin),
      _powerPin(powerPin),
      _pwrKey(pwrKey) {
}

void Sim800::begin() {
    pinMode(_resetPin, OUTPUT);
    pinMode(_powerPin, OUTPUT);
    pinMode(_pwrKey, OUTPUT);
    _serial.begin(9600, SERIAL_8N1, _rxPin, _txPin);
}

void Sim800::setDebugSerial(HardwareSerial *debugSerial) {
    _debugSerial = debugSerial;
}

bool Sim800::reset(String pinCode) {
    digitalWrite(_resetPin, LOW);
    delay(500);
    digitalWrite(_resetPin, HIGH);
    digitalWrite(_powerPin, HIGH);
    digitalWrite(_pwrKey, HIGH);
    delay(100);
    digitalWrite(_pwrKey, LOW);
    delay(1000);
    digitalWrite(_pwrKey, HIGH);

    delay(500);

    for (int i = 0; i < 10; i++) {
        _printSerial("AT\r\n");
        String buffer = _readSerial(1000);
        if (buffer.indexOf("OK") != -1) {
            if (buffer.indexOf("+CPIN: SIM PIN") != -1) {
                // pin code is requested
                if (pinCode.isEmpty()) {
                    Serial.println("Error: PIN code is required");
                    return true;
                }
                setPIN(pinCode);
            }
            break;
        }
    }

    for (int i = 0; i < 15; ++i) {
        String res = _readSerial(1000);
        if (res.indexOf("SMS Ready") != -1) {
            return false;
        }
    }
    return true;
}

String Sim800::_readSerial(time_t timeout) {
    String response = "";
    time_t timeMarker = millis();

    while (millis() - timeMarker < timeout && !_serial.available()) {
        delay(1);
    }

    if (_serial.available()) {
        response = _serial.readString();
    }
    if (_debugSerial && !response.isEmpty()) {
        String res2 = response;
        res2.replace("\r\n", "\r\n >>");
        _debugSerial->print(F(">> "));
        _debugSerial->print(res2);
    }

    return response;
}

void Sim800::_printSerial(String text) {
    _serial.print(text);
    if (_debugSerial) {
        _debugSerial->print(F("<< "));
        _debugSerial->print(text);
    }
}
void Sim800::_printSerial(char character) {
    _serial.print(character);
    if (_debugSerial) {
        _debugSerial->print(F("<< "));
        _debugSerial->print(character);
    }
}
void Sim800::_printSerial(uint8_t number) {
    _serial.print(number);
    if (_debugSerial) {
        _debugSerial->print(F("<< "));
        _debugSerial->print(number);
    }
}

bool Sim800::debugATLoop() {
    if (!_debugSerial)
        return false;

    _debugSerial->println(F("***********************************************************"));
    _debugSerial->println(F(" You can now send AT commands"));
    _debugSerial->println(F(" Enter \"AT\" (without quotes), and you should see \"OK\""));
    _debugSerial->println(F(" If it doesn't work, select \"Both NL & CR\" in Serial Monitor"));
    _debugSerial->println(F(" DISCLAIMER: Entering AT commands without knowing what they do"));
    _debugSerial->println(F(" can have undesired consiquences..."));
    _debugSerial->println(F("***********************************************************\n"));

    while (true) {
        while (_serial.available()) {
            _debugSerial->write(_serial.read());
        }
        while (_debugSerial->available()) {
            _serial.write(_debugSerial->read());
        }
        delay(1);
    }
    // Error found, return 1
}

// AT commands
bool Sim800::setPIN(String pin) {
    String command;
    command = "AT+CPIN=";
    command += pin;
    command += "\r";
    _printSerial(command);

    // Can take up to 5 seconds
    if ((_readSerial(5000).indexOf("ER")) == -1) {
        return true;
    }
    else
        return false;
    // Error found, return 0
    // Error NOT found, return 1
}

String Sim800::getProductInfo() {
    _printSerial("ATI\r");
    return (_readSerial());
}

String Sim800::getOperatorsList() {
    // Can take up to 45 seconds
    _printSerial("AT+COPS=?\r");
    return _readSerial(45000);
}

String Sim800::getOperator() {
    _printSerial("AT+COPS ?\r");
    return _readSerial();
}

bool Sim800::sendSms(String number, String text) {
    _printSerial(F("AT+CMGF=1\r")); // set sms to text mode
    _readSerial();
    _printSerial(F("AT+CMGS=\"")); // command to send sms
    _printSerial(number);
    _printSerial(F("\"\r"));
    _readSerial();
    _printSerial(text);
    _printSerial("\r");
    _readSerial();
    _printSerial((char)26);

    String buffer = _readSerial(60000);

    // expect CMGS:xxx   , where xxx is a number,for the sending sms.
    if ((buffer.indexOf("ER")) != -1) {
        return false;
    }
    else if ((buffer.indexOf("CMGS")) != -1) {
        return true;
    }
    else {
        return false;
    }
    // Error found, return 0
    // Error NOT found, return 1
}

bool Sim800::prepareForSmsReceive() {
    // Configure SMS in text mode
    _printSerial(F("AT+CMGF=1\r"));
    String buffer = _readSerial();
    if ((buffer.indexOf("OK")) == -1) {
        return false;
    }
    _printSerial(F("AT+CNMI=2,1,0,0,0\r"));
    buffer = _readSerial();
    if ((buffer.indexOf("OK")) == -1) {
        return false;
    }
    return true;
}

uint8_t Sim800::checkForSMS() {
    // Returns the index of the new SMS
    String buffer = _readSerial(100);
    if (buffer.length() == 0) {
        return 0;
    }
    buffer += _readSerial(1000);
    // +CMTI: "SM",1
    if (buffer.indexOf("CMTI") == -1) {
        return 0;
    }
    return buffer.substring(buffer.indexOf(',') + 1).toInt();
}

bool Sim800::getSms(uint8_t index, String *number, String *text) {
    if ((_readSerial(1000).indexOf("ER")) != -1) {
        return false;
    }

    _printSerial(F("AT+CMGR="));
    _printSerial(index);
    _printSerial("\r");
    String buffer = _readSerial();
    int indexCMGR = buffer.indexOf("CMGR");
    if (indexCMGR == -1) {
        return false;
    }

    // get number
    int start = buffer.indexOf("+", indexCMGR);
    int end = buffer.indexOf("\"", start);
    if (start == -1 || end == -1) {
        return false;
    }
    *number = buffer.substring(start, end);

    // get message
    start = buffer.indexOf("\n", indexCMGR) + 1;
    end = buffer.lastIndexOf("\nOK");
    if (start == -1 || end == -1) {
        return false;
    }
    *text = buffer.substring(start, end);
    text->trim();
    return true;
}

bool Sim800::delAllSms() {
    // Can take up to 25 seconds

    _printSerial(F("at+cmgda=\"del all\"\n\r"));
    String buffer = _readSerial(25000);
    if ((buffer.indexOf("ER")) == -1) {
        return true;
    }
    else
        return false;
    // Error found, return 0
    // Error NOT found, return 1
}

bool Sim800::enableSleep() {
    _printSerial(F("AT+CSCLK=2\r"));
    String buffer = _readSerial();
    if ((buffer.indexOf("OK")) == -1) {
        return false;
    }
    return true;
}

bool Sim800::disableSleep() {
    String buffer = _readSerial(10);
    _printSerial(F("AT\r")); // The first has no response
    delay(100);
    _printSerial(F("AT\r"));
    buffer = _readSerial();
    if ((buffer.indexOf("OK")) == -1) {
        return false;
    }
    _printSerial(F("AT+CSCLK=0\r"));
    buffer = _readSerial();
    if ((buffer.indexOf("OK")) == -1) {
        return false;
    }
    return true;
}
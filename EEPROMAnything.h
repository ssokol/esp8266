#include <EEPROM.h>
#include <Arduino.h>  // for type definitions

int eepromReadInt(int address){
   int value = 0x0000;
   value = value | (EEPROM.read(address) << 8);
   value = value | EEPROM.read(address+1);
   return value;
}
 
void eepromWriteInt(int address, int value){
   EEPROM.write(address, (value >> 8) & 0xFF );
   EEPROM.write(address+1, value & 0xFF);
}
 
float eepromReadFloat(int address){
   union u_tag {
     byte b[4];
     float fval;
   } u;   
   u.b[0] = EEPROM.read(address);
   u.b[1] = EEPROM.read(address+1);
   u.b[2] = EEPROM.read(address+2);
   u.b[3] = EEPROM.read(address+3);
   return u.fval;
}
 
void eepromWriteFloat(int address, float value){
   union u_tag {
     byte b[4];
     float fval;
   } u;
   u.fval=value;
 
   EEPROM.write(address  , u.b[0]);
   EEPROM.write(address+1, u.b[1]);
   EEPROM.write(address+2, u.b[2]);
   EEPROM.write(address+3, u.b[3]);
}

void eepromReadString(int offset, int bytes, char *buf){
  char c = 0;
  for (int i = offset; i < (offset + bytes); i++) {
    c = EEPROM.read(i);
    buf[i - offset] = c;
    if (c == 0) break;
  }
}

void eepromWriteString(int offset, int bytes, char *buf){
  char c = 0;
  //int len = (strlen(buf) < bytes) ? strlen(buf) : bytes;
  for (int i = 0; i < bytes; i++) {
    c = buf[i];
    EEPROM.write(offset + i, c); 
  }
}

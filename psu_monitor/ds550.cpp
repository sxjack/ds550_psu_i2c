/* -*- tab-width: 2; mode: c; -*-
 * 
 * Minimal C++ class for interfacing with the Astec DS550-3 PSU.
 *
 * Copyright (c) 2020, Steve Jack. All rights reserved.
 *
 * Use at your own risk.
 *
 * Notes
 *
 * DS450-3 / DS550-3 PS I2C, Technical Report, Phil Toplansky
 * 
 * The Dell AA23300 is slightly different to the DS550-3 on the I2C bus. 
 * The EEPROM is at 0x56.
 *
 * 
 */

#pragma GCC diagnostic warning "-Wunused-variable"

#define DIAGNOSTICS  1

#include "ds550.h"

#include <Wire.h>

/*
 *
 */

ds550::ds550() {

  eeprom_header  =
  eeprom_data    = eeprom_image;
  eeprom_chassis = 
  eeprom_product = &eeprom_image[8];

  return;
}

//

int ds550::init(int output,Stream *Debug_Serial) {

  int      i, offset;
  char     text[32];
  uint16_t power;

  Debug   = Debug_Serial;
  PSON_do = output;

  pinMode(PSON_do,OUTPUT);

  // Read the EEPROM out of curiosity.

  memset(eeprom_image,0,256);

  for (offset = 0; offset < 256; offset += 16) {

    delay(50);
    read_eeprom((uint8_t) offset,(uint8_t) 16,&eeprom_image[offset]);
  }

  //

  eeprom_product = &eeprom_image[8 + (8 * eeprom_chassis[1])];

  // This should be 72 bytes of PSU, 12V and 3.3V data.

  eeprom_data = &eeprom_image[8 * eeprom_header[5]];
  power       = ((uint16_t) eeprom_data[5]) | (((uint16_t) eeprom_data[6]) << 8);

  //

  for (i = 0; i < 30; ++i) {

    text[i] = (char) eeprom_chassis[i + 11];
  }

  text[i] = 0;

  if (Debug) {

    Debug->print(text);
    Debug->print("\r\n");

    sprintf(text,"%uW\r\n",power);
    Debug->print(text);
  }
 
//

  standby();

  if (Debug) {

    Debug->print("ds550::init() complete\r\n");
  }

  return 0;
}

//

void ds550::standby() {

  digitalWrite(PSON_do,HIGH);

  return;
}

//

void ds550::on() {

  digitalWrite(PSON_do,LOW);

  return;
}

//

int ds550::scan() {

  int      status, bytes = 0;
  
  Wire.beginTransmission(processor_addr);
  // Wire.write(0x00);

  if ((status = Wire.endTransmission(0)) == 0) {

    if (bytes = Wire.requestFrom(processor_addr,(uint8_t) 1)) {

      status_reg = Wire.read();

      OCP_12V    =  (status_reg       & 0x01);
      Fan_Fault  = ((status_reg >> 1) & 0x01);
      UVP_12V    = ((status_reg >> 2) & 0x01);
      OVP_12V    = ((status_reg >> 3) & 0x01);
      Vin_Good   = ((status_reg >> 4) & 0x01);
      P_Good     = ((status_reg >> 5) & 0x01);
      PS_Status  = ((status_reg >> 6) & 0x01);
      AC_Pfail   = ((status_reg >> 7) & 0x01);
    }
  }

  if (Debug) {

    char text[16];

    sprintf(text,"%2d,%2d,%02x\r\n",status,bytes,status_reg);
    Debug->print(text);
  }

  return 0;
}

//

uint32_t ds550::read_eeprom(uint8_t offset, uint8_t bytes, uint8_t *buffer) {

  int      i, status = 0, received = 0;
  uint32_t result = 0;
#if DIAGNOSTICS
  char     text[16], c;
#endif

  Wire.beginTransmission(eeprom_addr);
  Wire.write(offset);

  if ((status = Wire.endTransmission(0)) == 0) {

    received = Wire.requestFrom(eeprom_addr,bytes);

    for (i = 0; i < received; ++i) {

      buffer[i] = Wire.read();

      if (i < 4) {

        result <<= 8;
        result  |= buffer[i];
      }
    }
  }

#if DIAGNOSTICS

  if (Debug) {

    sprintf(text,"%02x: ",offset);
    Debug->print(text);

    for (i = 0; i < received; ++i) {

      sprintf(text,"%02x ",buffer[i]);
      Debug->print(text);
    }

    for (i = 0; i < received; ++i) {

      c = ((buffer[i] > 31)&&(buffer[i] < 127)) ? buffer[i]: '.';
      Debug->print(c);
    }

    Debug->print("\r\n");
  }

#endif

  return result;
}

/*
 *
 */


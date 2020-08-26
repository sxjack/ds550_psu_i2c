/*
 * Minimal C++ class for interfacing with the Astec DS550-3 PSU.
 *
 * Copyright (c) 2020, Steve Jack. All rights reserved.
 *
 * Use at your own risk.
 *
 *
 */

#ifndef DS550_H
#define DS550_H

#include <Arduino.h>

/*
 *
 */

class ds550 {

 public:
                ds550();
  int           init(int,Stream *);
  int           scan(void);
  void          standby(void);
  void          on(void);

  double        V_in = 240.0, I_out = 0.0;
  int           T_1 = 20, T_2 = 20;
  short int     OCP_12V = 0, Fan_Fault = 0, UVP_12V = 0, OVP_12V = 0, 
                Vin_Good = 0, P_Good = 0, PS_Status = 0, AC_Pfail = 0;
  uint8_t       status_reg = 0x00,
               *eeprom_product; 
 
 private:
  uint32_t      read_eeprom(uint8_t,uint8_t,uint8_t *);

  short int     PSON_do = 0;
  uint8_t       processor_addr = 0x3e, eeprom_addr = 0x56,
                eeprom_image[256], *eeprom_header, *eeprom_data, *eeprom_chassis; 
  Stream       *Debug = NULL;              
};

#endif


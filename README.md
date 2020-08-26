# ds550_psu_i2c
An Arduino sketch to communicate with an Astec DS550-3 (aka a Dell AA23300) PSU over I2C.

I wrote this thinking that I would be able to get some useful information out of an AA23300
that I use as a bench power supply. Sadly all that is available is a single byte, so it is 
pretty useless.

The EEPROM for the Dell AA23300 doesn't have all the information in it that the technical 
summary for the DS550-3 says that it should.

The AA23300 responds on three I2C addresses. The EEPROM is at 0x56 and the processor is at
0x3e. I haven't fully investiagated 0x2b.

Tested on a Wemos D1 Mini and a STM32F103C 'Blue Bill'.

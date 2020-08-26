/* -*- tab-width: 2; mode: c; -*-
 * 
 * Program to extract data from a Dell AA23300 (Astec DS550-3) power supply and present it. 
 *
 * Copyright (c) 2020, Steve Jack. All rights reserved.
 *
 * Use at your own risk.
 *
 * Notes
 *
 * Developed with a WeMos D1 Mini (ARDUINO_ESP8266_WEMOS_D1MINI), 
 * a blue pill (ARDUINO_GENERIC_STM32F103C) and a Pro Mini (ARDUINO_AVR_PRO).
 * 
 * Use an ESP if you want a web interface, the STM32 if you just want a display.
 * 
 *
 * 
 * 
 */

#if (not defined(ARDUINO_AVR_PRO)) && (not defined(ARDUINO_ESP8266_WEMOS_D1MINI)) && (not defined(ARDUINO_GENERIC_STM32F103C))
#error "This program is only for Pro Mini\'s, Wemos D1 Minis and STM32F103C\'s."
#endif

#pragma GCC diagnostic warning "-Wunused-variable"

#define DIAGNOSTICS  1
#define LCD_DISPLAY  0
#define WEBSERVER    1

/*
 *
 */

#include <Arduino.h>
#include <Wire.h>

#if defined(ARDUINO_ESP8266_WEMOS_D1MINI)

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <ESP8266HTTPClient.h>

int  syslog(IPAddress,char *);
int  connectWiFi(void);
void get_addresses(void);
void webserver(void);

static char        gateway_s[18], my_ip_s[18];
static IPAddress   my_ip, gateway_ip;
static WiFiUDP     udp;
#if WEBSERVER
static WiFiServer  server(80);
#endif

#elif defined(ARDUINO_GENERIC_STM32F103C)

void yield(void);

#endif

#include "ds550.h"

/*
 *
 */

#if (LCD_DISPLAY > 10) && (LCD_DISPLAY < 20) // LCD displays on I2C.

#include <U8x8lib.h>

#if LCD_DISPLAY == 11
const int display_address = 0x78;
U8X8_SH1106_128X64_NONAME_HW_I2C u8x8(/* reset=*/ U8X8_PIN_NONE);
#elif LCD_DISPLAY == 12
const int display_address = 0x78;
U8X8_SSD1306_128X64_NONAME_HW_I2C u8x8(U8X8_PIN_NONE);
#elif LCD_DISPLAY == 16
const int display_address = 0x78; // 0x3c?
U8X8_SSD1306_64X48_ER_HW_I2C u8x8(U8X8_PIN_NONE);
#endif

#endif

/*
 * 
 */

static const char     *title = "DS550-3", *build_date = __DATE__;

static int             display_enabled = 0;

static ds550           psu;

#if defined(ARDUINO_ESP8266_WEMOS_D1MINI)
static const int       PSON_do =   D5;
#elif defined(ARDUINO_GENERIC_STM32F103C)
static const int       PSON_do = PC13;
#else
static const int       PSON_do =   13;
#endif

/*
 * 
 */

void setup() {

  int status;

	status = 0;

#if defined(ARDUINO_ESP8266_WEMOS_D1MINI)

  Wire.begin(D2,D1);

#elif defined(ARDUINO_GENERIC_STM32F103C)

  Wire.begin();
  // Wire2.begin();

#else

  Wire.begin();

#endif

	Wire.setClock(100000);

#if defined(ARDUINO_GENERIC_STM32F103C)

  Serial2.begin(57600);

#define Debug Serial2

#else 

  Serial.begin(57600);

#define Debug Serial

#endif

#if DIAGNOSTICS

  Debug.print("\r\n");
  Debug.print((char *) title);
	Debug.print("\r\n");
	Debug.print((char *) build_date);
	Debug.print("\r\n\n");

#endif

	//

	delay(100);

	psu.init(PSON_do,&Debug);

	yield();

#if (LCD_DISPLAY > 10) && (LCD_DISPLAY < 20)

  u8x8.setI2CAddress(display_address);
  u8x8.begin();
  u8x8.setPowerSave(0);

	u8x8.setFont(u8x8_font_chroma48medium8_r);
 
	u8x8.refreshDisplay();

	display_enabled = 1;

#if LCD_DISPLAY < 16
  u8x8.drawString(5,0,(char *) title);
  u8x8.drawString(3,1,(char *) build_date);
#elif LCD_DISPLAY == 16
  u8x8.drawString(0,0,(char *) title);
#endif

#endif

	yield();

	//

#if defined(ARDUINO_ESP8266_WEMOS_D1MINI)

	status = connectWiFi();

	get_addresses();

  syslog(gateway_ip,(char *) "setup() complete");

#if WEBSERVER

  server.begin();

#endif

#endif

	psu.on();

  return;
}

/*
 * 
 */

void loop() {

  int              i;
  char             text[128], text2[8];
  uint32_t         run_msecs, run_secs;
  static uint32_t  last_display_update = 0, last_scan = 0;
#if LCD_DISPLAY
  int              len, col;
	static int       display_phase = 0;
#endif

  text[0] = text2[0] = i = 0; // Because I can't be bothered to sort out #if's

	//

  run_msecs = millis(); // run time
  run_secs  = run_msecs / 1000;

	if ((run_secs - last_scan) > 5) {

		psu.scan();

		last_scan = run_secs;
	}

#if defined(ARDUINO_ESP8266_WEMOS_D1MINI)

#endif

	//

	if ((display_enabled)&&(run_secs > 3)&&
			((run_msecs - last_display_update) > 50)) {

#if LCD_DISPLAY == 11

		switch (display_phase++) {

		case 0:

			break;

		case 1:
      for (i = 0; i < 16; ++i) {

        text[i] = ' ';
      }

      text[i] = 0;
			u8x8.drawString(0,1,text);
      break;

		case 2:

      sprintf(text,"%ds",run_secs);
      len = strlen(text);
      col = (len < 15) ? (16 - len) / 2:0;
      u8x8.drawString(col,2,text);
			break;

		case 3:
   
      break;

		case 4:
   
			break;

		case 5:
   
      break;

		case 6: // This is just for detecting memory leaks.

#if defined(ARDUINO_ESP8266_WEMOS_D1MINI)
      sprintf(text,"%-6u  %08x",ESP.getFreeHeap(),text);
#else
      sprintf(text,"%08x",text);
#endif
      u8x8.drawString(0,6,text);
      break;

    default:
    
			display_phase = 0;
      break;      
		}		

    u8x8.refreshDisplay();

#elif LCD_DISPLAY == 16

		switch (display_phase++) {

		case 0:
   
			break;

		case 1:

			sprintf(text,"%ds",run_secs);
			len = strlen(text);
			col = (len < 7) ? (8 - len) / 2:0;
			u8x8.drawString(col,1,text);
			break;

    case 2:

			u8x8.drawString(0,2,"+F-+IPPP");
      break;

    case 3:
 
			u8x8.drawString(0,3,"CFVVNGSF");
			break;

		case 4:

			for (i = 0; i < 8; ++i) {

				text[i] = ((psu.status_reg >> i) & 0x01) ? '1': '0';
			}

      text[i] = 0;

			u8x8.drawString(0,4,text);
			break;

		default:

#if defined(ARDUINO_ESP8266_WEMOS_D1MINI)
			sprintf(text,"%ddB",WiFi.RSSI());
      u8x8.drawString(1,5,text);
#endif
			display_phase = 0;
			break;
		}

    u8x8.refreshDisplay();

#endif	

		last_display_update = run_msecs;

	}

#if defined(ARDUINO_ESP8266_WEMOS_D1MINI) && WEBSERVER

	webserver();

#endif

  return;
}

/*
 *  Processor specific functions.
 */

#if defined(ARDUINO_ESP8266_WEMOS_D1MINI)

int connectWiFi() {

  int                status, i;
  static const char *ssid = "Woodmans Cottage",
	                  *password = "VHdMAB0pKxc67XEMcQVxNicSaKS4kgBqP4gyIcteZr9Mok0EkX959mlnSbO1Qe7";

	if ((status = WiFi.status()) != WL_CONNECTED) {

		WiFi.mode(WIFI_STA);

		WiFi.begin(ssid,password);

		for (i = 0; i < 20; ++i) {

			if ((status = WiFi.status()) == WL_CONNECTED) {

        udp.begin(2380);

				break;
			}

			delay(500);
		}
	}

	return status;
}

/*
 *
 */

#if WEBSERVER

void webserver() {

	int         i;
	char        text[128];
	const char *request_s;
  WiFiClient  client;
	String      request;
  static const char *http_header[] = {"HTTP/1.1 200 OK", "Content-Type: text/html",
																			"Connection: close", NULL},
                    *html_header[] = {"<html>", "<head>", "</head>", "<body>", NULL},
										*html_footer[] = {"</body", "</html>", NULL}, 
										*crlf = "\r\n", *on_s = "ON", *off_s = "OFF";

  if (!(client = server.available())) {

    return;
  }

  request   = client.readStringUntil('\r');
  request_s = request.c_str();

	if ((request_s[5] == 'o')&&(request_s[6] == 'n')) {

		psu.on();

	} else if ((request_s[5] == 'o')&&(request_s[6] == 'f')&&(request_s[7] == 'f')) {

		psu.standby();
	}

	for (i = 0; http_header[i]; ++i) {

		client.print(http_header[i]);
		client.print(crlf);
	}

	client.print(crlf);

	for (i = 0; html_header[i]; ++i) {

		client.print(html_header[i]);
		client.print("\n");
	}

	sprintf(text,"<h3 align=\"center\">%s</h3>\n",title);
	client.print(text);

	client.print("<table align=\"center\">\n");

	sprintf(text,"<tr><td>Output Over Current Protection</td><td>%s</td></tr>\n",(psu.OCP_12V) ? on_s: off_s);
	client.print(text);
  sprintf(text,"<tr><td>Fan Fault</td><td>%s</td></tr>\n",(psu.Fan_Fault) ? on_s: off_s);
  client.print(text);
  sprintf(text,"<tr><td>Output Under Voltage Protection</td><td>%s</td></tr>\n",(psu.UVP_12V) ? on_s: off_s);
  client.print(text);
  sprintf(text,"<tr><td>Output Over Voltage Protection</td><td>%s</td></tr>\n",(psu.OVP_12V) ? on_s: off_s);
  client.print(text);
  sprintf(text,"<tr><td>Vin Good</td><td>%s</td></tr>\n",(psu.Vin_Good) ? on_s: off_s);
  client.print(text);
  sprintf(text,"<tr><td>Power Outputs Good</td><td>%s</td></tr>\n",(psu.P_Good) ? on_s: off_s);
  client.print(text);
  sprintf(text,"<tr><td>Power Supply Status</td><td>%s</td></tr>\n",(psu.PS_Status) ? on_s: off_s);
  client.print(text);
  sprintf(text,"<tr><td>AC Pfail</td><td>%s</td></tr>\n",(psu.AC_Pfail) ? on_s: off_s);
  client.print(text);

	client.print("</table>\n");

  sprintf(text,"<!-- %s -->\n",build_date);
  client.print(text);
  sprintf(text,"<!-- %s -->\n",request_s);
  client.print(text);

	for (i = 0; html_footer[i]; ++i) {

		client.print(html_footer[i]);
		client.print("\n");
	}

	return;
}

#endif

/*
 *
 */

void get_addresses() {

  my_ip      = WiFi.localIP();
  gateway_ip = WiFi.gatewayIP(); 

  sprintf(my_ip_s,"%d.%d.%d.%d",my_ip[0],my_ip[1],my_ip[2],my_ip[3]);
  sprintf(gateway_s,"%d.%d.%d.%d",gateway_ip[0],gateway_ip[1],gateway_ip[2],gateway_ip[3]);

  return;
}

/*
 *
 */

int syslog(IPAddress server,char *message) {

	char       buffer[128];
  WiFiClient syslog;

	if (strlen(message) > 64) {

		message[64] = 0;
	}

  // <166> is local4, informational

	sprintf(buffer,"<166>1 ds550-3: %s",message);

	if (server) {

    udp.beginPacket(server,514);
    udp.print(buffer);
    udp.endPacket();    

    yield();

	}

	return 0;
}

#elif defined(ARDUINO_GENERIC_STM32F103C)

void yield(void) {

	return;
}

#endif

/*
 *
 */



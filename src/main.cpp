
#define _TASK_WDT_IDS

#include <Adafruit_NeoPixel.h>
#include <ArduinoOTA.h>
#include <ESP8266Wifi.h>
#include <ESP8266mDNS.h>
#include <TaskScheduler.h>
#include <WiFiUDP.h>
#include <string.h>
#include <arduino.h>

// ##################################################
#include <Arduino.h>
#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include "wire.h"

#include <ESP8266Wifi.h>
#include <WiFiUDP.h>
#include <ArduinoOTA.h>

#include <string.h>
// ##################################################

#define ledpin D4
#define numtasks 8
#define maxcolors 32
#define port 4100
#define commandlen 128

const char *ssid = "TRNNET-2G";
const char *password = "ripcord1";

const char *hostname = "ESP_TFT";

void parse_command();
void check_udp();
void t1_callback();

WiFiUDP Udp;
WiFiUDP Udp1;
WiFiServer Tcp(port);

Adafruit_NeoPixel strip = Adafruit_NeoPixel(numtasks, ledpin, NEO_GRB + NEO_KHZ800);
Scheduler runner;

Task *tasks[numtasks];

unsigned int numcolors[numtasks], currcolor[numtasks];
unsigned long colors[numtasks][maxcolors];
unsigned long steps[numtasks][maxcolors], currstep[numtasks];
unsigned char first[numtasks];
char command[commandlen + 1];

// ################################################################

WiFiServer Tcpt(1001);
WiFiClient client;

// For the Adafruit shield, these are the default.
#define TFT_DC D1
#define TFT_CS D8

// Use hardware SPI (on Uno, #13, #12, #11) and the above for CS/DC
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);
// If using the breakout, change pins as desired
// Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, TFT_MISO);

char buffer[81];
// ##################################################################

// ##################################################################
IRAM_ATTR void press()
{

  Udp.beginPacket("192.168.1.255", 3003);
  Udp.print("Press");
  Udp.endPacket();
}

void tft_bl()
{
  char *ptr;

  ptr = strtok(NULL, ", \r");
  if (ptr == NULL)
  {
    client.println("Error: BL state missing");
    return;
  }

  if (!strcasecmp(ptr, "on"))
    digitalWrite(D3, HIGH);
  else
    digitalWrite(D3, LOW);
}

void tft_clr()
{
  char *ptr;
  char colorbuf[3];
  int red, green, blue;

  ptr = strtok(NULL, ", \r");
  if (ptr == NULL)
  {
    client.println("Error: clr missing color");
    return;
  }

  strncpy(colorbuf, ptr, 2);
  red = strtol(colorbuf, NULL, 16);
  strncpy(colorbuf, ptr + 2, 2);
  green = strtol(colorbuf, NULL, 16);
  strncpy(colorbuf, ptr + 4, 2);
  blue = strtol(colorbuf, NULL, 16);

  tft.fillScreen(tft.color565(red, green, blue));
}

void tft_rot()
{
  char *ptr;
  int rot;

  ptr = strtok(NULL, ", \r");
  if (ptr == NULL)
  {
    client.println("Error: rot missing rotation");
    return;
  }

  rot = atoi(ptr);
  tft.setRotation(rot);

  // client.println("ok");
}

void tft_tcolor()
{
  char *ptr;
  char colorbuf[3];
  int red, green, blue;

  long fgcolor, bgcolor;

  ptr = strtok(NULL, ", \r");
  if (ptr == NULL)
  {
    client.println("Error: tcolor missing fg color");
    return;
  }

  strncpy(colorbuf, ptr, 2);
  red = strtol(colorbuf, NULL, 16);
  strncpy(colorbuf, ptr + 2, 2);
  green = strtol(colorbuf, NULL, 16);
  strncpy(colorbuf, ptr + 4, 2);
  blue = strtol(colorbuf, NULL, 16);

  fgcolor = tft.color565(red, green, blue);

  ptr = strtok(NULL, ", \r");
  if (ptr == NULL)
  {
    client.println("Error: tcolor missing bg color");
    return;
  }

  strncpy(colorbuf, ptr, 2);
  red = strtol(colorbuf, NULL, 16);
  strncpy(colorbuf, ptr + 2, 2);
  green = strtol(colorbuf, NULL, 16);
  strncpy(colorbuf, ptr + 4, 2);
  blue = strtol(colorbuf, NULL, 16);

  bgcolor = tft.color565(red, green, blue);

  tft.setTextColor(fgcolor, bgcolor);

  // client.println("ok");
}

void tft_text()
{
  char *ptr, *text;
  int size, xpos, ypos;

  ptr = strtok(NULL, ", \r");
  if (ptr == NULL)
  {
    client.println("Error: text missing size");
    return;
  }
  else
    size = atoi(ptr);

  ptr = strtok(NULL, ", \r");
  if (ptr == NULL)
  {
    client.println("Error: text missing xpos");
    return;
  }
  else
    xpos = atoi(ptr);

  ptr = strtok(NULL, ", \r");
  if (ptr == NULL)
  {
    client.println("Error: text missing ypos");
    return;
  }
  else
    ypos = atoi(ptr);

  ptr = strtok(NULL, ",\r");
  if (ptr == NULL)
  {
    client.println("Error: text missing text");
    return;
  }
  else
    text = ptr;

  // client.println("ok");

  tft.setTextSize(size);
  tft.setCursor(xpos, ypos);
  tft.print(text);
}

void tft_line()
{
  char *ptr;
  int x1, y1, x2, y2;

  char colorbuf[3];
  int red, green, blue;
  long color;

  ptr = strtok(NULL, ", \r");
  if (ptr == NULL)
  {
    client.println("Error: text missing x1");
    return;
  }
  else
    x1 = atoi(ptr);

  ptr = strtok(NULL, ", \r");
  if (ptr == NULL)
  {
    client.println("Error: text missing y1");
    return;
  }
  else
    y1 = atoi(ptr);

  ptr = strtok(NULL, ", \r");
  if (ptr == NULL)
  {
    client.println("Error: text missing x2");
    return;
  }
  else
    x2 = atoi(ptr);

  ptr = strtok(NULL, ", \r");
  if (ptr == NULL)
  {
    client.println("Error: text missing y2");
    return;
  }
  else
    y2 = atoi(ptr);

  ptr = strtok(NULL, ", \r");
  if (ptr == NULL)
  {
    client.println("Error: tcolor missing color");
    return;
  }

  strncpy(colorbuf, ptr, 2);
  red = strtol(colorbuf, NULL, 16);
  strncpy(colorbuf, ptr + 2, 2);
  green = strtol(colorbuf, NULL, 16);
  strncpy(colorbuf, ptr + 4, 2);
  blue = strtol(colorbuf, NULL, 16);

  color = tft.color565(red, green, blue);

  // client.println("ok");

  tft.drawLine(x1, y1, x2, y2, color);
}

void tft_rect()
{
  char *ptr;
  int x, y, w, h;
  int fill;

  char colorbuf[3];
  int red, green, blue;
  long color;

  ptr = strtok(NULL, ", \r");
  if (ptr == NULL)
  {
    client.println("Error: text missing x");
    return;
  }
  else
    x = atoi(ptr);

  ptr = strtok(NULL, ", \r");
  if (ptr == NULL)
  {
    client.println("Error: text missing y");
    return;
  }
  else
    y = atoi(ptr);

  ptr = strtok(NULL, ", \r");
  if (ptr == NULL)
  {
    client.println("Error: text missing width");
    return;
  }
  else
    w = atoi(ptr);

  ptr = strtok(NULL, ", \r");
  if (ptr == NULL)
  {
    client.println("Error: text missing height");
    return;
  }
  else
    h = atoi(ptr);

  ptr = strtok(NULL, ", \r");
  if (ptr == NULL)
  {
    client.println("Error: tcolor missing color");
    return;
  }

  strncpy(colorbuf, ptr, 2);
  red = strtol(colorbuf, NULL, 16);
  strncpy(colorbuf, ptr + 2, 2);
  green = strtol(colorbuf, NULL, 16);
  strncpy(colorbuf, ptr + 4, 2);
  blue = strtol(colorbuf, NULL, 16);

  color = tft.color565(red, green, blue);

  ptr = strtok(NULL, ", \r");
  if (ptr == NULL)
  {
    client.println("Error: text missing fill");
    return;
  }
  else
    fill = atoi(ptr);

  // client.println("ok");

  if (fill)
    tft.fillRect(x, y, w, h, color);
  else
    tft.drawRect(x, y, w, h, color);
}

void tft_circ()
{
  char *ptr;
  int x, y, r;
  int fill;

  char colorbuf[3];
  int red, green, blue;
  long color;

  ptr = strtok(NULL, ", \r");
  if (ptr == NULL)
  {
    client.println("Error: text missing x");
    return;
  }
  else
    x = atoi(ptr);

  ptr = strtok(NULL, ", \r");
  if (ptr == NULL)
  {
    client.println("Error: text missing y");
    return;
  }
  else
    y = atoi(ptr);

  ptr = strtok(NULL, ", \r");
  if (ptr == NULL)
  {
    client.println("Error: text missing radius");
    return;
  }
  else
    r = atoi(ptr);

  ptr = strtok(NULL, ", \r");
  if (ptr == NULL)
  {
    client.println("Error: tcolor missing color");
    return;
  }

  strncpy(colorbuf, ptr, 2);
  red = strtol(colorbuf, NULL, 16);
  strncpy(colorbuf, ptr + 2, 2);
  green = strtol(colorbuf, NULL, 16);
  strncpy(colorbuf, ptr + 4, 2);
  blue = strtol(colorbuf, NULL, 16);

  color = tft.color565(red, green, blue);

  ptr = strtok(NULL, ", \r");
  if (ptr == NULL)
  {
    client.println("Error: text missing fill");
    return;
  }
  else
    fill = atoi(ptr);

  // client.println("ok");

  if (fill)
    tft.fillCircle(x, y, r, color);
  else
    tft.drawCircle(x, y, r, color);
}

void check_net()
{
  char *ptr;

  int len;
  // static WiFiClient client;

  if (!client)
  {
    client = Tcpt.accept();
    // tft.fillScreen(ILI9341_BLACK);
    // tft.setCursor(0, 0);

    // tft.setTextSize(2);
    // tft.setTextColor(ILI9341_WHITE);
    // tft.print("TCP disconnected");
    // delay(2000);
  }

  if (client)
  {

    int numchars = client.available();
    if (numchars > 0)
    {
      Serial.print("Chars available:");
      Serial.println(numchars);

      len = client.readBytesUntil('\n', buffer, 80);
      if (buffer[len - 1] == '\r')
        len--;
      buffer[len] = '\0';

      Serial.print("buf:");
      Serial.println(buffer);

      ptr = strtok(buffer, ", \r");

      if (ptr == NULL)
        return;

      if (!strcasecmp(ptr, "clr"))
        tft_clr();
      else if (!strcasecmp(ptr, "text"))
        tft_text();
      else if (!strcasecmp(ptr, "tcolor"))
        tft_tcolor();
      else if (!strcasecmp(ptr, "rot"))
        tft_rot();
      else if (!strcasecmp(ptr, "line"))
        tft_line();
      else if (!strcasecmp(ptr, "rect"))
        tft_rect();
      else if (!strcasecmp(ptr, "circ"))
        tft_circ();
      else if (!strcasecmp(ptr, "bl"))
        tft_bl();

      else
      {
        client.printf("Error: invalid command %s\r\n", ptr);
        client.println("valid commands: clr,text,tcolor,rot,line,rect,circ");
      }
    }
  }
}
// ###################################################

void connect()
{
  // WiFi.mode(WIFI_AP);
  WiFi.mode(WIFI_STA);
  WiFi.setSleepMode(WIFI_NONE_SLEEP);

  WiFi.hostname(hostname);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println(WiFi.localIP());
  Udp.begin(port);
  Tcp.begin();

  ArduinoOTA.begin();

  // ####################################################
  Tcpt.begin();
  // ####################################################
}

void check_udp()
{

  int packetsize = Udp.parsePacket();
  if (packetsize)
  {

    memset(command, 0, commandlen + 1);
    Udp.read(command, commandlen);

    Serial.print("UDP: ");
    Serial.println(command);

    parse_command();
  }
}

void check_tcp()
{

  static WiFiClient client;
  static long unsigned noDataCnt;

  if (!client)
  {
    noDataCnt = 0;
    client = Tcp.available();
    if (client)
    {
      Serial.print("Connect:   ");
      Serial.println(client.remoteIP());
    }
  }
  else
  {
    int test = client.available();
    // Serial.print(client.remoteIP());
    // Serial.print("    ");
    // Serial.println(noDataCnt);
    if (test > 0)
    {
      noDataCnt = 0;
      memset(command, 0, commandlen + 1);
      client.readBytesUntil('\n', command, commandlen);

      Serial.print("TCP: ");
      Serial.println(command);
      parse_command();
    }
    else
    {
      noDataCnt++;
      // if(noDataCnt>5012)
      // client.stop(0);
    }
  }
}

void parse_command()
{

  unsigned ledId, pause;
  char *ptr, *ptr1;
  char *sav, *sav1;

  ptr = strtok_r(command, " ", &sav);
  ledId = atoi(ptr);
  (*tasks[ledId]).disable();

  currcolor[ledId] = 0;
  currstep[ledId] = 0;
  ptr = strtok_r(NULL, " ", &sav);

  if (strcmp(ptr, "+"))
  {
    numcolors[ledId] = 0;
    pause = atoi(ptr);
    (*tasks[ledId]).setInterval(pause);
  }

  while (ptr != NULL)
  {
    ptr = strtok_r(NULL, " ", &sav);
    if (ptr != NULL)
    {

      ptr = strupr(ptr);
      if (strchr(ptr, 'X') != NULL)
      {
        ptr1 = strtok_r(ptr, "X", &sav1);
        steps[ledId][numcolors[ledId]] = atol(ptr1);
        ptr1 = strtok_r(NULL, "X", &sav1);
      }
      else
      {
        steps[ledId][numcolors[ledId]] = 1;
        ptr1 = ptr;
      }
      colors[ledId][numcolors[ledId]] = strtol(ptr1, NULL, 16);
      numcolors[ledId]++;
    }
  }
  strip.setPixelColor(ledId, colors[ledId][currcolor[ledId]]);
  strip.show();
  first[ledId] = 1;
  (*tasks[ledId]).enable();
}

void t1_callback()
{
  int ledId;

  Task &taskRef = runner.currentTask();
  ledId = taskRef.getId();

  if (steps[ledId][currcolor[ledId]] != 0 and not first[ledId])
  {
    if (currstep[ledId] == steps[ledId][currcolor[ledId]] - 1)
    {

      if (currcolor[ledId] < numcolors[ledId] - 1)
        currcolor[ledId]++;
      else
        currcolor[ledId] = 0;

      currstep[ledId] = 0;
      strip.setPixelColor(ledId, colors[ledId][currcolor[ledId]]);
      strip.show();
    }
    else
      currstep[ledId]++;
  }
  first[ledId] = 0;
}

void setup(void)
{

  Serial.begin(9600);
  connect();
  ArduinoOTA.begin();

  for (int i = 0; i <= numtasks - 1; i++)
  {
    numcolors[i] = 1;
    currcolor[i] = 0;
    currstep[i] = 0;
    first[i] = 0;
    for (int j = 0; j <= maxcolors - 1; j++)
    {
      colors[i][j] = 0;
      steps[i][j] = 1;
    }
  }

  // ##################################################################################

  SPI.setFrequency(40000000);
  tft.begin();
  tft.fillScreen(ILI9341_BLACK);
  tft.setRotation(3);
  tft.setCursor(0, 0);

  tft.setTextColor(ILI9341_YELLOW);
  tft.setTextSize(2);
  tft.println(WiFi.localIP());

  delay(5000);
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_WHITE);

  pinMode(D3, OUTPUT);
  digitalWrite(D3, HIGH);

  pinMode(D2, INPUT);
  attachInterrupt(digitalPinToInterrupt(D2), press,RISING);
  // #################################################################################

  runner.init();
  for (int i = 0; i <= numtasks - 1; i++)
  {
    tasks[i] = new Task(500, TASK_FOREVER, &t1_callback);
    (*tasks[i]).setId(i);

    runner.addTask((*tasks[i]));
    (*tasks[i]).enable();
  }
  //pinMode(D5, INPUT_PULLUP);
  strip.begin();
}

void loop(void)
{
  runner.execute();
  ArduinoOTA.handle();

  // MDNS.update();

  check_udp();
  check_tcp();

  // #####################################################
  check_net();
  // #####################################################
}

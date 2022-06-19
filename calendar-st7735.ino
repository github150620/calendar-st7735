#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <FS.h>
#include <LittleFS.h>
#include <SPI.h>
#include <TZ.h>
#include <WiFiClient.h>

#define Y2K     946656000

#define TFT_CS  4
#define TFT_RST 16
#define TFT_DC  5

const char *html = \
  "<html>"\
  "<title>ESP8266</title>"
  "<body>"\
  "<form action=\"save\" method=\"post\">"\
  "<p>SSID: <input type=\"text\" name=\"ssid\" value=\"%s\"/></p>"\
  "<p>PASSWORD: <input type=\"password\" name=\"pass\" value=\"%s\"/></p>"\
  "<input type=\"submit\" value=\"Save\"/>"\
  "</form>"\
  "</body>"\
  "</html>";

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
ESP8266WebServer server(80);
WiFiClient client;

char ssid[32]     = "TP-LINK";
char password[64] = "12345678";

const int days[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
char buf[512];

void tft_init() {
  tft.initR(INITR_BLACKTAB);
  tft.setColRowStart(2,1);
  tft.setRotation(0);
  //tft.setRotation(2);
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(0, 0);
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
}

int load_config(char *_ssid, char *_password) {
  int n;
  
  File file = LittleFS.open("/wifi.cfg", "r");
  if (!file) {
    tft.println("Failed to open config file");
    return -1;
  }

  n = file.read((uint8_t *)buf, sizeof(buf));
  if (n <= 0) {
    tft.printf("Failed to read file");
    file.close();
    return -1;
  }
  
  sscanf(buf, "%s %s", _ssid, _password);
  tft.printf("SSID: %s\n", _ssid);
  tft.printf("PASS: %s\n", _password);
  file.close();
  return 0;
}

int save_config(char *_ssid, char *_password) {
  File file = LittleFS.open("/wifi.cfg", "w");
  if (!file) {
    Serial.println("Failed to open config file");
    return -1;
  }
  file.printf("%s %s", _ssid, _password);
  file.close();
}

int wifi_init() {
  int timeout;
  tft.print("WiFi.");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  timeout = 0;
  while (WiFi.status() != WL_CONNECTED) {
    if (timeout >= 30) {
      return -1;
    }
    tft.print(".");
    delay(1000);
    timeout++;
  }
  
  tft.println();
  tft.print("IP: ");
  tft.println(WiFi.localIP());

  return 0;
}

void ntp_init() {
  char buf[20];
  time_t t = 0;
  
  tft.print("NTP.");
  configTime(TZ_Asia_Shanghai, "cn.pool.ntp.org");
  while (t < Y2K) {
    tft.print(".");
    delay(1000);
    t = time(NULL);
  }
  tft.println();
  tft.print("Time: ");
  strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&t));
  tft.println(buf);
}

void print_time() {
  char buf[6];
  time_t t;
  struct tm * tt;

  t = time(NULL);
  tt = localtime(&t);
  strftime(buf, sizeof(buf), "%H:%M", tt);  

  tft.fillRect(6, 14, 116, 28, ST77XX_BLACK);
  tft.setTextSize(4);
  tft.setCursor(6, 14);
  tft.setTextColor(ST77XX_YELLOW);
  tft.print(buf);
}

void print_calender() {
  const uint16_t x = 0;
  const uint16_t y = 80;
  const uint16_t h = 13;

  time_t t;
  struct tm * tt;
  int first;
  char buf[12];

  t = time(NULL);
  tt = localtime(&t);

  tft.fillRect(x, y, 128, 80, ST77XX_BLACK);

  tft.setTextSize(2);
  tft.setCursor(4, 59);
  tft.setTextColor(ST77XX_ORANGE);
  strftime(buf, sizeof(buf), "%B", tt);
  tft.print(buf);

  tft.drawLine(4, 76, 123, 76, (0x0F<<11)|(0x0F<<6)|0x0F);

  tft.setTextSize(1);
  tft.setCursor(x + 18 * 0 + 7, y);
  tft.setTextColor(ST77XX_RED);
  tft.print("S");
  tft.setCursor(x + 18 * 1 + 7, y);
  tft.setTextColor(ST77XX_WHITE);
  tft.print("M");
  tft.setCursor(x + 18 * 2 + 7, y);
  tft.print("T");
  tft.setCursor(x + 18 * 3 + 7, y);
  tft.print("W");
  tft.setCursor(x + 18 * 4 + 7, y);
  tft.print("T");
  tft.setCursor(x + 18 * 5 + 7, y);
  tft.print("F");
  tft.setCursor(x + 18 * 6 + 7, y);
  tft.setTextColor(ST77XX_RED);
  tft.print("S");
  
  first = (tt->tm_wday - tt->tm_mday % 7 + 1) % 7;
  if (first < 0) {
    first += 7;
  }
  tft.drawRect(x + 18 * ((first + tt->tm_mday - 1) % 7) + 4 - 2, y + h + h * ((first + tt->tm_mday - 1) / 7) - 2, 15, 11, ST77XX_WHITE);

  for (int i=0;i<days[tt->tm_mon];i++) {
    tft.setCursor(x + 18 * ((first + i) % 7) + 4, y + h + h * ((first + i) / 7));
    sprintf(buf, "%02d", i + 1);
    if (i + 1 >= tt->tm_mday) {
      if ((first + i) % 7 == 0 || (first + i) % 7 == 6) {
        tft.setTextColor(ST77XX_RED);
      } else {
        tft.setTextColor(ST77XX_WHITE);  
      }
    } else {
      tft.setTextColor((0x0F<<11)|(0x0F<<6)|0x0F);
    }
    tft.print(buf);
  }
}

void print_img(u8 pos_x, u8 pos_y, const u16 *img) {
  for (int x=0;x<img[0];x++) {
    for (int y=0;y<img[1];y++) {
      tft.drawPixel(pos_x+x, pos_y+y, img[2+y*img[0]+x]);
    }
  }
}

void handleRoot() {
  sprintf(buf, html, ssid, password);
  server.send(200, "text/html", buf);
}

void handleSave() {
  if (server.method() != HTTP_POST) {
    server.send(405, "text/plain", "Method Not Allowed");
    return;
  }

  for (int i=0;i<server.args();i++) {
    if (server.argName(i) == "ssid") {
      strncpy(ssid, server.arg(i).c_str(), sizeof(ssid));
    }

    if (server.argName(i) == "pass") {
      strncpy(password, server.arg(i).c_str(), sizeof(password));      
    }
  }

  if (strlen(ssid) == 0 || strlen(password) == 0) {
    server.send(400, "text/plain", "Bad request");
    return;
  }

  save_config(ssid, password);
  server.send(200, "text/plain", "Success. Please restart manually!");
}

void configServer() {
  char *ap_ssid = "ESP8266";
  
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextSize(1);
  tft.setCursor(0, 0);  
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ap_ssid, "");
  server.on("/", handleRoot);
  server.on("/save", handleSave);
  server.begin();
  tft.println("------ Config -------");
  tft.println("SSID:");
  tft.println(ap_ssid);
  tft.println();
  tft.println("URL:");
  tft.printf("http://");
  tft.println(WiFi.softAPIP());
  while (1) {
    server.handleClient();
  }
}

void setup() {
  Serial.begin(115200);
  LittleFS.begin();
  tft_init();
  load_config(ssid, password);
  if (wifi_init() == -1) {
    configServer();
  }
  
  ntp_init();
  tft.fillScreen(ST77XX_BLACK);
}

void loop() {
  static time_t next1 = 0;
  static time_t next2 = 0;
  
  time_t t;

  t = time(NULL);
  if (t > next1) {
    next1 = (t / 60) * 60 + 60;
    print_time();
  }

  if (t > next2) {
    next2 = (t / 86400) * 86400 + 86400;
    print_calender();
  }
  
  delay(1000);
}

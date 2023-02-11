//
// ESPSU v1
//
// ESP8266 D1 Mini Based
//   Modified ATX PSU Controller
//   GPIO Button for manual PC power-on and -off
//   IR Remote
//   Physical Button (IRBUTTON) for easy power-on of TV
//
//

/*--------       Libraries        ----------*/

#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>


/*--------    WiFi Credentials    ----------*/

#ifndef STASSID
#define STASSID       "YOUR_WIFI_SSID_HERE"  // WiFi SSID
#define STAPSK        "YOUR_WIFI_PASS_HERE"  // WiFi Password
#endif
#define WiFiHostname  "ESPSU"                // WiFi Hostname


/*-------- D1 Mini GPIO Reference ----------*/

static const uint8_t D1D0   = 16;
static const uint8_t D1D1   = 5;
static const uint8_t D1D2   = 4;
static const uint8_t D1D3   = 0;
static const uint8_t D1D4   = 2;
static const uint8_t D1D5   = 14;
static const uint8_t D1D6   = 12;
static const uint8_t D1D7   = 13;
static const uint8_t D1D8   = 15;
static const uint8_t D1D9   = 3;
static const uint8_t D1D10  = 1;


/*--------          GPIO          ----------*/

// PSU GPIO
#define PSU_ENABLE_PIN      4    // D2
#define PSU_STATUS_PIN      5    // D1 - D3 cant be low at boot
#define PSU_STATUS_LED      2    // D4 - Built In LED

// IR GPIO
#define IRBUTTON            14   // D5
const uint16_t IRLED =      13;  // D7

// Power Button
#define GPIOBUTTON          12   // D6


/*---------------- IR Codes ----------------*/
/*-- *All the remotes exist in one array* --*/

// TCL Series 4 Roku Remote - 0           0           1           2           3           4           5           6           7           8           9           10          11          12          13          14          15
//                                        Power       Back        Home        Up          Left        Down        Right       Ok          Jump Back?  */Option    Rewind      Play/Pause  Fast Fwd    Vol Up      Vol Down    Mute
static const uint64_t remotes[10][28] = { {0x57E3E817, 0x57E36699, 0x57E3C03F, 0x57E39867, 0x57E37887, 0x57E3CC33, 0x57E3B44B, 0x57E354AB, 0x57E31EE1, 0x57E38679, 0x57E32CD3, 0x57E332CD, 0x57E3AA55, 0x57E3F00F, 0x57E308F7, 0x57E304FB},
// Sony RM-YD023 & RM-YD092 - 1           0      1      2      3      4      5     6      7      8      9      10     11     12     13     14    15     16     17     18     19    20     21     22     23     24     25     26     27
//   12 bit                               Power  Input  PicOff Disp   Sleep  Home  Pict   Up     Left   Down   Right  Ok     Vol +  Vol -  Ch +  Ch -   Jump   Mute   0      1     2      3      4      5      6      7      8      9
                                         {0xA90, 0xA50, 0x7D0, 0x5D0, 0x6D0, 0x70, 0x270, 0x2F0, 0x2D0, 0xAF0, 0xCD0, 0xA70, 0x490, 0xC90, 0x90, 0x890, 0xDD0, 0x290, 0x910, 0x10, 0x810, 0x410, 0xC10, 0x210, 0xA10, 0x610, 0xE10, 0x110},
// Sony RM-YD023 & RM-YD092 - 2           0      1       2       3       4       5       6       7       8       9       10     11      12      13      14      15      16
//   15 bit                               SyncMu Wide    Option  Return  Rwd     FF      Play    Pause   Prev    Next    Stop   Yellow  Blue    Red     Green   .       CC
                                         {0xD58, 0x5E25, 0x36E9, 0x62E9, 0x6CE9, 0x1CE9, 0x2CE9, 0x4CE9, 0x1EE0, 0x5EE9, 0xCE9, 0x72E9, 0x12E9, 0x52E9, 0x32E9, 0x5CE9, 0x425},
// 3x2 HDMI Matrix Switch   - 3           0          1          2          3          4          5
//   Codes overlap with Arizer            Out A: 1   2          3          Out B1     2          3
                                         {0x1FE48B7, 0x1FE807F, 0x1FE20DF, 0x1FE7887, 0x1FEC03F, 0x1FE609F},
// Arizer                   - 4           0          1          2          3          4          5          6          7          8          9          10         11         12         13         14         15         16         17
//                                        Power      Fan Off    1          2          3          Temp +     Temp -     50C        100C       200C       210C       220C       230C       Timer 0hr  2hr        4hr        Audio      Light
                                         {0x1FE7887, 0x1FE10EF, 0x1FE20DF, 0x1FEA05F, 0x1FE609F, 0x1FED827, 0x1FEB04F, 0x1FEE01F, 0x1FE906F, 0x1FE50AF, 0x1FEF807, 0x1FE30CF, 0x1FE708F, 0x1FE807F, 0x1FE40BF, 0x1FEC03F, 0x1FE48B7, 0x1FE58A7},
// Philips PicoPix Max      - 5           0           1           2           3           4           5           6           7           8           9           10          11          12
//   Mouse button doesn't do anything     Power       Mouse       Up          Left        Down        Right       Ok          Back        Menu        Home        Vol -       Vol +       Auto Focus
                                         {0x40BF48B7, 0x00000000, 0x40BFF807, 0x40BFF00F, 0x40BF827D, 0x40BF906F, 0x40BF22DD, 0x40BF0AF5, 0x40BFC837, 0x40BF40BF, 0x40BF609F, 0x40BF00FF, 0x40BFC03F},
// 4x2 HDMI Matrix Switch   - 6           0          1          2          3          4          5
//   Codes overlap with Arizer            Power      Out A: 1   2          3          4          Out B1     2          3          4
                                         {0x1FE48B7, 0x1FE807F, 0x1FEC03F, 0x1FE20DF, 0x1FE609F, 0x1FEE01F, 0x1FE906F, 0x1FE50AF, 0x1FEF807},
// Sony RMT-AA401U          - 7           0       1      2       3       4       5       6      7       8       9       10     11      12      13      14     15      16      17      18      19      20      21      22
//   15 bit                               Power   Sleep  SAT     GAME    CD      TV      FM     2Ch     Movie   Music   Night  Dimmer  Disp    AmpMenu Up     Down    Left    Right   Ok      Home    Mute    Vol +   Vol -
                                         {0x540C, 0x30C, 0x600D, 0x1F0C, 0x520C, 0x2B0C, 0xC0C, 0x710D, 0x610D, 0x490D, 0x20D, 0x590C, 0x690C, 0x770D, 0xF0D, 0x4F0D, 0x2F0D, 0x6F0D, 0x180C, 0x650C, 0x140C, 0x240C, 0x640C},
// Sony RMT-AA401U          - 8           0        1        2       3        4        5        6        7        8        9        10       11       12      13       14
//   20 bit                               Info     BlueT    Media   BD       BT Pair  PureDir  Frt Sur  Back     Options  RW/Tun-  Play/Mem FF/Tun+  Prev    Stop     Next
                                         {0x18110, 0x8E114, 0x2116, 0x68114, 0xEE114, 0x9E114, 0x9C116, 0xBE110, 0xCE110, 0xCC110, 0x5C110, 0x2C110, 0xC110, 0x1C110, 0x8C110},
// Macros                   - 9           0  1  2
//                              TCL Input 1  2  3
                                         {0, 1, 2}
};


/*--------  Program Variables  ----------*/

const char * ssid = STASSID;
const char * pass = STAPSK;
ESP8266WebServer server(80);

IRsend irsend(IRLED);  // Set the GPIO to be used to sending IR

bool buttonState = false;


/*--------    Main Functions   ----------*/

void setup() {
  // Setup PSU GPIO
  // Bring enable pin LOW to turn off PSU
  pinMode(PSU_ENABLE_PIN, OUTPUT);
  digitalWrite(PSU_ENABLE_PIN, LOW);
  // Bring internal LED pin HIGH to turn off
  pinMode(PSU_STATUS_LED, OUTPUT);
  digitalWrite(PSU_STATUS_LED, HIGH);
  // PSU_STATUS_PIN reads HIGH when on
  pinMode(PSU_STATUS_PIN, INPUT_PULLUP);
  
  // Setup GPIO Button (useful for turning on servers manually)
  pinMode(GPIOBUTTON, OUTPUT);
  digitalWrite(GPIOBUTTON, LOW);
  
  // Initialize IR and IRBUTTON
  irsend.begin();
  pinMode(IRBUTTON, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(IRBUTTON), buttonWatcher, FALLING);
  
  // Connect to WiFi
  connectWiFi();

  // Start HTML Server
  serverSetup();
}

ICACHE_RAM_ATTR void buttonWatcher() {
  buttonState = !buttonState;
}

void loop() {
  // When PSU_STATUS_PIN is HIGN, LED turns on (LOW)
  digitalWrite(PSU_STATUS_LED, !digitalRead(PSU_STATUS_PIN));
  
  // Button Trigger
  if (buttonState) {
    irsend.sendNEC(remotes[0][0]);
    buttonState = false;
  }
  
  // Webserver
  server.handleClient();
  
  // Let the ESP8266 do its thing
  yield();
}


/*--------      WiFi Code      ----------*/

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.hostname(WiFiHostname);
  WiFi.begin(ssid, pass);

  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - startTime) <= 10000) // Timeout WiFi connection attempt after 10 sec
  {
    delay(500);
  }
}


/*--------    PSU Functions    ----------*/

void txIR() {
  int a = server.arg("remoteNumber").toInt();
  int i = server.arg("irCode").toInt();
  uint64_t irCodeToTX = remotes[a][i];
  
  switch ( a ) {
    case 0:                              // TCL Series 4 Roku Remote
      irsend.sendNEC(irCodeToTX);
      break;
    
    case 1:                              // Sony RM-YD023 & RM-YD092 12 bit
      irsend.sendSony(irCodeToTX, 12, 2);
      break;
    
    case 2:                              //  Sony RM-YD023 & RM-YD092 15 bit
      irsend.sendSony(irCodeToTX, 15, 2);
      break;
    
    case 3:                              // 3x2 HDMI Matrix Switch
      irsend.sendNEC(irCodeToTX);
      break;
    
    case 4:                              // Arizer Extreme Q
      irsend.sendNEC(irCodeToTX);
      break;
    
    case 5:                              // Philips PicoPix Max
      irsend.sendNEC(irCodeToTX);
      break;
    
    case 6:                              // 4x2 HDMI Matrix Switch
      irsend.sendNEC(irCodeToTX);
      break;
    
    case 7:                              // Sony RMT-AA401U 15 bit
      irsend.sendSony(irCodeToTX, 15, 2);
      break;
    
    case 8:                              // Sony RMT-AA401U 20 bit
      irsend.sendSony(irCodeToTX, 20, 2);
      break;
    
    case 9:                              // Macros
      switch ( irCodeToTX ) {
        case 0:                          // TCL Input 1
          irsend.sendNEC(remotes[0][2]);
          delay(1500);
          irsend.sendNEC(remotes[0][6]);
          delay(100);
          irsend.sendNEC(remotes[0][7]);
          break;

        case 1:                          // TCL Input 2
          irsend.sendNEC(remotes[0][2]);
          delay(1500);
          irsend.sendNEC(remotes[0][6]);
          delay(100);
          irsend.sendNEC(remotes[0][6]);
          delay(100);
          irsend.sendNEC(remotes[0][7]);
          break;

        case 2:                          // TCL Input 3
          irsend.sendNEC(remotes[0][2]);
          delay(1500);
          irsend.sendNEC(remotes[0][6]);
          delay(100);
          irsend.sendNEC(remotes[0][6]);
          delay(100);
          irsend.sendNEC(remotes[0][6]);
          delay(100);
          irsend.sendNEC(remotes[0][7]);
          break;
      }
      break;
  }
  redirect();
}

void toggle() {
  // Toggle the PSU power state
  digitalWrite(PSU_ENABLE_PIN, !digitalRead(PSU_ENABLE_PIN));
  redirect();
}

void buttonPress() {
  // Press the GPIOBUTTON for .1 sec
  digitalWrite(GPIOBUTTON, HIGH);
  unsigned long currentMillis = millis();
  while(currentMillis + 100 > millis()) {
    yield();
  }
  digitalWrite(GPIOBUTTON, LOW);
  redirect();
}

void buttonPressTen() {
  // Press the GPIOBUTTON for 10 sec
  digitalWrite(GPIOBUTTON, HIGH);
  unsigned long currentMillis = millis();
  while(currentMillis + 10000 > millis()) {
    yield();
  }
  digitalWrite(GPIOBUTTON, LOW);
  redirect();
}


/*-------- Webserver Functions ----------*/

void serverSetup() {
  server.on("/", handleRoot);
  server.on("/txIR", HTTP_GET, txIR);
  server.on("/toggle", HTTP_GET, toggle);
  server.on("/buttonPress", HTTP_GET, buttonPress);
  server.on("/buttonPressTen", HTTP_GET, buttonPressTen);
  server.onNotFound(handleNotFound);
  server.begin();
}

String webpage = ""
                 "<!DOCTYPE html>"
                 "<html>"
                 "<head>"
                   "<title>ESPSU</title>"
                   "<meta name=\"mobile-web-app-capable\" content=\"yes\" />"
                   "<meta name=\"viewport\" content=\"width=device-width\" />"
                   "<style>"
                     "body {background-color: #000; color: #a40; font-family: Helvetica;}"
                     "p { font-size: 1.25em; }"
                     "input.colorButton { width: 100%; height: 2.5em; padding: 0; font-size: 2em; background-color: #222; border-color: #222; color: #a40; font-family: Helvetica;} "
                     "button.myButton { width: 33.33%; height: 2.5em; padding: 0; position: relative; font-size: 2em; background-color: #222; border-color: #222; color: #a40; font-family: Helvetica; } "
                     ".tabs { position: relative; min-height: 200px; clear: both; margin: 25px 0; }"
                     ".tab { float: left; }"
                     ".tab label { background: #eee; padding: 10px; border: 3px solid #000; margin-left: -1px; position: relative; left: 1px; font-size: 1.25em; background-color: #222; }"
                     ".tab [type=radio] { display: none; }"
                     ".content { position: absolute; top: 33px; left: 0; background: white; right: 0; padding: 20px; border: 0px solid #222; background-color: #111; height: 47em; }"
                     "[type=radio]:checked ~ label { background: #111; border-bottom: 0px solid #333; z-index: 2; }"
                     "[type=radio]:checked ~ label ~ .content { z-index: 1; }"
                   "</style>"
                 "</head>"
                 "<body>"
                   "<div class=\"tabs\">"
                     "<div class=\"tab\">"
                       "<input type=\"radio\" id=\"tab-0\" name=\"tab-group-1\" checked>"
                       "<label for=\"tab-0\">Favorites</label>"
                       "<div class=\"content\">"
                         "<p>TCL TV</p>"
                         "<form action=\"/txIR\" method=\"GET\">"
                           "<input type=\"hidden\" name=\"remoteNumber\" value=\"0\"></input>"
                           "<button name=\"irCode\" class=\"myButton\" onclick=\"this.form.submit()\" value=\"0\" style=\"width: 100%;\">Power</button><br />" // Power &#9211;
                         "</form>"
                         "<form action=\"/txIR\" method=\"GET\">"
                           "<input type=\"hidden\" name=\"remoteNumber\" value=\"9\"></input>"
                           "<button name=\"irCode\" class=\"myButton\" onclick=\"this.form.submit()\" value=\"0\">In 1</button>"
                           "<button name=\"irCode\" class=\"myButton\" onclick=\"this.form.submit()\" value=\"1\">In 2</button>"
                           "<button name=\"irCode\" class=\"myButton\" onclick=\"this.form.submit()\" value=\"2\">In 3</button>"
                         "</form>"
                         "<p>Sony TV</p>"
                         "<form action=\"/txIR\" method=\"GET\">"
                           "<input type=\"hidden\" name=\"remoteNumber\" value=\"1\"></input>"
                           "<button name=\"irCode\" class=\"myButton\" onclick=\"this.form.submit()\" value=\"0\" style=\"width: 50%;\">Power</button>"
                           "<button name=\"irCode\" class=\"myButton\" onclick=\"this.form.submit()\" value=\"1\" style=\"width: 50%;\">Input</button>"
                         "</form>"
                         "<p>Arizer</p>"
                         "<form action=\"/txIR\" method=\"GET\">"
                           "<input type=\"hidden\" name=\"remoteNumber\" value=\"4\"></input>"
                           "<button name=\"irCode\" class=\"myButton\" onclick=\"this.form.submit()\" value=\"6\">-</button>"
                           "<button name=\"irCode\" class=\"myButton\" onclick=\"this.form.submit()\" value=\"0\">Power</button>"
                           "<button name=\"irCode\" class=\"myButton\" onclick=\"this.form.submit()\" value=\"5\">+</button>"
                           "<button name=\"irCode\" class=\"myButton\" onclick=\"this.form.submit()\" value=\"1\" style=\"width: 50%;\">Fan 0</button>"
                           "<button name=\"irCode\" class=\"myButton\" onclick=\"this.form.submit()\" value=\"3\" style=\"width: 50%;\">Fan 2</button>"
                         "</form>"
                       "</div>"
                     "</div>"
                     "<div class=\"tab\">"
                       "<input type=\"radio\" id=\"tab-1\" name=\"tab-group-1\">"
                       "<label for=\"tab-1\">ESPSU</label>"
                       "<div class=\"content\">"
                         "<form action=\"/toggle\" method=\"GET\"><input type=\"submit\" value=\"Turn PSU %toggleStub%\" class=\"colorButton\"></form><br />"
                         "<form action=\"/buttonPress\" method=\"GET\"><input type=\"submit\" value=\"Press Button\" class=\"colorButton\"></form>"
                         "<form action=\"/buttonPressTen\" method=\"GET\"><input type=\"submit\" value=\"Press Button 10s\" class=\"colorButton\"></form>"
                       "</div>"
                     "</div>"
                     "<div class=\"tab\">"
                       "<input type=\"radio\" id=\"tab-2\" name=\"tab-group-1\">"
                       "<label for=\"tab-2\">TCL TV</label>"
                       "<div class=\"content\">"
                         "<form action=\"/txIR\" method=\"GET\">"
                           "<input type=\"hidden\" name=\"remoteNumber\" value=\"0\"></input>"
                           "<button name=\"irCode\" class=\"myButton\" onclick=\"this.form.submit()\" value=\"0\" style=\"width: 100%;\">Power</button><br />" // Power &#9211;
                         "</form>"
                         "<form action=\"/txIR\" method=\"GET\">"
                           "<input type=\"hidden\" name=\"remoteNumber\" value=\"9\"></input>"
                           "<button name=\"irCode\" class=\"myButton\" onclick=\"this.form.submit()\" value=\"0\">In 1</button>"
                           "<button name=\"irCode\" class=\"myButton\" onclick=\"this.form.submit()\" value=\"1\">In 2</button>"
                           "<button name=\"irCode\" class=\"myButton\" onclick=\"this.form.submit()\" value=\"2\">In 3</button>"
                         "</form>"
                         "<form action=\"/txIR\" method=\"GET\">"
                           "<input type=\"hidden\" name=\"remoteNumber\" value=\"0\"></input>"
                           "<button name=\"irCode\" class=\"myButton\" onclick=\"this.form.submit()\" value=\"1\">&#60;</button>" // Back
                           "<button name=\"irCode\" class=\"myButton\" onclick=\"this.form.submit()\" value=\"3\">&#8593;</button>" // Up
                           "<button name=\"irCode\" class=\"myButton\" onclick=\"this.form.submit()\" value=\"2\">Home</button>" // Home
                           "<button name=\"irCode\" class=\"myButton\" onclick=\"this.form.submit()\" value=\"4\">&#8592;</button>" // Left
                           "<button name=\"irCode\" class=\"myButton\" onclick=\"this.form.submit()\" value=\"7\">OK</button>" // Ok
                           "<button name=\"irCode\" class=\"myButton\" onclick=\"this.form.submit()\" value=\"6\">&#8594;</button>" // Right
                           "<button name=\"irCode\" class=\"myButton\" onclick=\"this.form.submit()\" value=\"8\">Jump</button>" // Jump
                           "<button name=\"irCode\" class=\"myButton\" onclick=\"this.form.submit()\" value=\"5\">&#8595;</button>" // Down
                           "<button name=\"irCode\" class=\"myButton\" onclick=\"this.form.submit()\" value=\"9\">&#42;</button>" // Option
                           "<button name=\"irCode\" class=\"myButton\" onclick=\"this.form.submit()\" value=\"10\">Rwd</button>" // Rewind
                           "<button name=\"irCode\" class=\"myButton\" onclick=\"this.form.submit()\" value=\"11\">P/P</button>" // Play/Pause
                           "<button name=\"irCode\" class=\"myButton\" onclick=\"this.form.submit()\" value=\"12\">FF</button>" // Fast Forward
                           "<button name=\"irCode\" class=\"myButton\" onclick=\"this.form.submit()\" value=\"15\">Mute</button>" // Mute
                           "<button name=\"irCode\" class=\"myButton\" onclick=\"this.form.submit()\" value=\"14\">Vol -</button>" // Vol -
                           "<button name=\"irCode\" class=\"myButton\" onclick=\"this.form.submit()\" value=\"13\">Vol +</button>" // Vol +
                         "</form>"
                       "</div>"
                     "</div>"
                     "<div class=\"tab\">"
                       "<input type=\"radio\" id=\"tab-3\" name=\"tab-group-1\">"
                       "<label for=\"tab-3\">Arizer</label>"
                       "<div class=\"content\">"
                         "<form action=\"/txIR\" method=\"GET\">"
                           "<input type=\"hidden\" name=\"remoteNumber\" value=\"4\"></input>"
                           "<button name=\"irCode\" class=\"myButton\" onclick=\"this.form.submit()\" value=\"0\" style=\"left: 33.3%; position: relative;\">Power</button><br />"
                           "<button name=\"irCode\" class=\"myButton\" onclick=\"this.form.submit()\" value=\"2\">Fan 1</button>"
                           "<button name=\"irCode\" class=\"myButton\" onclick=\"this.form.submit()\" value=\"3\">Fan 2</button>"
                           "<button name=\"irCode\" class=\"myButton\" onclick=\"this.form.submit()\" value=\"4\">Fan 3</button>"
                           "<button name=\"irCode\" class=\"myButton\" onclick=\"this.form.submit()\" value=\"6\">-</button>"
                           "<button name=\"irCode\" class=\"myButton\" onclick=\"this.form.submit()\" value=\"1\">Fan 0</button>"
                           "<button name=\"irCode\" class=\"myButton\" onclick=\"this.form.submit()\" value=\"5\">+</button>"
                           "<button name=\"irCode\" class=\"myButton\" onclick=\"this.form.submit()\" value=\"7\">50C</button>"
                           "<button name=\"irCode\" class=\"myButton\" onclick=\"this.form.submit()\" value=\"8\">100C</button>"
                           "<button name=\"irCode\" class=\"myButton\" onclick=\"this.form.submit()\" value=\"9\">200C</button>"
                           "<button name=\"irCode\" class=\"myButton\" onclick=\"this.form.submit()\" value=\"10\">210C</button>"
                           "<button name=\"irCode\" class=\"myButton\" onclick=\"this.form.submit()\" value=\"11\">220C</button>"
                           "<button name=\"irCode\" class=\"myButton\" onclick=\"this.form.submit()\" value=\"12\">230C</button>"
                           "<button name=\"irCode\" class=\"myButton\" onclick=\"this.form.submit()\" value=\"13\">0hr</button>"
                           "<button name=\"irCode\" class=\"myButton\" onclick=\"this.form.submit()\" value=\"14\">2hr</button>"
                           "<button name=\"irCode\" class=\"myButton\" onclick=\"this.form.submit()\" value=\"15\">4hr</button>"
                           "<button name=\"irCode\" class=\"myButton\" onclick=\"this.form.submit()\" value=\"16\" style=\"width: 50%;\">Audio</button>"
                           "<button name=\"irCode\" class=\"myButton\" onclick=\"this.form.submit()\" value=\"17\" style=\"width: 50%;\">Light</button>"
                         "</form>"
                       "</div>"
                     "</div>"
                     "<div class=\"tab\">"
                       "<input type=\"radio\" id=\"tab-4\" name=\"tab-group-1\">"
                       "<label for=\"tab-4\">Sony</label>"
                       "<div class=\"content\">"
                         "<form action=\"/txIR\" method=\"GET\">"
                           "<input type=\"hidden\" name=\"remoteNumber\" value=\"1\"></input>"
                           "<button name=\"irCode\" class=\"myButton\" onclick=\"this.form.submit()\" value=\"0\" style=\"width: 50%;\">Power</button>"
                           "<button name=\"irCode\" class=\"myButton\" onclick=\"this.form.submit()\" value=\"1\" style=\"width: 50%;\">Input</button>"
                           "<button name=\"irCode\" class=\"myButton\" onclick=\"this.form.submit()\" value=\"12\">Vol +</button>"
                           "<button name=\"irCode\" class=\"myButton\" onclick=\"this.form.submit()\" value=\"7\">&#8593;</button>" // Up
                           "<button name=\"irCode\" class=\"myButton\" onclick=\"this.form.submit()\" value=\"14\">CH +</button>"
                           "<button name=\"irCode\" class=\"myButton\" onclick=\"this.form.submit()\" value=\"8\">&#8592;</button>" // Left
                           "<button name=\"irCode\" class=\"myButton\" onclick=\"this.form.submit()\" value=\"11\">OK</button>"
                           "<button name=\"irCode\" class=\"myButton\" onclick=\"this.form.submit()\" value=\"10\">&#8594;</button>" // Right
                           "<button name=\"irCode\" class=\"myButton\" onclick=\"this.form.submit()\" value=\"13\">Vol -</button>"
                           "<button name=\"irCode\" class=\"myButton\" onclick=\"this.form.submit()\" value=\"9\">&#8595;</button>" // Down
                           "<button name=\"irCode\" class=\"myButton\" onclick=\"this.form.submit()\" value=\"15\">CH -</button>"
                           "<button name=\"irCode\" class=\"myButton\" onclick=\"this.form.submit()\" value=\"17\">Mute</button>"
                           "<button name=\"irCode\" class=\"myButton\" onclick=\"this.form.submit()\" value=\"5\">Home</button>"
                           "<button name=\"irCode\" class=\"myButton\" onclick=\"this.form.submit()\" value=\"16\">Jump</button>"
                         "</form>"
                       "</div>"
                     "</div>"
                   "</div>"
                 "</body>"
                 "</html>";

void handleRoot() {
  String deliveredHTML = webpage;
  
  // If the PSU is on (HIGH), the webpage will show the option to turn it off
  if (digitalRead(PSU_STATUS_PIN) == HIGH) {
    deliveredHTML.replace("%toggleStub%", "Off");
  }
  else {
    deliveredHTML.replace("%toggleStub%", "On");
  }
  
  server.send(200, "text/html", deliveredHTML);
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void redirect() {
  // Sends the client back to the index after running
  server.sendHeader("Location", "/");
  server.send(303);
}

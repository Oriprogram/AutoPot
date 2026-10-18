#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <XPT2046_Touchscreen.h>
#include "faces.h"

// hardware pins
#define RELAY_PUMP_PIN   22
#define RELAY_LIGHT_PIN  27
#define SOIL_PIN         35
#define LDR_PIN          34
#define TFT_BL           21

// display spi
#define TFT_CS    15
#define TFT_DC     2
#define TFT_MOSI  13
#define TFT_CLK   14
#define TFT_RST   -1
#define TFT_MISO  12

// touch spi
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK  25
#define XPT2046_CS   33

SPIClass tftSPI = SPIClass(HSPI);
Adafruit_ILI9341 tft = Adafruit_ILI9341(&tftSPI, TFT_DC, TFT_CS, TFT_RST);

SPIClass touchSPI = SPIClass(VSPI);
XPT2046_Touchscreen ts(XPT2046_CS);

// network & firebase
const char* FIREBASE_HOST = "https://the-autopot-default-rtdb.firebaseio.com";
const char* DEVICE_ID     = "POT-001";

Preferences prefs;
WebServer server(80);
DNSServer dnsServer;
const byte DNS_PORT = 53;

bool isPortalActive = false;
String savedSSID = "";
String savedPass = "";

// face states
enum UIView { VIEW_TAMAGOTCHI, VIEW_SETTINGS, VIEW_PROFILES, VIEW_STATS, VIEW_WIFI_INFO, VIEW_PORTAL };
UIView currentView = VIEW_TAMAGOTCHI;

enum FaceState { FACE_NONE, FACE_HAPPY, FACE_THIRSTY, FACE_PET, FACE_ANGRY, FACE_SLEEP };
FaceState currentFace = FACE_NONE;

// pot settings/thersholds
int targetMoisture    = 35;
int dailyLightHours   = 10;
bool isManualMode     = false;
bool manualLightState = false;
String currentPlantMode = "CUSTOM";

bool controlsNeedPush = false; 

// timerssss
unsigned long petUntilTime      = 0;
unsigned long angryUntilTime    = 0;
unsigned long lastTapTime       = 0;
int rapidTapStreak              = 0;
bool wasTouched                 = false;

unsigned long lastWateredMillis = 0;
const unsigned long soakCooldown = 300000;

unsigned long lastCloudSync     = 0;
const long cloudSyncInterval    = 2500;

unsigned long lastUiRefresh     = 0;
int lastDrawnMoisture           = -1;
int lastDrawnLight              = -1;

String currentStatus = "HAPPY";

// web for login
const char PORTAL_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>AutoPot Wi-Fi</title>
  <style>
    body { font-family: sans-serif; background: #022c22; color: #f8fafc; text-align: center; padding: 20px; }
    .card { background: rgba(6, 44, 34, 0.9); border: 1px solid #34d399; border-radius: 16px; padding: 24px; max-width: 320px; margin: auto; box-shadow: 0 10px 25px rgba(0,0,0,0.5); }
    h2 { color: #34d399; margin-bottom: 8px; }
    p { font-size: 13px; color: #94a3b8; }
    input { width: 90%; padding: 12px; margin: 10px 0; border-radius: 8px; border: 1px solid #10b981; background: #01140e; color: white; font-size: 14px; }
    button { background: #10b981; color: white; border: none; padding: 14px; border-radius: 8px; font-weight: bold; cursor: pointer; width: 98%; font-size: 16px; }
    button:active { background: #059669; }
  </style>
</head>
<body>
  <div class="card">
    <h2>AutoPot Wi-Fi Setup</h2>
    <p>Connect your companion to the internet</p>
    <form action="/save" method="POST">
      <input type="text" name="ssid" placeholder="Wi-Fi Name (SSID)" required><br>
      <input type="password" name="password" placeholder="Wi-Fi Password"><br>
      <button type="submit">Save & Connect</button>
    </form>
  </div>
</body>
</html>
)rawliteral";


// runs the pump
void waterPlant(int seconds) {
  Serial.printf("watering for %d seconds...\n", seconds);
  digitalWrite(RELAY_PUMP_PIN, HIGH);
  delay(seconds * 1000);
  digitalWrite(RELAY_PUMP_PIN, LOW);
  lastWateredMillis = millis();
}

// saves user settings to flash
void savePlantSettingsToFlash() {
  prefs.begin("autopot", false);
  prefs.putInt("moist", targetMoisture);
  prefs.putInt("light", dailyLightHours);
  prefs.putBool("manual", isManualMode);
  prefs.putString("mode", currentPlantMode);
  prefs.end();
  controlsNeedPush = true;
}


// screen stuff

void drawSettingsGearIcon() {
  tft.fillRoundRect(275, 5, 40, 32, 8, ILI9341_DARKGREY);
  tft.drawRoundRect(275, 5, 40, 32, 8, ILI9341_WHITE);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.setCursor(288, 12);
  tft.print("*");
}

void drawFace(FaceState face) {
  if (currentView != VIEW_TAMAGOTCHI) return;
  if (face == currentFace) return;
  currentFace = face;

  switch (face) {
    case FACE_HAPPY:   tft.drawRGBBitmap(0, 0, face_happy, 320, 240);   break;
    case FACE_THIRSTY: tft.drawRGBBitmap(0, 0, face_thirsty, 320, 240); break;
    case FACE_PET:     tft.drawRGBBitmap(0, 0, face_pet, 320, 240);     break;
    case FACE_ANGRY:   tft.drawRGBBitmap(0, 0, face_angry, 320, 240);   break;
    case FACE_SLEEP:   tft.drawRGBBitmap(0, 0, face_sleep, 320, 240);   break;
    default: break;
  }
  drawSettingsGearIcon();
}

void drawSettingsScreen(int moisture) {
  tft.fillScreen(ILI9341_BLACK);

  tft.setTextColor(ILI9341_GREEN);
  tft.setTextSize(2);
  tft.setCursor(20, 10);
  tft.println("AUTOPOT SETTINGS");

  // close btn
  tft.fillRoundRect(275, 5, 40, 30, 6, ILI9341_RED);
  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(288, 11);
  tft.println("X");

  // mode switcher
  if (isManualMode) {
    tft.fillRoundRect(20, 40, 280, 32, 6, ILI9341_ORANGE);
    tft.setTextColor(ILI9341_BLACK);
    tft.setTextSize(2);
    tft.setCursor(35, 48);
    tft.println("[MODE: MANUAL OVERRIDE]");
  } else {
    tft.fillRoundRect(20, 40, 280, 32, 6, ILI9341_DARKGREEN);
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(2);
    tft.setCursor(55, 48);
    tft.println("[MODE: AUTO CARE]");
  }

  // controls
  if (!isManualMode) {
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(2);
    tft.setCursor(20, 85);
    tft.print("Moist:");

    tft.fillRoundRect(185, 78, 45, 27, 6, ILI9341_NAVY);
    tft.setTextColor(ILI9341_CYAN);
    tft.setCursor(202, 83);
    tft.println("-");

    tft.fillRoundRect(240, 78, 45, 27, 6, ILI9341_NAVY);
    tft.setCursor(257, 83);
    tft.println("+");

    tft.setTextColor(ILI9341_WHITE);
    tft.setCursor(20, 118);
    tft.print("Light:");

    tft.fillRoundRect(185, 110, 45, 27, 6, ILI9341_NAVY);
    tft.setTextColor(ILI9341_CYAN);
    tft.setCursor(202, 115);
    tft.println("-");

    tft.fillRoundRect(240, 110, 45, 27, 6, ILI9341_NAVY);
    tft.setCursor(257, 115);
    tft.println("+");
  } else {
    tft.fillRoundRect(20, 78, 280, 27, 6, ILI9341_BLUE);
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(2);
    tft.setCursor(45, 83);
    tft.println("[ WATER PLANT (3s) ]");

    tft.fillRoundRect(20, 110, 280, 27, 6, manualLightState ? ILI9341_YELLOW : ILI9341_DARKGREY);
    tft.setTextColor(manualLightState ? ILI9341_BLACK : ILI9341_WHITE);
    tft.setTextSize(2);
    tft.setCursor(40, 115);
    tft.printf("[ GROW LIGHT: %s ]\n", manualLightState ? "ON" : "OFF");
  }

  tft.fillRoundRect(20, 145, 135, 25, 6, 0x03E0);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(1);
  tft.setCursor(35, 154);
  tft.println("PLANT PROFILES");

  tft.fillRoundRect(165, 145, 135, 25, 6, ILI9341_PURPLE);
  tft.setCursor(185, 154);
  tft.println("STATS DASHBOARD");

  tft.fillRoundRect(20, 178, 280, 25, 6, ILI9341_DARKGREEN);
  tft.setCursor(120, 187);
  tft.println("SETUP WI-FI");

  // footer status
  tft.setTextSize(1);
  tft.setCursor(20, 210);
  if (WiFi.status() == WL_CONNECTED) {
    tft.setTextColor(ILI9341_GREEN);
    String shortSSID = savedSSID;
    if (shortSSID.length() > 12) shortSSID = shortSSID.substring(0, 12) + "..";
    tft.printf("WIFI: %s\n", shortSSID.c_str());
  } else {
    tft.setTextColor(ILI9341_ORANGE);
    tft.println("WIFI: Offline");
  }

  tft.setTextColor(ILI9341_LIGHTGREY);
  tft.setCursor(180, 210); 
  if (lastWateredMillis == 0) tft.print("WATER: Never");
  else tft.printf("WATER: %lum ago", (millis() - lastWateredMillis) / 60000);

  lastDrawnMoisture = -1;
  lastDrawnLight = -1;
}

void drawProfilesScreen() {
  tft.fillScreen(ILI9341_BLACK);

  // back btn
  tft.fillRoundRect(10, 8, 35, 24, 6, ILI9341_DARKGREY);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.setCursor(18, 12);
  tft.print("<");

  tft.setTextColor(ILI9341_GREEN);
  tft.setCursor(60, 12);
  tft.println("PLANT PROFILES");

  tft.fillRoundRect(20, 45, 135, 60, 8, 0x03E0);
  tft.setTextColor(ILI9341_WHITE); tft.setCursor(50, 55); tft.print("CACTUS");
  tft.setTextColor(ILI9341_CYAN); tft.setTextSize(1); tft.setCursor(60, 80); tft.print("15% | 10h");

  tft.fillRoundRect(165, 45, 135, 60, 8, 0x03E0);
  tft.setTextSize(2); tft.setTextColor(ILI9341_WHITE); tft.setCursor(175, 55); tft.print("SUCCULENT");
  tft.setTextColor(ILI9341_CYAN); tft.setTextSize(1); tft.setCursor(200, 80); tft.print("20% | 8h");

  tft.fillRoundRect(20, 115, 135, 60, 8, 0x03E0);
  tft.setTextSize(2); tft.setTextColor(ILI9341_WHITE); tft.setCursor(40, 125); tft.print("TROPICAL");
  tft.setTextColor(ILI9341_CYAN); tft.setTextSize(1); tft.setCursor(60, 150); tft.print("55% | 10h");

  tft.fillRoundRect(165, 115, 135, 60, 8, 0x03E0);
  tft.setTextSize(2); tft.setTextColor(ILI9341_WHITE); tft.setCursor(205, 125); tft.print("ROSE");
  tft.setTextColor(ILI9341_CYAN); tft.setTextSize(1); tft.setCursor(200, 150); tft.print("60% | 12h");
  
  tft.fillRoundRect(92, 185, 135, 45, 8, 0x03E0);
  tft.setTextSize(2); tft.setTextColor(ILI9341_WHITE); tft.setCursor(138, 195); tft.print("HERB");
  tft.setTextColor(ILI9341_CYAN); tft.setTextSize(1); tft.setCursor(130, 215); tft.print("65% | 14h");
}

void drawWiFiInfoScreen() {
  tft.fillScreen(ILI9341_BLACK);

  tft.fillRoundRect(10, 8, 35, 24, 6, ILI9341_DARKGREY);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.setCursor(18, 12);
  tft.print("<");

  tft.setTextColor(ILI9341_GREEN);
  tft.setTextSize(2);
  tft.setCursor(60, 12);
  tft.println("WI-FI CONNECTION");

  tft.drawRoundRect(20, 45, 280, 80, 8, ILI9341_GREEN);
  tft.setCursor(35, 60);
  tft.setTextSize(1);
  tft.setTextColor(ILI9341_WHITE);
  tft.print("NETWORK: ");
  tft.setTextColor(ILI9341_YELLOW);
  tft.println(savedSSID);

  tft.setCursor(35, 80);
  tft.setTextColor(ILI9341_WHITE);
  tft.print("IP ADDR: ");
  tft.setTextColor(ILI9341_CYAN);
  tft.println(WiFi.localIP().toString());

  tft.setCursor(35, 100);
  tft.setTextColor(ILI9341_WHITE);
  tft.print("STATUS:  ");
  tft.setTextColor(ILI9341_GREEN);
  tft.println("CONNECTED");

  tft.fillRoundRect(20, 135, 280, 36, 6, ILI9341_DARKGREEN);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.setCursor(45, 145);
  tft.println("[ CHANGE NETWORK ]");

  tft.fillRoundRect(20, 180, 280, 36, 6, ILI9341_MAROON);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.setCursor(55, 190);
  tft.println("[ FORGET WI-FI ]");
}

void drawStatsScreen() {
  tft.fillScreen(ILI9341_BLACK);

  tft.fillRoundRect(10, 8, 35, 24, 6, ILI9341_DARKGREY);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.setCursor(18, 12);
  tft.print("<");

  tft.setTextColor(ILI9341_GREEN);
  tft.setTextSize(2);
  tft.setCursor(55, 12);
  tft.println("BOTANIST TELEMETRY");

  tft.drawRect(20, 45, 280, 80, ILI9341_GREEN);
  tft.setCursor(35, 60);
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_WHITE);
  tft.println("Soil Moisture:");

  tft.drawRect(20, 135, 280, 80, ILI9341_YELLOW);
  tft.setCursor(35, 150);
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_WHITE);
  tft.println("Sun Exposure:");

  lastDrawnMoisture = -1;
  lastDrawnLight = -1;
}

// updates the small text numbers without clearing the whole screen
void updateLiveScreenNumbers(int moisture, int light) {
  if (currentView == VIEW_SETTINGS) {
    if (moisture != lastDrawnMoisture || light != lastDrawnLight) {
      lastDrawnMoisture = moisture;
      lastDrawnLight = light;
      
      tft.fillRect(20, 226, 280, 12, ILI9341_BLACK);
      tft.setCursor(20, 226);
      tft.setTextSize(1);
      tft.setTextColor(ILI9341_GREEN);
      tft.printf("LIVE SOIL: %d%%  |  LIGHT: %d LUX\n", moisture, light);
    }

    if (!isManualMode) {
      tft.fillRect(100, 85, 75, 20, ILI9341_BLACK);
      tft.setCursor(105, 85);
      tft.setTextSize(2);
      tft.setTextColor(ILI9341_CYAN);
      tft.printf("%d%%\n", targetMoisture);

      tft.fillRect(100, 118, 75, 20, ILI9341_BLACK);
      tft.setCursor(105, 118);
      tft.setTextColor(ILI9341_YELLOW);
      tft.printf("%dh\n", dailyLightHours);
    }
  } 
  else if (currentView == VIEW_STATS) {
    if (moisture != lastDrawnMoisture) {
      lastDrawnMoisture = moisture;
      tft.fillRect(40, 88, 240, 30, ILI9341_BLACK);
      tft.setCursor(40, 88);
      tft.setTextSize(3);
      tft.setTextColor(ILI9341_GREEN);
      tft.printf("%d %%\n", moisture);
    }
    if (light != lastDrawnLight) {
      lastDrawnLight = light;
      tft.fillRect(40, 178, 240, 30, ILI9341_BLACK);
      tft.setCursor(40, 178);
      tft.setTextSize(3);
      tft.setTextColor(ILI9341_YELLOW);
      tft.printf("%d LUX\n", light);
    }
  }
}

// wifi stuff

boolean isIp(String str) {
  for (size_t i = 0; i < str.length(); i++) {
    int c = str.charAt(i);
    if (c != '.' && (c < '0' || c > '9')) return false;
  }
  return true;
}

boolean captivePortal() {
  if (!isIp(server.hostHeader())) {
    server.sendHeader("Location", String("http://") + server.client().localIP().toString(), true);
    server.send(302, "text/plain", ""); // redirect trick to make phone popup work
    server.client().stop();
    return true;
  }
  return false;
}

void launchCaptivePortal() {
  currentView = VIEW_PORTAL;
  isPortalActive = true;

  WiFi.mode(WIFI_AP);
  WiFi.softAP("AutoPot-Setup");

  IPAddress IP = WiFi.softAPIP();
  dnsServer.start(DNS_PORT, "*", IP);

  server.on("/", []() {
    if (captivePortal()) return;
    server.send(200, "text/html", PORTAL_HTML);
  });

  server.on("/save", HTTP_POST, []() {
    if (server.hasArg("ssid")) {
      String newSSID = server.arg("ssid");
      String newPass = server.arg("password");

      prefs.begin("wifi-config", false);
      prefs.putString("ssid", newSSID);
      prefs.putString("pass", newPass);
      prefs.end();

      server.send(200, "text/html", "<html><body style='background:#022c22;color:#34d399;text-align:center;padding:50px;'><h2>Saved!</h2><p>Rebooting...</p></body></html>");
      delay(1500);
      ESP.restart();
    }
  });

  server.onNotFound([]() {
    if (!captivePortal()) {
      server.send(200, "text/html", PORTAL_HTML);
    }
  });

  server.begin();

  tft.fillScreen(ILI9341_BLACK);
  tft.drawRoundRect(15, 15, 290, 155, 10, ILI9341_GREEN);
  tft.setTextColor(ILI9341_GREEN);
  tft.setTextSize(2);
  tft.setCursor(55, 30);
  tft.println("WI-FI SETUP MODE");

  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(1);
  tft.setCursor(35, 65);
  tft.println("1. On phone, connect to Wi-Fi:");

  tft.setTextColor(ILI9341_YELLOW);
  tft.setTextSize(2);
  tft.setCursor(45, 85);
  tft.println("\"AutoPot-Setup\"");

  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(1);
  tft.setCursor(35, 120);
  tft.println("2. Setup pops up automatically!");

  tft.fillRoundRect(20, 185, 280, 42, 8, ILI9341_RED);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.setCursor(55, 198);
  tft.println("[ CANCEL & RETURN ]");
}

void exitCaptivePortal(int moisture) {
  dnsServer.stop();
  server.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);
  isPortalActive = false;

  currentView = VIEW_SETTINGS;
  drawSettingsScreen(moisture);
}

// syncs data with the web app
void syncWithCloud(int moisture, int light) {
  if (WiFi.status() != WL_CONNECTED || isPortalActive) return;

  WiFiClientSecure client; client.setInsecure();
  HTTPClient https;
  https.setTimeout(3500);
  https.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

  String controlsUrl = String(FIREBASE_HOST) + "/devices/" + DEVICE_ID + "/controls.json";

  // push changes to firebase if someone tapped the screen
  if (controlsNeedPush) {
    controlsNeedPush = false;
    https.begin(client, controlsUrl);
    https.addHeader("Content-Type", "application/json");

    StaticJsonDocument<256> outDoc;
    outDoc["manualMode"]     = isManualMode;
    outDoc["lightState"]     = manualLightState;
    outDoc["customMoisture"] = targetMoisture;
    outDoc["customLight"]    = dailyLightHours;
    outDoc["mode"]           = currentPlantMode;

    String patchBody;
    serializeJson(outDoc, patchBody);
    https.sendRequest("PATCH", (uint8_t*)patchBody.c_str(), patchBody.length());
    https.end();
  }
  else {
    // otherwise just fetch current settings from the app
    https.begin(client, controlsUrl);
    if (https.GET() == 200) {
      StaticJsonDocument<512> doc;
      if (!deserializeJson(doc, https.getString())) {
        bool incomingManual = doc["manualMode"] | false;
        bool incomingLight  = doc["lightState"] | false;
        bool triggerPump    = doc["triggerPump"] | false;
        int  incomingMoist  = doc["customMoisture"] | targetMoisture;
        int  incomingLightH = doc["customLight"] | dailyLightHours;
        String incomingMode = doc["mode"] | currentPlantMode;

        bool screenNeedsRedraw = false;

        if (incomingManual != isManualMode || incomingLight != manualLightState ||
            incomingMoist != targetMoisture || incomingLightH != dailyLightHours || incomingMode != currentPlantMode) {
          isManualMode     = incomingManual;
          manualLightState = incomingLight;
          targetMoisture   = incomingMoist;
          dailyLightHours  = incomingLightH;
          currentPlantMode = incomingMode;
          screenNeedsRedraw = true;
          
          prefs.begin("autopot", false);
          prefs.putInt("moist", targetMoisture);
          prefs.putInt("light", dailyLightHours);
          prefs.putBool("manual", isManualMode);
          prefs.putString("mode", currentPlantMode);
          prefs.end();
        }

        if (screenNeedsRedraw && currentView == VIEW_SETTINGS) {
          drawSettingsScreen(moisture);
        }

        if (isManualMode && triggerPump) {
          waterPlant(3);
          https.end();
          https.begin(client, controlsUrl);
          https.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
          https.addHeader("Content-Type", "application/json");
          https.sendRequest("PATCH", (uint8_t*)"{\"triggerPump\":false}", 21);
        }
      }
    }
    https.end();
  }

  // send sensor data up to firebase
  String telemetryUrl = String(FIREBASE_HOST) + "/devices/" + DEVICE_ID + "/telemetry.json";
  https.begin(client, telemetryUrl);
  https.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  https.addHeader("Content-Type", "application/json");

  StaticJsonDocument<200> tDoc;
  tDoc["moisture"] = moisture;
  tDoc["light"]    = light;
  tDoc["status"]   = currentStatus;

  String body; serializeJson(tDoc, body);
  https.PUT(body);
  https.end();
}

void setup() {
  Serial.begin(115200);

  pinMode(RELAY_PUMP_PIN, OUTPUT);
  pinMode(RELAY_LIGHT_PIN, OUTPUT);
  digitalWrite(RELAY_PUMP_PIN, LOW);
  digitalWrite(RELAY_LIGHT_PIN, LOW);

  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  tftSPI.begin(TFT_CLK, TFT_MISO, TFT_MOSI, TFT_CS);
  tft.begin(40000000);
  tft.setRotation(1);
  tft.fillScreen(ILI9341_BLACK);

  touchSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  ts.begin(touchSPI);
  ts.setRotation(1);

  drawFace(FACE_HAPPY);

  // load saved settings from flash memory
  prefs.begin("autopot", true);
  targetMoisture   = prefs.getInt("moist", 35);
  dailyLightHours  = prefs.getInt("light", 10);
  isManualMode     = prefs.getBool("manual", false);
  currentPlantMode = prefs.getString("mode", "CUSTOM");
  prefs.end();

  // connect to wifi in background if saved
  prefs.begin("wifi-config", true);
  savedSSID = prefs.getString("ssid", "");
  savedPass = prefs.getString("pass", "");
  prefs.end();

  if (savedSSID != "") {
    WiFi.mode(WIFI_STA);
    WiFi.begin(savedSSID.c_str(), savedPass.c_str());
  }
}

void loop() {
  unsigned long now = millis();

  // 1. read sensors
  int rawSoil = analogRead(SOIL_PIN);
  int moisture = constrain(map(rawSoil, 2600, 1500, 0, 100), 0, 100);
  int lightLux = constrain(map(analogRead(LDR_PIN), 4095, 0, 0, 1000), 0, 1000);

  // 2. handle portal
  if (isPortalActive) {
    dnsServer.processNextRequest();
    server.handleClient();

    if (ts.touched()) {
      TS_Point p = ts.getPoint();
      if (p.z > 300) {
        int touchY = map(p.y, 200, 3800, 0, 240);
        if (touchY > 175) {
          exitCaptivePortal(moisture); // cancel button
        }
      }
    }
    return;
  }

  // 3. touch UI logic
  if (ts.touched()) {
    TS_Point p = ts.getPoint();
    if (p.z > 300 && !wasTouched) {
      wasTouched = true;

      int touchX = map(p.x, 200, 3800, 0, 320);
      int touchY = map(p.y, 200, 3800, 0, 240);

      if (currentView == VIEW_TAMAGOTCHI) {
        if (touchX > 250 && touchY < 60) {
          currentView = VIEW_SETTINGS;
          drawSettingsScreen(moisture);
        } else {
          // pet the plant
          petUntilTime = now + 700;
          if (now - lastTapTime < 1500) rapidTapStreak++;
          else rapidTapStreak = 1;
          lastTapTime = now;

          // angry if tapped too fast
          if (rapidTapStreak >= 4) {
            angryUntilTime = now + 20000;
            petUntilTime   = 0;
            rapidTapStreak = 0;
          }
        }
      }
      else if (currentView == VIEW_SETTINGS) {
        // close settings
        if (touchX > 265 && touchY < 40) {
          currentView = VIEW_TAMAGOTCHI;
          currentFace = FACE_NONE;
          tft.fillScreen(ILI9341_BLACK);
        }
        // mode toggle
        else if (touchY >= 40 && touchY <= 75) {
          isManualMode = !isManualMode;
          savePlantSettingsToFlash();
          drawSettingsScreen(moisture);
        }
        else if (!isManualMode) {
          // moist -
          if (touchX >= 185 && touchX <= 230 && touchY >= 78 && touchY <= 105) {
            targetMoisture = constrain(targetMoisture - 5, 10, 90);
            currentPlantMode = "CUSTOM"; 
            savePlantSettingsToFlash();
            lastDrawnMoisture = -1;
          }
          // moist +
          else if (touchX >= 240 && touchX <= 285 && touchY >= 78 && touchY <= 105) {
            targetMoisture = constrain(targetMoisture + 5, 10, 90);
            currentPlantMode = "CUSTOM";
            savePlantSettingsToFlash();
            lastDrawnMoisture = -1;
          }
          // light -
          else if (touchX >= 185 && touchX <= 230 && touchY >= 110 && touchY <= 137) {
            dailyLightHours = constrain(dailyLightHours - 1, 0, 24);
            currentPlantMode = "CUSTOM";
            savePlantSettingsToFlash();
            lastDrawnMoisture = -1;
          }
          // light +
          else if (touchX >= 240 && touchX <= 285 && touchY >= 110 && touchY <= 137) {
            dailyLightHours = constrain(dailyLightHours + 1, 0, 24);
            currentPlantMode = "CUSTOM";
            savePlantSettingsToFlash();
            lastDrawnMoisture = -1;
          }
        } else {
          // manual watering
          if (touchY >= 78 && touchY <= 105) waterPlant(3);
          // manual light toggle
          else if (touchY >= 110 && touchY <= 137) {
            manualLightState = !manualLightState;
            savePlantSettingsToFlash();
            drawSettingsScreen(moisture);
          }
        }

        // nav buttons
        if (touchY >= 145 && touchY <= 170) {
          if (touchX < 160) {
            currentView = VIEW_PROFILES; 
            drawProfilesScreen();
          } else {
            currentView = VIEW_STATS;
            drawStatsScreen();
          }
        }
        else if (touchY >= 178 && touchY <= 203) {
          if (WiFi.status() == WL_CONNECTED) {
            currentView = VIEW_WIFI_INFO;
            drawWiFiInfoScreen();
          } else {
            launchCaptivePortal();
          }
        }
      }
      else if (currentView == VIEW_PROFILES) {
        // back button
        if (touchX < 60 && touchY < 45) {
          currentView = VIEW_SETTINGS;
          drawSettingsScreen(moisture);
        }
        else if (touchX < 160 && touchY > 45 && touchY < 105) {
          targetMoisture = 15; dailyLightHours = 10; isManualMode = false; currentPlantMode = "CACTUS";
          savePlantSettingsToFlash(); currentView = VIEW_SETTINGS; drawSettingsScreen(moisture);
        }
        else if (touchX > 160 && touchY > 45 && touchY < 105) {
          targetMoisture = 20; dailyLightHours = 8; isManualMode = false; currentPlantMode = "SUCCULENT";
          savePlantSettingsToFlash(); currentView = VIEW_SETTINGS; drawSettingsScreen(moisture);
        }
        else if (touchX < 160 && touchY > 115 && touchY < 175) {
          targetMoisture = 55; dailyLightHours = 10; isManualMode = false; currentPlantMode = "TROPICAL";
          savePlantSettingsToFlash(); currentView = VIEW_SETTINGS; drawSettingsScreen(moisture);
        }
        else if (touchX > 160 && touchY > 115 && touchY < 175) {
          targetMoisture = 60; dailyLightHours = 12; isManualMode = false; currentPlantMode = "ROSE";
          savePlantSettingsToFlash(); currentView = VIEW_SETTINGS; drawSettingsScreen(moisture);
        }
        else if (touchY > 185) {
          targetMoisture = 65; dailyLightHours = 14; isManualMode = false; currentPlantMode = "HERB";
          savePlantSettingsToFlash(); currentView = VIEW_SETTINGS; drawSettingsScreen(moisture);
        }
      }
      else if (currentView == VIEW_WIFI_INFO) {
        if (touchX < 60 && touchY < 45) {
          currentView = VIEW_SETTINGS; drawSettingsScreen(moisture);
        }
        else if (touchY >= 135 && touchY <= 170) launchCaptivePortal();
        else if (touchY >= 180 && touchY <= 215) {
          prefs.begin("wifi-config", false); prefs.clear(); prefs.end();
          savedSSID = ""; WiFi.disconnect(true);
          currentView = VIEW_SETTINGS; drawSettingsScreen(moisture);
        }
      }
      else if (currentView == VIEW_STATS) {
        if (touchX < 60 && touchY < 45) {
          currentView = VIEW_SETTINGS; drawSettingsScreen(moisture);
        }
      }
    }
  } else {
    wasTouched = false;
  }

  // 4. physical care engine
  if (isManualMode) {
    digitalWrite(RELAY_LIGHT_PIN, manualLightState ? HIGH : LOW);
  } else {
    digitalWrite(RELAY_LIGHT_PIN, (dailyLightHours > 0) ? HIGH : LOW);

    // auto water with safety cooldown
    if (moisture < targetMoisture && (now - lastWateredMillis >= soakCooldown)) {
      waterPlant(2);
    }
  }

  // 5. facial expressions
  if (currentView == VIEW_TAMAGOTCHI) {
    FaceState desiredFace;
    if (now < petUntilTime) {
      desiredFace = FACE_PET;
    } else if (moisture < 30) {
      desiredFace = FACE_THIRSTY;
      currentStatus = "THIRSTY";
    } else if (now < angryUntilTime) {
      desiredFace = FACE_ANGRY;
      currentStatus = "HAPPY";
    } else {
      desiredFace = FACE_HAPPY;
      currentStatus = "HAPPY";
    }
    drawFace(desiredFace);
  }

  // 6. refresh ui text
  if (now - lastUiRefresh >= 400) {
    lastUiRefresh = now;
    updateLiveScreenNumbers(moisture, lightLux);
  }

  // 7. cloud sync
  if (now - lastCloudSync >= cloudSyncInterval) {
    lastCloudSync = now;
    syncWithCloud(moisture, lightLux);
  }

  delay(20);
}
// Finally we r done.
// The end
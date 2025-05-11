#include <EEPROM.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Firebase_ESP_Client.h>

// Constants
#define EEPROM_SIZE 100
#define FIREBASE_AUTH "AIzaSyDQZ_kzHvIKiT8wL_pGUdfLWC0lYQVNNZQ"
#define FIREBASE_URL "https://esp-project-569ce-default-rtdb.asia-southeast1.firebasedatabase.app/"
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define CONFIG_BUTTON 0
#define STATUS_LED 2

// OLED Display
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Firebase
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Web Server
WebServer server(80);

// Credentials
String ssid = "";
String password = "";
String deviceId = "";
bool apmode = false;
bool scanComplete = false;
String scannedNetworks = "<p>Scanning networks...</p>";

// Function Prototypes
void showMessage(String line1, String line2 = "", String line3 = "");
void connectWiFi();
void enterAPMode();
void handleWebServer();
void writeEEPROM(int start, String data);
String readEEPROM(int start);
void setupFirebase();
void fetchFirebaseText();
void clearEEPROM();

void showMessage(String line1, String line2, String line3) {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.println(line1);
  if (line2 != "") display.println(line2);
  if (line3 != "") display.println(line3);
  display.display();
  
  Serial.println(line1);
  if (line2 != "") Serial.println(line2);
  if (line3 != "") Serial.println(line3);
  Serial.println("----------------------");
}

void connectWiFi() {
  WiFi.begin(ssid.c_str(), password.c_str());
  showMessage("Connecting to WiFi", ssid);
  Serial.print("Connecting to WiFi: "); Serial.println(ssid);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    display.print(".");
    display.display();
    Serial.print(".");
    attempts++;
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    showMessage("Connected", WiFi.localIP().toString());
    Serial.println("WiFi connected");
    delay(2000);  // Show connection status for 2 seconds
    Serial.print("IP Address: "); Serial.println(WiFi.localIP());
    apmode = false;
  } else {
    showMessage("WiFi Failed", "Starting AP Mode");
    Serial.println("WiFi connection failed - Starting AP Mode");
    delay(2000);  // Show failure status for 2 seconds
    enterAPMode();  // Automatically enter AP mode when WiFi fails
  }
}

void enterAPMode() {
  apmode = true;
  digitalWrite(STATUS_LED, HIGH);
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP("ESP32_Config", "");
  showMessage("AP Mode", "SSID: ESP32_Config", "IP: " + WiFi.softAPIP().toString());
  WiFi.scanNetworks(true);
  handleWebServer();
}

void handleWebServer() {
  server.on("/", HTTP_GET, []() {
    String html = R"rawliteral(
      <!DOCTYPE html>
      <html lang="en">
      <head>
          <meta charset="UTF-8">
          <meta name="viewport" content="width=device-width, initial-scale=1.0">
          <title>ESP32 Configuration</title>
          <style>
              body {
                  font-family: 'Arial', sans-serif;
                  background-color: #fafafa;
                  color: #4a4a4a;
                  padding: 20px;
                  display: flex;
                  justify-content: center;
                  align-items: center;
                  min-height: 100vh;
                  margin: 0;
              }
              .container {
                  background: #ffffff;
                  border-radius: 15px;
                  box-shadow: 0 4px 15px rgba(0, 0, 0, 0.1);
                  width: 100%;
                  max-width: 600px;
                  padding: 30px;
                  text-align: center;
              }
              h1 {
                  color: #e91e63;
                  font-size: 28px;
                  margin-bottom: 30px;
                  font-weight: 600;
              }
              .form-group {
                  margin-bottom: 20px;
                  text-align: left;
              }
              label {
                  display: block;
                  color: #666;
                  margin-bottom: 8px;
                  font-size: 14px;
              }
              input[type="text"], input[type="password"] {
                  width: 100%;
                  padding: 12px;
                  border: 2px solid #f8bbd0;
                  border-radius: 8px;
                  font-size: 16px;
                  transition: all 0.3s ease;
                  box-sizing: border-box;
              }
              input[type="text"]:focus, input[type="password"]:focus {
                  border-color: #e91e63;
                  outline: none;
                  box-shadow: 0 0 0 3px rgba(233, 30, 99, 0.1);
              }
              .btn {
                  background-color: #e91e63;
                  color: white;
                  border: none;
                  padding: 12px 30px;
                  border-radius: 8px;
                  font-size: 16px;
                  cursor: pointer;
                  transition: background-color 0.3s ease;
                  width: 100%;
                  margin-top: 10px;
              }
              .btn:hover {
                  background-color: #c2185b;
              }
              .btn-secondary {
                  background-color: #9e9e9e;
                  margin-top: 10px;
              }
              .btn-secondary:hover {
                  background-color: #757575;
              }
              .btn-danger {
                  background-color: #f44336;
                  margin-top: 10px;
              }
              .btn-danger:hover {
                  background-color: #d32f2f;
              }
              .status {
                  margin-top: 20px;
                  padding: 15px;
                  border-radius: 8px;
                  background-color: #f8bbd0;
                  color: #c2185b;
                  font-size: 14px;
              }
              .current-settings {
                  margin-top: 30px;
                  padding: 20px;
                  background-color: #fff5f8;
                  border-radius: 8px;
                  text-align: left;
              }
              .current-settings h2 {
                  color: #e91e63;
                  font-size: 18px;
                  margin-bottom: 15px;
              }
              .current-settings p {
                  margin: 8px 0;
                  color: #666;
                  font-size: 14px;
              }
              .current-settings strong {
                  color: #e91e63;
              }
              .networks-table {
                  width: 100%;
                  margin-top: 20px;
                  border-collapse: separate;
                  border-spacing: 0;
                  background: #fff5f8;
                  border-radius: 12px;
                  overflow: hidden;
                  box-shadow: 0 2px 8px rgba(233,30,99,0.08);
                  border: 2px solid #f8bbd0;
              }
              .networks-table th {
                  background-color: #e91e63;
                  color: white;
                  padding: 12px;
                  text-align: left;
                  font-weight: 500;
                  border-bottom: 2px solid #f8bbd0;
              }
              .networks-table td {
                  padding: 12px;
                  border-bottom: 1px solid #f8bbd0;
                  color: #666;
              }
              .networks-table tr:last-child td {
                  border-bottom: none;
              }
              .networks-table tr:hover {
                  background-color: #f8bbd0;
              }
              .signal-strength {
                  display: inline-block;
                  width: 60px;
                  height: 20px;
                  background: #f8bbd0;
                  border-radius: 10px;
                  overflow: hidden;
                  position: relative;
              }
              .signal-bar {
                  height: 100%;
                  background: #e91e63;
                  border-radius: 10px;
                  transition: width 0.3s ease;
              }
              .button-group {
                  display: flex;
                  gap: 10px;
                  margin-top: 20px;
              }
              .button-group .btn {
                  flex: 1;
                  margin: 0;
              }
              .refresh-btn {
                  background-color: #f8bbd0;
                  color: #e91e63;
                  font-weight: bold;
                  margin-top: 10px;
                  border: none;
                  border-radius: 8px;
                  box-shadow: 0 2px 8px rgba(233,30,99,0.08);
                  transition: background 0.3s, color 0.3s;
              }
              .refresh-btn:hover {
                  background-color: #e91e63;
                  color: #fff;
              }
          </style>
      </head>
      <body>
          <div class="container">
              <h1>ESP32 Configuration</h1>
              <div class="current-settings">
                  <h2>Current Settings</h2>
                  <p><strong>SSID:</strong> )rawliteral" + ssid + R"rawliteral(</p>
                  <p><strong>Device ID:</strong> )rawliteral" + deviceId + R"rawliteral(</p>
              </div>
              <form action="/save" method="POST">
                  <div class="form-group">
                      <label for="ssid">WiFi SSID</label>
                      <input type="text" id="ssid" name="ssid" placeholder="Enter WiFi SSID" required>
                  </div>
                  <div class="form-group">
                      <label for="pass">WiFi Password</label>
                      <input type="password" id="pass" name="pass" placeholder="Enter WiFi Password">
                  </div>
                  <div class="form-group">
                      <label for="devid">Device ID</label>
                      <input type="text" id="devid" name="devid" placeholder="Enter Device ID" required>
                  </div>
                  <div class="button-group">
                      <button type="submit" class="btn">Save Settings</button>
                      <button type="button" class="btn btn-secondary" onclick="clearFields()">Clear Fields</button>
                  </div>
              </form>
              <h3>Available Networks</h3>
              <button class="btn refresh-btn" onclick="window.location.reload()">Refresh Networks</button>
              <div style="margin-top:10px;">
              )rawliteral" + scannedNetworks + R"rawliteral(
              </div>
              <div class="button-group">
                  <button class="btn btn-danger" onclick="window.location.href='/clear';">Clear All Data</button>
              </div>
              <div class="status">
                  Connect to ESP32_Config to configure your device
              </div>
          </div>
          <script>
              function clearFields() {
                  document.getElementById('ssid').value = '';
                  document.getElementById('pass').value = '';
                  document.getElementById('devid').value = '';
              }
          </script>
      </body>
      </html>
    )rawliteral";
    server.send(200, "text/html", html);
  });

  server.on("/save", HTTP_POST, []() {
    String newSSID = server.arg("ssid");
    String newPass = server.arg("pass");
    String newDevID = server.arg("devid");
    
    if (newSSID.length() > 0 && newDevID.length() > 0) {
      writeEEPROM(0, newSSID);
      writeEEPROM(30, newPass);
      writeEEPROM(60, newDevID);
      String html = R"rawliteral(
        <!DOCTYPE html>
        <html lang="en">
        <head>
            <meta charset="UTF-8">
            <meta name="viewport" content="width=device-width, initial-scale=1.0">
            <title>Settings Saved</title>
            <style>
                body {
                    font-family: 'Arial', sans-serif;
                    background-color: #fafafa;
                    color: #4a4a4a;
                    padding: 20px;
                    display: flex;
                    justify-content: center;
                    align-items: center;
                    min-height: 100vh;
                    margin: 0;
                }

                .container {
                    background: #ffffff;
                    border-radius: 15px;
                    box-shadow: 0 4px 15px rgba(0, 0, 0, 0.1);
                    width: 100%;
                    max-width: 400px;
                    padding: 30px;
                    text-align: center;
                }

                .success-icon {
                    font-size: 60px;
                    color: #e91e63;
                    margin-bottom: 20px;
                }

                h1 {
                    color: #e91e63;
                    font-size: 24px;
                    margin-bottom: 20px;
                }

                p {
                    color: #666;
                    margin-bottom: 30px;
                }

                .btn {
                    background-color: #e91e63;
                    color: white;
                    border: none;
                    padding: 12px 30px;
                    border-radius: 8px;
                    font-size: 16px;
                    cursor: pointer;
                    transition: background-color 0.3s ease;
                    text-decoration: none;
                    display: inline-block;
                }

                .btn:hover {
                    background-color: #c2185b;
                }
            </style>
        </head>
        <body>
            <div class="container">
                <div class="success-icon">✓</div>
                <h1>Settings Saved Successfully!</h1>
                <p>The device will restart to apply the new configuration.</p>
                <a href="/" class="btn">Back to Configuration</a>
            </div>
        </body>
        </html>
      )rawliteral";
      server.send(200, "text/html", html);
      delay(2000);
      ESP.restart();
    }
  });

  server.on("/clear", HTTP_GET, []() {
    clearEEPROM();
    String html = R"rawliteral(
      <!DOCTYPE html>
      <html lang="en">
      <head>
          <meta charset="UTF-8">
          <meta name="viewport" content="width=device-width, initial-scale=1.0">
          <title>All Data Cleared</title>
          <style>
              body { font-family: 'Arial', sans-serif; background-color: #fafafa; color: #333; display: flex; justify-content: center; align-items: center; height: 100vh; margin: 0; }
              .container { background: #ffffff; border-radius: 10px; box-shadow: 0 4px 8px rgba(0, 0, 0, 0.1); padding: 40px; text-align: center; width: 300px; }
              h2 { color: #f44336; font-size: 24px; margin-bottom: 20px; }
              p { color: #555; font-size: 16px; margin-bottom: 20px; }
              .btn-back { padding: 12px 20px; background-color: #e91e63; color: white; border: none; border-radius: 5px; font-size: 18px; cursor: pointer; transition: background-color 0.3s ease; text-decoration: none; }
              .btn-back:hover { background-color: #d81b60; }
              .icon { font-size: 50px; color: #f44336; margin-bottom: 20px; }
          </style>
      </head>
      <body>
          <div class="container">
              <div class="icon">❌</div>
              <h2>All Data Cleared</h2>
              <p>The device will restart now. Please reconnect and reconfigure if needed.</p>
              <a href="/" class="btn-back">Back to Configuration</a>
          </div>
      </body>
      </html>
    )rawliteral";
    server.send(200, "text/html", html);
    delay(2000);
    ESP.restart();
  });

  server.begin();
}

void setupFirebase() {
  if (apmode) return;
  
config.api_key = FIREBASE_AUTH;
config.database_url = FIREBASE_URL;
auth.user.email = "hfshahidatul@gmail.com";
auth.user.password = "hfshahidatul123";
Firebase.begin(&config, &auth);
Firebase.reconnectWiFi(true);

// Clear display and set up the cursor
display.clearDisplay();
display.setCursor(0, 0);  // Set cursor to top-left
display.setTextSize(2);    // Set text size
display.println("Firebase");  // First line
display.setCursor(0, 20);    // Move the cursor down to the next line
display.println("Ready");    // Second line
display.display();          // Show the content on the display

delay(2000);  // Show Firebase ready status for 2 seconds
Serial.println("Firebase initialized with user credentials");

}

void fetchFirebaseText() {
  if (apmode || !Firebase.ready()) return;
  
  if (Firebase.RTDB.getString(&fbdo, "/oled/displayText")) {
    String value = fbdo.stringData();
    
    // Show device ID
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(2);
    display.println("Device:");
    display.setCursor(0, 20);
    display.println(deviceId);
    display.display();
    delay(2000);  // Show device info for 2 seconds

    // Show fetched data
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Data:");
    display.setCursor(0, 20);
    display.setTextSize(2);
    display.println(value);
    display.display();
    delay(2000);  // Show data for 2 seconds

    Serial.println("Device: " + deviceId);
    Serial.println("Data: " + value);
  } else {
    showMessage("Firebase Error", fbdo.errorReason());
    delay(2000);  // Show error for 2 seconds
    Serial.println("Firebase error: " + fbdo.errorReason());
  }
}

void writeEEPROM(int start, String data) {
  for (int i = 0; i < data.length(); i++) EEPROM.write(start + i, data[i]);
  EEPROM.write(start + data.length(), '\0');
  EEPROM.commit();
}

String readEEPROM(int start) {
  char data[50];
  int len = 0;
  unsigned char k = EEPROM.read(start);
  while (k != '\0' && len < 50) {
    data[len] = k;
    len++;
    k = EEPROM.read(start + len);
  }
  data[len] = '\0';
  return String(data);
}

void clearEEPROM() {
  EEPROM.begin(EEPROM_SIZE);
  for (int i = 0; i < 60; i++) EEPROM.write(i, 0);
  EEPROM.commit();
  EEPROM.end();
}

void setup() {
  Serial.begin(115200);
  pinMode(CONFIG_BUTTON, INPUT_PULLUP);
  pinMode(STATUS_LED, OUTPUT);
  digitalWrite(STATUS_LED, LOW);
  
  EEPROM.begin(EEPROM_SIZE);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  showMessage("OLED Initialized");
  Serial.println("OLED display initialized");

  // Read credentials from EEPROM
  ssid = readEEPROM(0);
  password = readEEPROM(30);
  deviceId = readEEPROM(60);

  // Check if button is pressed during boot (3 seconds window)
  unsigned long startTime = millis();
  while (millis() - startTime < 3000) {
    if (digitalRead(CONFIG_BUTTON) == LOW) {
      enterAPMode();
      return;
    }
    delay(100);
  }

  // Fallback default credentials if empty
  if (ssid.length() == 0) {
    ssid = "hfsha";
    password = "12345678";
    deviceId = "ESP32Device001";
    writeEEPROM(0, ssid);
    writeEEPROM(30, password);
    writeEEPROM(60, deviceId);
    Serial.println("Default credentials written to EEPROM");
  }

  connectWiFi();
  if (!apmode) {
    setupFirebase();
  }
}

void loop() {
  // Check for config button press
  if (digitalRead(CONFIG_BUTTON) == LOW) {
    delay(50); // Debounce
    if (digitalRead(CONFIG_BUTTON) == LOW) {
      enterAPMode();
    }
  }
  
  if (apmode) {
    server.handleClient();
    
    // Handle WiFi scanning
    if (!scanComplete) {
      int n = WiFi.scanComplete();
      if (n == -2) WiFi.scanNetworks(true);
      else if (n >= 0) {
        scannedNetworks = "<table border='1' style='width:100%; border-collapse:collapse;'>"
                         "<tr><th>SSID</th><th>Signal Strength</th></tr>";
        for (int i = 0; i < n; ++i) {
          scannedNetworks += "<tr><td>" + WiFi.SSID(i) + "</td><td>" + String(WiFi.RSSI(i)) + " dBm</td></tr>";
        }
        scannedNetworks += "</table>";
        scanComplete = true;
        WiFi.scanDelete();
      }
    }
  } else {
    fetchFirebaseText();
  }
  
  delay(100);
}

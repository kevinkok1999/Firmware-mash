/*
  BW16 Analyzer PRO v0.1
  Target: Realtek AmebaD RTL8720DN BW16
  Read-only Wi-Fi discovery interface over serial (115200 baud).
  Based on documented public Ameba Arduino WiFi APIs.
  No deauthentication, beacon generation, phishing, or DoS traffic.
*/

#include <WiFi.h>

namespace {
constexpr unsigned long kScanIntervalMs = 15000UL;
unsigned long lastScanMs = 0;
bool autoScan = false;

void printHelp() {
  Serial.println();
  Serial.println("BW16 Analyzer PRO v0.1");
  Serial.println("  s  Scan Wi-Fi networks");
  Serial.println("  a  Toggle periodic scans (15 s)");
  Serial.println("  i  Print firmware information");
  Serial.println("  h  Show this help");
  Serial.println();
}

const char* securityName(int kind) {
  switch (kind) {
    case ENC_TYPE_NONE: return "Open";
    case ENC_TYPE_WEP: return "WEP (legacy)";
    case ENC_TYPE_WPA: return "WPA";
    case ENC_TYPE_WPA2: return "WPA2";
    case ENC_TYPE_WPA3: return "WPA3";
    case ENC_TYPE_AUTO: return "Mixed/auto";
    default: return "Unknown";
  }
}

void scanWifi() {
  Serial.println();
  Serial.println("Scanning nearby Wi-Fi networks...");
  int n = WiFi.scanNetworks();

  if (n < 0) {
    Serial.println("Scan failed; check board configuration.");
    return;
  }

  Serial.print("Networks: ");
  Serial.println(n);
  Serial.println("# | SSID | RSSI (dBm) | Security");

  for (int i = 0; i < n; ++i) {
    Serial.print(i + 1);
    Serial.print(" | ");
    const char* ssid = WiFi.SSID(i);
    Serial.print((ssid != nullptr && ssid[0] != '\0') ? ssid : "<hidden>");
    Serial.print(" | ");
    Serial.print(WiFi.RSSI(i));
    Serial.print(" | ");
    Serial.println(securityName(WiFi.encryptionType(i)));
  }

  Serial.println("Scan complete; no network connections initiated.");
}
}  // namespace

void setup() {
  Serial.begin(115200);
  delay(1200);
  WiFi.status();  // Initialize the Ameba Wi-Fi subsystem.
  printHelp();
}

void loop() {
  while (Serial.available() > 0) {
    char command = static_cast<char>(Serial.read());
    if (command == 's' || command == 'S') {
      scanWifi();
      lastScanMs = millis();
    } else if (command == 'a' || command == 'A') {
      autoScan = !autoScan;
      lastScanMs = millis();
      Serial.println(autoScan ? "Periodic scanning ON." : "Periodic scanning OFF.");
    } else if (command == 'i' || command == 'I') {
      Serial.println("BW16 Analyzer PRO v0.1 | RTL8720DN | serial only");
    } else if (command == 'h' || command == 'H') {
      printHelp();
    }
  }

  if (autoScan && (millis() - lastScanMs >= kScanIntervalMs)) {
    scanWifi();
    lastScanMs = millis();
  }
}

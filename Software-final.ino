/*
 * Hardware-Accelerated Network Packet Filtering Engine & Priority Scheduler
 * 
 * Copyright (C) 2026 Your Name or Organization
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 * 
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

const char* WIFI_SSID = "NETWORK_SSID";
const char* WIFI_PASS = "NETWORK_PASSWORD";

#define PIN_SIGNAL_A    15
#define PIN_SIGNAL_B    16
#define PIN_SIGNAL_C    17
#define PIN_SIGNAL_D    18

#define PIN_Q_PASS      8
#define PIN_MUX_OUT     3

#define SCREEN_WIDTH    128
#define SCREEN_HEIGHT   64
#define OLED_RESET      -1
#define OLED_I2C_ADDR   0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

volatile unsigned long totalCapturedPackets = 0;
volatile unsigned long floodPacketCounter   = 0;
volatile unsigned long lastFloodWindowCheck = 0;
volatile bool isFloodDetected               = false;

unsigned long countVipPass      = 0;
unsigned long countStdPass      = 0;
unsigned long countThreatDrop   = 0;
unsigned long countCorruptDrop  = 0;

volatile bool last_A = false;
volatile bool last_B = false;
volatile bool last_C = false;
volatile bool last_D = false;

bool isEapolFrame(const uint8_t* payload, uint16_t len) {
  if (len < 32) return false;
  uint8_t frame_type = (payload[0] & 0x0C) >> 2;
  if (frame_type != 2) return false;

  uint8_t header_len = 24;
  uint8_t frame_subtype = (payload[0] & 0xF0) >> 4;
  if (frame_subtype & 0x08) header_len += 2; // QoS Data

  if (len < header_len + 8) return false;
  uint16_t ether_type = (payload[header_len + 6] << 8) | payload[header_len + 7];
  return (ether_type == 0x888E);
}

void IRAM_ATTR wifi_sniffer_packet_handler(void* buf, wifi_promiscuous_pkt_type_t type) {
  wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buf;
  uint8_t* payload = pkt->payload;
  uint16_t len = pkt->rx_ctrl.sig_len;

  totalCapturedPackets++;
  floodPacketCounter++;

  uint16_t frame_control = payload[0] | (payload[1] << 8);
  uint8_t frame_type    = (frame_control & 0x0C) >> 2;
  uint8_t frame_subtype = (frame_control & 0xF0) >> 4;

  bool is_deauth_disassoc = (frame_type == 0 && (frame_subtype == 10 || frame_subtype == 12));
  bool is_eapol_attack   = isEapolFrame(payload, len);
  bool is_malformed      = (len < 24) || (len > 1600);

  bool flag_A_Threat   = is_deauth_disassoc || is_eapol_attack || is_malformed;
  bool flag_B_Flood    = isFloodDetected;
  bool flag_C_Priority = (frame_type == 0) || (frame_type == 2 && (frame_subtype & 0x08));
  bool flag_D_ValidCRC = (pkt->rx_ctrl.rx_state == 0) && !is_malformed;

  last_A = flag_A_Threat;
  last_B = flag_B_Flood;
  last_C = flag_C_Priority;
  last_D = flag_D_ValidCRC;

  digitalWrite(PIN_SIGNAL_A, flag_A_Threat ? HIGH : LOW);
  digitalWrite(PIN_SIGNAL_B, flag_B_Flood ? HIGH : LOW);
  digitalWrite(PIN_SIGNAL_C, flag_C_Priority ? HIGH : LOW);
  digitalWrite(PIN_SIGNAL_D, flag_D_ValidCRC ? HIGH : LOW);
}

void setup() {
  Serial.begin(115200);

  pinMode(PIN_SIGNAL_A, OUTPUT);
  pinMode(PIN_SIGNAL_B, OUTPUT);
  pinMode(PIN_SIGNAL_C, OUTPUT);
  pinMode(PIN_SIGNAL_D, OUTPUT);

  pinMode(PIN_Q_PASS, INPUT);
  pinMode(PIN_MUX_OUT, INPUT);

  Wire.begin(6, 5);
  if(!display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDR)) {
    Serial.println(F("[ERROR] OLED Allocation Failed"));
  }
  display.clearDisplay();

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Connecting WiFi...");
  display.display();

  uint8_t retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries < 20) {
    delay(500);
    retries++;
  }

  uint8_t currentChannel = 1;
  if (WiFi.status() == WL_CONNECTED) {
    currentChannel = WiFi.channel();
    Serial.printf("Connected to %s on Channel %d (IP: %s)\n", 
                  WIFI_SSID, currentChannel, WiFi.localIP().toString().c_str());
  } else {
    Serial.println("Wi-Fi Connection Failed! Defaulting sniffer to Channel 1.");
  }

  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(currentChannel, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous_rx_cb(&wifi_sniffer_packet_handler);

  Serial.println(F("[SYSTEM] Live Wi-Fi Packet Capture & Security Classifier Active..."));
}

void loop() {
  if (millis() - lastFloodWindowCheck >= 100) {
    isFloodDetected = (floodPacketCounter > 150);
    floodPacketCounter = 0;
    lastFloodWindowCheck = millis();
  }

  bool hw_pass = digitalRead(PIN_Q_PASS);
  bool hw_mux  = digitalRead(PIN_MUX_OUT);

  String routeClassification = "";

  if (hw_pass) {
    if (last_C) {
      countVipPass++;
      routeClassification = "VIP PASS (Q3)";
    } else {
      countStdPass++;
      routeClassification = "STD PASS (Q1)";
    }
  } else {
    if (last_A || last_B) {
      countThreatDrop++;
      routeClassification = "DROP (THREAT)";
    } else {
      countCorruptDrop++;
      routeClassification = "DROP (CORRUPT)";
    }
  }

  Serial.printf("BUS:[%d %d %d %d] | HW_PASS:%d | MUX:%d | CLASS: %s\n", 
                last_A, last_B, last_C, last_D, hw_pass, hw_mux, routeClassification.c_str());

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0, 0);
  display.printf("BUS: A:%d B:%d C:%d D:%d", last_A, last_B, last_C, last_D);
  display.drawLine(0, 9, 128, 9, SSD1306_WHITE);

  display.setCursor(0, 13);
  display.printf("ROUTE: %s\n", routeClassification.c_str());
  display.printf("TOTAL: %lu\n", totalCapturedPackets);

  display.drawLine(0, 32, 128, 32, SSD1306_WHITE);

  display.setCursor(0, 36);
  display.printf("VIP  Pass: %-5lu", countVipPass);
  display.setCursor(0, 45);
  display.printf("STD  Pass: %-5lu", countStdPass);
  display.setCursor(0, 54);
  display.printf("Threat Drp: %-5lu", countThreatDrop);

  display.setCursor(75, 54);
  display.printf("CRP:%lu", countCorruptDrop);

  display.display();

  delay(150);
}

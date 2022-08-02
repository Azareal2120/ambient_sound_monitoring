/* Code that reads wireless data, and upon receiving, executes multiple tasks:
 *  - Motor Status
 *  - Battery Status
 *  - Saves sound data
 *  - Prints to serial monitor for debugging
 *  Created by Riley Dean, with ESP-NOW code by Arvind Ravulavaru <https://github.com/arvindr21>.
 */

#include <esp_now.h>
#include <WiFi.h>

// Defining channel for communication
#define CHANNEL 1
#define ALARM_OFF
#define ALARM_ON 1

// Defining sound_data struct
typedef struct sound_data {
  int alarm_mode;
  int sound_level;
} sound_data;

// Init ESP Now with fallback
void InitESPNow() {
  WiFi.disconnect();
  if (esp_now_init() == ESP_OK) {
    Serial.println("ESPNow Init Success");
  }
  else {
    Serial.println("ESPNow Init Failed");
    // Retry InitESPNow, add a counte and then restart?
    // InitESPNow();
    // or Simply Restart
    ESP.restart();
  }
}

// config AP SSID
void configDeviceAP() {
  const char *SSID = "Slave_1";
  bool result = WiFi.softAP(SSID, "Slave_1_Password", CHANNEL, 0);
  if (!result) {
    Serial.println("AP Config failed.");
  } else {
    Serial.println("AP Config Success. Broadcasting with AP: " + String(SSID));
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Program begin");
  
  // Setting ADC resolution
  analogReadResolution(12);
  // Setting Motor Pin direction
  pinMode(3, OUTPUT);

  // Setting pin directions for LEDs
  pinMode(0, OUTPUT);
  pinMode(1, OUTPUT);
  pinMode(2, OUTPUT);
  
  //Set device in AP mode to begin with
  WiFi.mode(WIFI_AP);
  // configure device AP mode
  configDeviceAP();
  // This is the mac address of the Slave in AP Mode
  Serial.print("AP MAC: "); Serial.println(WiFi.softAPmacAddress());
  // Init ESPNow with a fallback logic
  InitESPNow();
  // Once ESPNow is successfully Init, we will register for recv CB to
  // get recv packer info.
  esp_now_register_recv_cb(OnDataRecv);
}

// callback when data is recv from Master
void OnDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len) {
  // Checking battery voltage, and altering LEDs if required
  int batt_voltage = analogReadMilliVolts(2);
  if (batt_voltage < 3400) {
    // Low battery level
    digitalWrite(0, HIGH);
    digitalWrite(1, LOW);
    digitalWrite(2, LOW);
  } else if (batt_voltage < 3600) {
    // Medium Battery Level
    digitalWrite(0, HIGH);
    digitalWrite(1, HIGH);
    digitalWrite(2, LOW);
  } else {
    // High Battery Level
    digitalWrite(0, HIGH);
    digitalWrite(1, HIGH);
    digitalWrite(2, HIGH);
  }

  // Processing the sound data received
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  // Printing for debugging
  Serial.print("Last Packet Recv from: "); Serial.println(macStr);

  // Converting data to sound_data struct
  sound_data *recorded_data = (sound_data *) data;
  if (recorded_data->alarm_mode == 0) {
    Serial.print("No alarm required");
  } else if (recorded_data->alarm_mode >= 1) {
    Serial.print("Alarm required");
    digitalWrite(3, HIGH);
  } else {
    Serial.print("Invalid data read");
  }
  // Printing out serial data
  char output_string[32];
  sprintf(output_string, "Sound = %d dB", recorded_data->sound_level);
  Serial.println(output_string);
  
  Serial.println("");
}

void loop() {
  // Loop occurs earlier, so nothing to be done here :)
}

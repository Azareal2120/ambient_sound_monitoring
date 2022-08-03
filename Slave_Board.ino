/* Code that reads wireless data, and upon receiving, executes multiple tasks:
 *  - Motor Status
 *  - Battery Status
 *  - Saves sound data
 *  - Prints to serial monitor for debugging
 *  
 *  AN interrupt routine is also available that is used to print out the sound data recorded by
 *  the user over the past 256 samples (or less)
 *  Created by Riley Dean, with ESP-NOW code by Arvind Ravulavaru <https://github.com/arvindr21>.
 */

#include <esp_now.h>
#include <WiFi.h>

// Defining channel for communication
#define CHANNEL 1
#define ALARM_OFF
#define ALARM_ON 1

// Defining maximum array size
#define MAX_SIZE 256

// Pin definitions
#define MOTOR_PIN 3
#define LOW_LED 0
#define MED_LED 1
#define HIGH_LED 2
#define INTERRUPT_PIN 6

// Defining sound_data struct
typedef struct sound_data {
  int alarm_mode;
  int sound_level;
} sound_data;

// Defining *shudder* global variables for printing data
int ind = 0;
sound_data *sound_array[MAX_SIZE];

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

// Interrupt handle function
void IRAM_ATTR find_print() {
  Serial.print("Interrupt detected. Printing out recorded sound levels");
  print_sound_data();
}

void setup() {
  Serial.begin(115200);
  Serial.println("Program begin");
  
  // Setting ADC resolution
  analogReadResolution(12);
  // Setting Motor Pin direction
  pinMode(MOTOR_PIN, OUTPUT);\

  // Setting pin directions for LEDs
  pinMode(LOW_LED, OUTPUT);
  pinMode(MED_LED, OUTPUT);
  pinMode(HIGH_LED, OUTPUT);
  
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
  Serial.print("Current battery voltage: ");
  Serial.println(batt_voltage);
  if (batt_voltage < 3400) {
    // Low battery level
    digitalWrite(LOW_LED, HIGH);
    digitalWrite(MED_LED, LOW);
    digitalWrite(HIGH_LED, LOW);
  } else if (batt_voltage < 3600) {
    // Medium Battery Level
    digitalWrite(LOW_LED, HIGH);
    digitalWrite(MED_LED, HIGH);
    digitalWrite(HIGH_LED, LOW);
  } else {
    // High Battery Level
    digitalWrite(LOW_LED, HIGH);
    digitalWrite(MED_LED, HIGH);
    digitalWrite(HIGH_LED, HIGH);
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
    Serial.print("No alarm required\n");
  } else if (recorded_data->alarm_mode >= 1) {
    Serial.print("Alarm required\n");
    alarm_user(recorded_data->alarm_mode);
  } else {
    Serial.print("Invalid data read");
  }
  // Printing out serial data
  char output_string[32];
  sprintf(output_string, "Sound = %d dB\n", recorded_data->sound_level);
  Serial.println(output_string);

  Serial.println("");
}


// Code to print out each element in the array upon an interrupt occurring
void print_sound_data(void) {
  Serial.print("Total recorded sound data: ");
  for (int i = 0; i < ind; i++) {
    char output_string[32];
    sprintf(output_string, "Sound = %d dB", sound_array[i]->sound_level);
    Serial.println(output_string);
    sprintf(output_string, "Alarm Mode: %d", sound_array[i]->alarm_mode);
    Serial.println(output_string);
  }
}

// Function for calculating which mode to print out for the motor
void alarm_user(int alarm_mode) {
  if (alarm_mode == 1) {
    // standard alarm
    digitalWrite(MOTOR_PIN, HIGH);
    delay(1000);
    digitalWrite(MOTOR_PIN, LOW);
  } else if (alarm_mode == 2) {
    // panik
    for (int i = 0; i < 3; i++) {
      digitalWrite(MOTOR_PIN, HIGH);
      delay(200);
      digitalWrite(MOTOR_PIN, LOW);
      delay(200);
    }
  }
}


void loop() {
  // Loop occurs earlier, so nothing to be done here :)
}

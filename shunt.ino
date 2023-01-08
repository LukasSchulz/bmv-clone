/*
 * ToDo:
 * maybe midpoint-voltage
 */


#include <Wire.h>
#include <INA219_WE.h>
#define I2C_ADDRESS 0x40
#include <U8x8lib.h>
#include <EEPROM.h>
#include "Rotary.h"

//Definitions for victron Messages
#define NEWLINE (String) (char) 0x0D + (String) (char) 0x0A
#define TAB (String) (char) 0x09


U8X8_SH1106_128X64_NONAME_HW_I2C display(U8X8_PIN_NONE);

int display_height = 0;
int display_width = 0;

/* There are several ways to create your INA219 object:
   INA219_WE ina219 = INA219_WE()              -> uses Wire / I2C Address = 0x40
   INA219_WE ina219 = INA219_WE(ICM20948_ADDR) -> uses Wire / I2C_ADDRESS
   INA219_WE ina219 = INA219_WE(&wire2)        -> uses the TwoWire object wire2 / I2C_ADDRESS
   INA219_WE ina219 = INA219_WE(&wire2, I2C_ADDRESS) -> all together
   Successfully tested with two I2C busses on an ESP32
*/
INA219_WE ina219 = INA219_WE(I2C_ADDRESS);

//value sizes
/*
 * float: 4 bytes
 * int: 4 bytes
 * long: 4 bytes
 */

int EEPROM_OFFSET = 64;

//Setup internal variables
#define voltage_multiplier_EEPROM_ADDRESS 0
float voltage_multiplier = 30.0;
int measure_interval_ms = 1000;
int display_update_interval_ms = 150;
const int BUTTONPIN = 5;
int rows = 0;
int cols = 0;
int saving_interval_ms = 60000;
#define part_cycles_EEPROM_ADDRESS 4
int part_cycles = 1;

#define correction_factor_EEPROM_ADDRESS 8
float correction_factor = 1.0;


//additional settings
#define screen_timeout_EEPROM_ADDRESS 12
int screen_timeout = 10000; //ms
#define shunt_size_EEPROM_ADDRESS 16
int shunt_size = 500; //in A at 50mV
#define peukert_factor_EEPROM_ADDRESS 20
float peukert_factor = 1.00;
#define high_voltage_mV_EEPROM_ADDRESS 24
float high_voltage_mV = 15000.0;
#define low_voltage_mV_EEPROM_ADDRESS 28
float low_voltage_mV = 10000.0;
#define charge_voltage_mV_EEPROM_ADDRESS 32
float charge_voltage_mV = 14800.0;
#define tail_current_mA_EEPROM_ADDRESS 36
float tail_current_mA = 1000.0;
#define charge_detection_time_EEPROM_ADDRESS 40
int charge_detection_time = 600;
#define charging_efficency_EEPROM_ADDRESS 44
float charging_efficency = 1.00;



String inputString = "";         // a string to hold incoming data
boolean stringComplete = false;  // whether the string is complete


//victron variables
float voltage_mV = 0.0;
float shuntVoltage_mV = 0.0;
float current_mA = 0.0;
float power_W = 0.0;
#define used_capacity_mAh_EEPROM_ADDRESS 48
float used_capacity_mAh = 0.0;
#define capacity_mAh_EEPROM_ADDRESS 52
float capacity_mAh = 0.0;
boolean alarm = false;
boolean relay = false;
byte alarm_reason = 0x00;
String bmv = "700";
String fw = "0308";



#define dodd_mAh_EEPROM_ADDRESS 56
int dodd_mAh = 0; //Depth of deepest Discharge mAh
#define dold_mAh_EEPROM_ADDRESS 60
int dold_mAh = 0; //Depth of Last Discharge mAh
#define average_dod_mAh_EEPROM_ADDRESS 64 
int average_dod_mAh = 0; // Average DoD mAh
#define charge_cycles_EEPROM_ADDRESS 68 
int charge_cycles = 0; // Number of Charge Cycles 
#define full_cycles_EEPROM_ADDRESS 72 
int full_cycles = 0; // Number of full Cycles
#define cumulative_mAh_EEPROM_ADDRESS 76 
long cumulative_mAh = 0; // Cumulative Ah mAh
#define min_voltage_mV_EEPROM_ADDRESS 80 
int min_voltage_mV = 0; // Minimum Voltage mV
#define max_voltage_mV_EEPROM_ADDRESS 84 
int max_voltage_mV = 0; // Maximum Voltage mV
#define time_since_full_EEPROM_ADDRESS 88
float time_since_full = 0; // Number of Seconds since last full charge sec
#define automatic_syncs_EEPROM_ADDRESS 92 
int automatic_syncs = 0; // Number of automatic synchronisations 
#define low_alarms_EEPROM_ADDRESS 96 
int low_alarms = 0; // Number of low main voltage alarams
#define high_alarms_EEPROM_ADDRESS 100 
int high_alarms = 0; // Number of high main voltage alarms
#define discharged_energy_EEPROM_ADDRESS 104 
long discharged_energy = 0; // Amount of discharged Energy 0.01kWh
#define charged_energy_EEPROM_ADDRESS 108 
long charged_energy = 0; // Amount of charged Energy 0.01kWh

#define soc_EEPROM_ADDRESS 116
float soc = 0.0;

#define vrm_name_EEPROM_ADDRESS 512
char vrm_name [20] = {'Z', 'a', 'u', 'b', 'e', 'r', 's', 'h', 'u', 'n', 't', 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

//#define offset_mV_EEPROM_ADDRESS 124
//float offset_mV = 0.0;

void setup() {
  //shunt_size = 500;
  Serial.begin(19200);
  //Serial.println("Starting");
  
  //load variables from eeprom
  EEPROM.get((int)used_capacity_mAh_EEPROM_ADDRESS,used_capacity_mAh);
  EEPROM.get((int)capacity_mAh_EEPROM_ADDRESS,capacity_mAh);
  
  EEPROM.get((int)voltage_multiplier_EEPROM_ADDRESS,voltage_multiplier);
  //Serial.println(voltage_multiplier);
  EEPROM.get((int)correction_factor_EEPROM_ADDRESS,correction_factor);
  //EEPROM.get((int)screen_timeout_EEPROM_ADDRESS,screen_timeout);
  EEPROM.get((int)shunt_size_EEPROM_ADDRESS,shunt_size);
  EEPROM.get((int)peukert_factor_EEPROM_ADDRESS,peukert_factor);
  EEPROM.get((int)high_voltage_mV_EEPROM_ADDRESS,high_voltage_mV);
  EEPROM.get((int)low_voltage_mV_EEPROM_ADDRESS,low_voltage_mV);
  EEPROM.get((int)charge_voltage_mV_EEPROM_ADDRESS,charge_voltage_mV);
  EEPROM.get((int)tail_current_mA_EEPROM_ADDRESS,tail_current_mA);
  //tail_current_mA = 1000;
  EEPROM.get((int)charge_detection_time_EEPROM_ADDRESS,charge_detection_time);
  //Serial.println(EEPROM.get((int)soc_EEPROM_ADDRESS,soc));
  EEPROM.get((int)part_cycles_EEPROM_ADDRESS,part_cycles);
  //charging_efficency = 1.0;
  EEPROM.get((int)charging_efficency_EEPROM_ADDRESS,charging_efficency);
  //Serial.print("charging efficency ");
  //Serial.println(charging_efficency);

  EEPROM.get((int)dodd_mAh_EEPROM_ADDRESS,dodd_mAh); 
  EEPROM.get((int)dold_mAh_EEPROM_ADDRESS,dold_mAh); 
  EEPROM.get((int)average_dod_mAh_EEPROM_ADDRESS,average_dod_mAh); 
  EEPROM.get((int)charge_cycles_EEPROM_ADDRESS,charge_cycles); 
  EEPROM.get((int)full_cycles_EEPROM_ADDRESS,full_cycles);  
  EEPROM.get((int)cumulative_mAh_EEPROM_ADDRESS,cumulative_mAh);  
  EEPROM.get((int)min_voltage_mV_EEPROM_ADDRESS,min_voltage_mV);  
  EEPROM.get((int)max_voltage_mV_EEPROM_ADDRESS,max_voltage_mV);  
  EEPROM.get((int)time_since_full_EEPROM_ADDRESS,time_since_full);  
  EEPROM.get((int)automatic_syncs_EEPROM_ADDRESS,automatic_syncs);  
  EEPROM.get((int)low_alarms_EEPROM_ADDRESS,low_alarms);  
  EEPROM.get((int)high_alarms_EEPROM_ADDRESS,high_alarms);  
  EEPROM.get((int)discharged_energy_EEPROM_ADDRESS,discharged_energy);  
  EEPROM.get((int)charged_energy_EEPROM_ADDRESS,charged_energy);  

  EEPROM.get((int)vrm_name_EEPROM_ADDRESS,vrm_name);  
  
  //EEPROM.get((int)offset_mV_EEPROM_ADDRESS,offset_mV); 
  
  //soc = 99.9;

  
  //rotary encoder
  attachInterrupt(digitalPinToInterrupt(2), rotate, CHANGE);
  attachInterrupt(digitalPinToInterrupt(3), rotate, CHANGE);
  pinMode(BUTTONPIN, INPUT_PULLUP);
  // attachInterrupt(BUTTONPIN, button_click, FALLING);
  
  //display
  display.begin();
  display.setPowerSave(0);
  rows = display.getRows();
  cols = display.getCols();


  //Serial.println("Display loaded");
  //Serial.println("Shunt started");
  
  //Wire.begin();
  if (!ina219.init()) {
    Serial.println("INA219 not connected!");
  }

  /* Set ADC Mode for Bus and ShuntVoltage
    Mode *            * Res / Samples *       * Conversion Time
    BIT_MODE_9        9 Bit Resolution             84 µs
    BIT_MODE_10       10 Bit Resolution            148 µs
    BIT_MODE_11       11 Bit Resolution            276 µs
    BIT_MODE_12       12 Bit Resolution            532 µs  (DEFAULT)
    SAMPLE_MODE_2     Mean Value 2 samples         1.06 ms
    SAMPLE_MODE_4     Mean Value 4 samples         2.13 ms
    SAMPLE_MODE_8     Mean Value 8 samples         4.26 ms
    SAMPLE_MODE_16    Mean Value 16 samples        8.51 ms
    SAMPLE_MODE_32    Mean Value 32 samples        17.02 ms
    SAMPLE_MODE_64    Mean Value 64 samples        34.05 ms
    SAMPLE_MODE_128   Mean Value 128 samples       68.10 ms
  */
  ina219.setADCMode(SAMPLE_MODE_128); // choose mode and uncomment for change of default

  /* Set measure mode
    POWER_DOWN - INA219 switched off
    TRIGGERED  - measurement on demand
    ADC_OFF    - Analog/Digital Converter switched off
    CONTINUOUS  - Continuous measurements (DEFAULT)
  */
  ina219.setMeasureMode(CONTINUOUS); // choose mode and uncomment for change of default

  /* Set PGain
    Gain *  * Shunt Voltage Range *   * Max Current (if shunt is 0.1 ohms)
    PG_40       40 mV                    0.4 A
    PG_80       80 mV                    0.8 A
    PG_160      160 mV                   1.6 A
    PG_320      320 mV                   3.2 A (DEFAULT)
  */
  ina219.setPGain(PG_80); // choose gain and uncomment for change of default

  /* Set Bus Voltage Range
    BRNG_16   -> 16 V
    BRNG_32   -> 32 V (DEFAULT)
  */
  // ina219.setBusRange(BRNG_32); // choose range and uncomment for change of default

  //Serial.println("INA219 Current Sensor Example Sketch - Continuous");

  /* If the current values delivered by the INA219 differ by a constant factor
     from values obtained with calibrated equipment you can define a correction factor.
     Correction factor = current delivered from calibrated equipment / current delivered by INA219
  */
  ina219.setCorrectionFactor(correction_factor); // insert your correction factor if necessary

  /* If you experience a shunt voltage offset, that means you detect a shunt voltage which is not
     zero, although the current should be zero, you can apply a correction. For this, uncomment the
     following function and apply the offset you have detected.
  */
  ina219.setShuntVoltOffset_mV(0.014); // insert the shunt voltage (millivolts) you detect at zero current
  ina219.setShuntSizeInOhms(0.05/shunt_size);
}


unsigned long last_measurement = 0;
unsigned long last_display_update = 0;
int measure_delay = 0;
unsigned long last_victron = 0;
int victron_interval_ms = 1000;

void loop() {
  
 if((millis() - last_measurement) > measure_interval_ms){
    measure_delay = millis() - last_measurement;
    //Serial.println("measuring");
    //Serial.println((String)(millis()));
    last_measurement = millis();
    measureValues();
    last_measurement = millis();
    calculateValues();
    last_measurement = millis();
  }

  if((millis() - last_victron) > victron_interval_ms){  
    sendVictronMessages();
    last_victron = millis();
  }
  

  //if(millis() > last_display_update + display_update_interval_ms){
    updateScreen();
    //Serial.println("Display updated");
  //  last_display_update = millis();
  //}

  if(!digitalRead(BUTTONPIN)){
    button_click();
  }

//  if(stringComplete){
//    if(inputString == ":154"){
//      Serial.println(":501440B"); 
//    }
//  }


}

int screenState = 0;
int menu_item = 0;
boolean item_selected = false;
const int page_count = 2;
const int menu_item_count = 14;
boolean update_menu = true;
void updateScreen() {
  switch (screenState){
    case 0: //display Values
      displayValues();
      break;
    case 1: //display Menu
      if(update_menu){
        displayMenu();
        update_menu = false;
      }
      break;
  }
  
}


float temp_voltage_mV = voltage_mV;
boolean temp_voltage_changed = false;

float temp_current_mA = current_mA;
boolean temp_current_changed = false;

float capacity_temp = capacity_mAh;
boolean capacity_changed = false;

float soc_temp = soc;
boolean soc_changed = false;

float peukert_temp = peukert_factor;
boolean peukert_changed = false;

float high_voltage_temp = high_voltage_mV;
boolean high_voltage_changed = false;

float low_voltage_temp = low_voltage_mV;
boolean low_voltage_changed = false;

float charge_voltage_temp = charge_voltage_mV;
boolean charge_voltage_changed = false;

float tail_current_temp = tail_current_mA;
boolean tail_current_changed = false;

int charge_detection_time_temp = charge_detection_time;
boolean charge_detection_time_changed = false;

int screen_timeout_temp = screen_timeout;
boolean screen_timeout_changed = false;

boolean reset_values = false;

float temp_current_mA_saved = 0.0;

void saveSettings(){
  //load variables from eeprom
  EEPROM.put((int)screen_timeout_EEPROM_ADDRESS,screen_timeout);
  EEPROM.put((int)shunt_size_EEPROM_ADDRESS,shunt_size);
  EEPROM.put((int)peukert_factor_EEPROM_ADDRESS,peukert_factor);

  

  if(temp_voltage_changed){
    voltage_multiplier = 1023.0 * (temp_voltage_mV / analogRead(A0));
    EEPROM.put((int) voltage_multiplier_EEPROM_ADDRESS,voltage_multiplier);
    Serial.println(F("v s"));
    temp_voltage_changed = false;
  }
  if(temp_current_changed){
    
    
    correction_factor =  (temp_current_mA*1.0)/((temp_current_mA_saved*1.0)/(correction_factor*1.0));
    //correction_factor = 1.0;
    ina219.setCorrectionFactor(correction_factor);
    EEPROM.put((int)correction_factor_EEPROM_ADDRESS,correction_factor);
    Serial.println(F("c s"));
  
    temp_current_changed = false;
  }
  if(soc_changed){
    soc = soc_temp;
    EEPROM.put((int)soc_EEPROM_ADDRESS,soc);
    Serial.println(F("s s"));
    soc_changed = false;
  }
  if(capacity_changed){
    capacity_mAh = capacity_temp;
    EEPROM.put((int)capacity_mAh_EEPROM_ADDRESS,capacity_mAh);
    Serial.println(F("c s"));
    capacity_changed = false;
  }
  if(peukert_changed) {
    peukert_factor = peukert_temp;
    EEPROM.put((int)peukert_factor_EEPROM_ADDRESS,peukert_factor);
    Serial.println(F("p s"));
    peukert_changed = false;
  }
  if(high_voltage_changed){
    high_voltage_mV = high_voltage_temp;
    EEPROM.put((int)high_voltage_mV_EEPROM_ADDRESS,high_voltage_mV);
    Serial.println(F("hv s"));
    high_voltage_changed = false;
  }
  if(low_voltage_changed){
    low_voltage_mV = low_voltage_temp;
    EEPROM.put((int)low_voltage_mV_EEPROM_ADDRESS,low_voltage_mV);
    Serial.println(F("lv s"));
    low_voltage_changed = false;
  }
  if(charge_voltage_changed){
    charge_voltage_mV = charge_voltage_temp;
    EEPROM.put((int)charge_voltage_mV_EEPROM_ADDRESS,charge_voltage_mV);
    Serial.println(F("c s"));
    charge_voltage_changed = false;
  }
  if(tail_current_changed){
    tail_current_mA = tail_current_temp;
    EEPROM.put((int)tail_current_mA_EEPROM_ADDRESS,tail_current_mA);
    Serial.println(F("tc s"));
    tail_current_changed = false;
  }
  if(charge_detection_time_changed){
    charge_detection_time = charge_detection_time_temp;
    EEPROM.put((int)charge_detection_time_EEPROM_ADDRESS,charge_detection_time);
    Serial.println(F("cd s"));
    charge_detection_time_changed = false;
  }

  

  EEPROM.put((int)shunt_size_EEPROM_ADDRESS,shunt_size); 
  
  if(reset_values){
    dodd_mAh = 0;
    dold_mAh = 0;
    average_dod_mAh = 0;
    charge_cycles = 0;
    full_cycles = 0;
    cumulative_mAh = 0;
    min_voltage_mV = 0;
    max_voltage_mV = 0;
    time_since_full = 0;
    automatic_syncs = 0;
    low_alarms = 0;
    high_alarms = 0;
    discharged_energy = 0;
    charged_energy = 0;
    charging_efficency = 1.0;
    saveValues();
  }
  
  
  
  
  
  
  
  
  

}

boolean font_changed = false;
void displayValues() {
  if(!font_changed){
    display.setFont(u8x8_font_amstrad_cpc_extended_r);
    font_changed = true;
  }
  //display.clear();
  display.setInverseFont(0);
  display.drawString(0, 0, ((String) (" U: " + (String) (voltage_mV/1000) + "V     ")).c_str());
  display.drawString(0, 2, ((String) (" SoC: " + (String) soc + "%      ")).c_str());
  display.drawString(0, 4, ((String) (" I: " + (String) (current_mA / 1000)) + "A      ").c_str());
  display.drawString(0, 6, ((String) (" P: " + (String) (power_W) + "W     ")).c_str());
}


/**
 * Menu items:
 * 
 * calibrate Voltage
 * calibrate current
 * set capacity
 * set shunt size (500, 1000, 2000)
 * set soc
 * peukert        float peukert_factor
 * high-voltage   float high_voltage_mV
 * low-voltage    float low_voltage_mV 
 * charge-voltage float charge_voltage_mV
 * tail-current   float tail_current_mA
 * charge-detection-time  int charge_detection_time
 * screen timeout int screen_timeout (in s)
 * save
 * back
 */

void displayMenu(){
  if(!font_changed){
    display.setFont(u8x8_font_amstrad_cpc_extended_r);
    font_changed = true;
  }

  

display.clearDisplay();
//display.drawString(8, 0, ((String) menu_item).c_str());
  int item = 0;
//calibrate voltage
  if(!(item - menu_item > rows - 2) && item - menu_item >= 0){
    display.setInverseFont(!item_selected && menu_item == item);
    display.drawString(0, item - menu_item, "calib. V");
    display.setInverseFont(item_selected && menu_item == item);
    display.drawString(10, item - menu_item, ((String) (String(temp_voltage_mV/1000, 2) + "V")).c_str());

  }
      item += 1;
//calibrate current
  if(!(item - menu_item > rows - 2) && item - menu_item >= 0){
    display.setInverseFont(!item_selected && menu_item == item);
    display.drawString(0, item - menu_item, "calib. A");
    display.setInverseFont(item_selected && menu_item == item);
    display.drawString(10, item - menu_item, ((String) (String(temp_current_mA/1000, 2) + "A")).c_str());

  }
  item += 1;
//set capacity
  if(!(item - menu_item > rows - 2) && item - menu_item >= 0){
    display.setInverseFont(!item_selected && menu_item == item);
    display.drawString(0, item - menu_item, "Capacity");
    display.setInverseFont(item_selected && menu_item == item);
    display.drawString(10, item - menu_item, ((String) ((String) (int) (capacity_temp / 1000)) + "Ah").c_str());
  }
  item += 1;
//set shunt size
  if(!(item - menu_item > rows - 2) && item - menu_item >= 0){
    display.setInverseFont(!item_selected && menu_item == item);
    display.drawString(0, item - menu_item, "Shunt s.");
    display.setInverseFont(item_selected && menu_item == item);
    display.drawString(10, item - menu_item, ((String) ((int) shunt_size) + "A").c_str());
  }
  item += 1;
//set soc
  if(!(item - menu_item > rows - 2) && item - menu_item >= 0){
    display.setInverseFont(!item_selected && menu_item == item);
    display.drawString(0, item - menu_item, "SoC");
    display.setInverseFont(item_selected && menu_item == item);
    display.drawString(10, item - menu_item, ((String) (String(soc_temp, 1) + "%")).c_str());
  }
  item += 1;
//set peukert
  if(!(item - menu_item > rows - 2) && item - menu_item >= 0){
    display.setInverseFont(!item_selected && menu_item == item);
    display.drawString(0, item - menu_item, "Peukert");
    display.setInverseFont(item_selected && menu_item == item);
    display.drawString(10, item - menu_item, ((String) (String(peukert_temp, 2) + "")).c_str());
  }
  item += 1;
//set high-voltage
  if(!(item - menu_item > rows - 2) && item - menu_item >= 0){
    display.setInverseFont(!item_selected && menu_item == item);
    display.drawString(0, item - menu_item, "High V.");
    display.setInverseFont(item_selected && menu_item == item);
    display.drawString(10, item - menu_item, ((String) (String((high_voltage_temp/1000.0), 2) + "V")).c_str());
  }
  item += 1;
//set low-voltage
  if(!(item - menu_item > rows - 2) && item - menu_item >= 0){
    display.setInverseFont(!item_selected && menu_item == item);
    display.drawString(0, item - menu_item, "Low V.");
    display.setInverseFont(item_selected && menu_item == item);
    display.drawString(10, item - menu_item, ((String) (String((low_voltage_temp/1000.0), 2) + "V")).c_str());
  }
  item += 1;
//set charge-voltage
  if(!(item - menu_item > rows - 2) && item - menu_item >= 0){
    display.setInverseFont(!item_selected && menu_item == item);
    display.drawString(0, item - menu_item, "Chrg V.");
    display.setInverseFont(item_selected && menu_item == item);
    display.drawString(10, item - menu_item, ((String) (String((charge_voltage_temp/1000.0), 2) + "V")).c_str());
  } 
  item += 1;
//set tail-current
  if(!(item - menu_item > rows - 2) && item - menu_item >= 0){
    display.setInverseFont(!item_selected && menu_item == item);
    display.drawString(0, item - menu_item, "Tail I");
    display.setInverseFont(item_selected && menu_item == item);
    display.drawString(10, item - menu_item, ((String) (String((tail_current_temp/1000.0), 2) + "A")).c_str());
  }
  item += 1;
//set charge-detection-time
  if(!(item - menu_item > rows - 2) && item - menu_item >= 0){
    display.setInverseFont(!item_selected && menu_item == item);
    display.drawString(0, item - menu_item, "Charg. T.");
    display.setInverseFont(item_selected && menu_item == item);
    display.drawString(10, item - menu_item, ((String) (String((charge_detection_time_temp/1.0)) + "s")).c_str());
  }
  item += 1;
//reset_values
  if(!(item - menu_item > rows - 2) && item - menu_item >= 0){
    display.setInverseFont(!item_selected && menu_item == item);
    display.drawString(0, item - menu_item, "Reset.");
    display.setInverseFont(item_selected && menu_item == item);
    display.drawString(10, item - menu_item, reset_values ? "yes" : "no");
  }
  item += 1;
//save
  display.setInverseFont(menu_item == menu_item_count - 2);
  display.drawString(1, (int) (rows -1) , "Save");
//back
  display.setInverseFont(menu_item == menu_item_count - 1);
  display.drawString((int) (cols - 6), (int) (rows -1), "Back");
  
}


void measureValues() {
  shuntVoltage_mV = ina219.getShuntVoltage_mV();
  current_mA = ina219.getCurrent_mA();
  voltage_mV = analogRead(A0) * voltage_multiplier / 1023.0;
}

unsigned long charge_detection_timer = millis();
boolean tail_current_reached = false;
unsigned long last_save = millis();

boolean discharging_stopped = true;
boolean full_discharge = false;
boolean charging = false;
boolean low_alarm_triggered = false;
boolean high_alarm_triggered = false;
boolean synced = false;
float last_charged_capacity = 0;

void calculateValues() {

  

  power_W = (current_mA/1000.0) * (voltage_mV/1000.0);
  used_capacity_mAh += + current_mA*(measure_delay/(1000.0*3600.0));
//  Serial.println(measure_delay);
//  Serial.println(String((float) measure_delay/(float) (1000.0*3600.0),10));
//  Serial.print("capacity ");
//  Serial.println(capacity_mAh);
//  Serial.println(String(((float) ((float) measure_delay/(float)(1000.0*3600.0*capacity_mAh))),10));
//  Serial.println(String(((float) current_mA*((float) measure_delay/(float)(1000.0*3600.0*capacity_mAh)))*charging_efficency,10));
//  Serial.println(String((current_mA > 0) ? + ((float) current_mA*((float) measure_delay/(float)(1000.0*3600.0*capacity_mAh)))*charging_efficency : - pow(abs(current_mA*1.0)*(measure_delay/(1000.0*3600.0)), peukert_factor)/(capacity_mAh*1.0),10));
  //Serial.println(((float) current_mA*((float) measure_delay/(float)(1000.0*3600.0*capacity_mAh)))*charging_efficency*100 );
  soc += ((current_mA > 0) ? + ((float) current_mA*((float) measure_delay/(float)(1000.0*3600.0*capacity_mAh)))*charging_efficency : - pow( abs(current_mA)*(measure_delay/(1000.0*3600.0)), peukert_factor)/ capacity_mAh)*100;
  alarm_reason = 0x00;
  if(voltage_mV > high_voltage_mV){
    high_alarm_triggered = true;
    alarm_reason += 1;
  }
  if(voltage_mV < low_voltage_mV){
    low_alarm_triggered = true;
    alarm_reason += 2;
  }

  alarm = low_alarm_triggered || high_alarm_triggered;

  if(voltage_mV<high_voltage_mV){
    high_alarms += (high_alarm_triggered) ? 1 : 0; 
    high_alarm_triggered = false;
  }
  if(voltage_mV > low_voltage_mV){
    low_alarms += (low_alarm_triggered) ? 1 : 0;
    low_alarm_triggered = false;
  }

  if(used_capacity_mAh < dodd_mAh){
    dodd_mAh = used_capacity_mAh;
  }
  if(power_W < 0){
    dold_mAh = used_capacity_mAh;
  }

  if(power_W > 0 && !discharging_stopped){
    discharging_stopped = true;
    average_dod_mAh = (average_dod_mAh/part_cycles + used_capacity_mAh/(part_cycles +1))/2;
    part_cycles += 1;
  }
  if(soc >= 100 && charging){
    charge_cycles += 1;
    charging = false;
    if(full_discharge){
      full_cycles += 1;
    }
  }

  if(soc < 90) {
    charging = true;
    synced = false;
  }

  min_voltage_mV = (voltage_mV < min_voltage_mV) ? voltage_mV : min_voltage_mV;
  max_voltage_mV = (voltage_mV > max_voltage_mV) ? voltage_mV : max_voltage_mV;
  
  cumulative_mAh += current_mA*((float)measure_delay/(1000.0*3600.0));

  time_since_full = (soc == 100) ? 0 : time_since_full +measure_delay/1000;
  //Serial.print("soc ");
  //Serial.println(soc);
  if(voltage_mV > charge_voltage_mV && tail_current_mA > current_mA && current_mA >= 0 && !tail_current_reached){
    //Serial.println("tail reached");
    charge_detection_timer = millis();
    tail_current_reached = true;
  }

  if(tail_current_mA < current_mA || voltage_mV < charge_voltage_mV || current_mA < 0){
    //Serial.println("tail reset");
    charge_detection_timer = millis();
    tail_current_reached = false;
  }

  if(voltage_mV > charge_voltage_mV && tail_current_mA >= current_mA && current_mA >= 0 && (millis() - charge_detection_timer) > charge_detection_time*1000 ){
    //Serial.println("recalibrating");
    soc = 100;
    if(!synced){
      automatic_syncs += 1;
      synced = true;
    }
    //Serial.println("last_cap " + (String) last_charged_capacity + "dold " + (String) dold_mAh);
    //Serial.println(-dold_mAh / (last_charged_capacity*1000.0));
    if(-dold_mAh / (last_charged_capacity*1000.0) < 10 && -dold_mAh / (last_charged_capacity*1000.0) > 0.4){
      charging_efficency = (float) -dold_mAh*1.0 / (last_charged_capacity*1000.0);
      EEPROM.put((int)charging_efficency_EEPROM_ADDRESS,charging_efficency);
    }
    //Serial.println("chrg_eff " + (String) charging_efficency);
    
    last_charged_capacity = 0;
    used_capacity_mAh = 0;
    tail_current_reached = false;
    charge_detection_timer = millis();
  }

  discharged_energy += (power_W < 0) ?(float) (power_W/10.0)*(measure_delay/(1000.0*3600.0)) : 0;
  charged_energy += (power_W > 0) ? (float) (power_W/10.0)*(measure_delay/(1000.0*3600.0)) : 0;
  last_charged_capacity += (current_mA > 0) ? + (float) (current_mA)*(measure_delay/(1000.0*3600.0)) : 0;
  




  
  if(millis() > last_save + saving_interval_ms){
    saveValues();
  }
}

void saveValues() {
  EEPROM.put((int)dodd_mAh_EEPROM_ADDRESS,dodd_mAh); 
  EEPROM.put((int)dold_mAh_EEPROM_ADDRESS,dold_mAh); 
  EEPROM.put((int)average_dod_mAh_EEPROM_ADDRESS,average_dod_mAh); 
  EEPROM.put((int)charge_cycles_EEPROM_ADDRESS,charge_cycles); 
  EEPROM.put((int)full_cycles_EEPROM_ADDRESS,full_cycles);  
  EEPROM.put((int)cumulative_mAh_EEPROM_ADDRESS,cumulative_mAh);  
  EEPROM.put((int)min_voltage_mV_EEPROM_ADDRESS,min_voltage_mV);  
  EEPROM.put((int)max_voltage_mV_EEPROM_ADDRESS,max_voltage_mV);  
  EEPROM.put((int)time_since_full_EEPROM_ADDRESS,time_since_full);  
  EEPROM.put((int)automatic_syncs_EEPROM_ADDRESS,automatic_syncs);  
  EEPROM.put((int)low_alarms_EEPROM_ADDRESS,low_alarms);  
  EEPROM.put((int)high_alarms_EEPROM_ADDRESS,high_alarms);  
  EEPROM.put((int)discharged_energy_EEPROM_ADDRESS,discharged_energy);  
  EEPROM.put((int)charged_energy_EEPROM_ADDRESS,charged_energy);
  EEPROM.put((int)charging_efficency_EEPROM_ADDRESS,charging_efficency);
  
  last_save = millis();
}

void sendVictronMessages() {
  /*
     Beschreibung der Variablen:
     V:     Spannung der Batterie         in mV
     I:     Strom der Batterie            in mA
     T:     Temperatur der Batterie       in °C
     P:     Momentane Leistung            in W
     CE:    Verbrauchter Strom            in mAh
     SOC:   State of Charge               in ‰
     TTG:   Time-to-Go                    in min    //when not discharging -1
     Alarm: Alarm Aktiv                   OFF/ON
     Relay: Relay state                   OFF/ON
     AR:    Alarm Reason                  siehe unten
     H1:    Depth of Deepest Discharge    in mAh
     H2:    Depth of the last Discharge   in mAh
     H3:    Average DoD                   in mAh
     H4:    Number of Charge Cycles
     H5:    Number of full Cycles
     H6:    Cumulative Ah drawn           in mAh
     H7:    Minimum Voltage               in mV
     H8:    Maximum Voltage               in mV
     H9:    Number of Seconds since last full charge in sec
     H10:   Number of automatic synchronisations
     H11:   Number of low main voltage alarms
     H12:   Number of high main voltage alarms
     H13:   aux stuff
     H14:   aux
     H15:   aux
     H16:   aux
     H17:   Amount of discharged Energy in 0.01 kWh (10Wh)
     H18:   Amount of charged Energy    in 0.01 kWh (10Wh)
     H19:   mppt
     H20:   mppt
     ...
     BMV:   Model description (deprecated)
     FW:    Firmware version
     PID:   Product ID


     Infos zu AR (Alarm Reason)
     1:   Low Voltage
     2:   High Voltage
     4:   Low SoC
     8:   Low Starter Voltage
     16:  High Starter Voltage
     32:  Low Temperature
     64:  High Temperature
     128: Mid Voltage

     Sendezyklus:
     Alles außer H{n
     Checksum
     Dann alle H{n}
     Checksum

  */
  String message = "";
  //Allgemeine Infos
  message += "\x0D\nPID\t0x203";
  message += "\x0D\nV\t" + (String) (int) voltage_mV;
  message += "\x0D\nI\t" + (String) (int) current_mA;
  message += "\x0D\nP\t" + (String) (int) power_W;
  message += "\x0D\nCE\t" + (String) (int) used_capacity_mAh;
  message += "\x0D\nSOC\t" + (String) (int) (float) (soc*10.0);
  message += "\x0D\nTTG\t" + (String) (int) ((current_mA < 0) ? -((capacity_mAh - used_capacity_mAh)*1.0 / pow(current_mA*1.0, peukert_factor)) * 60 : -1);
  message += "\x0D\nALARM\t" + (String) (alarm ? "ON" : "OFF");
  message += "\x0D\nRELAY\t" + (String) (relay ? "ON" : "OFF");
  message += "\x0D\nAR\t" + (String) (int) alarm_reason;
  message += "\x0D\nBMV\t" + (String) bmv;
  message += "\x0D\nFW\t" + (String) fw;
  //byte message_bytes[message.length()];
  //message.getBytes(message_bytes, message.length());
  //byte checksum = calcChecksum(message_bytes);
  //byte checksum = calcChecksum(message);
  message += "\x0D\nChecksum\t";
  uint8_t checksum = 0;
  for (int i = 0; i < message.length(); i++) {
    //Serial.println("summing");
    checksum = (checksum - (byte) message.charAt(i));
  }
  Serial.write(message.c_str());
  Serial.write(checksum);
  //Serial.print(checksum, HEX);
  //Serial.print("\x0D\n");
  

  //Spezielle infos
  message = "";
  message += "\x0D\nH1\t" + (String) + dodd_mAh; //Depth of deepest Discharge
  message += "\x0D\nH2\t" + (String) + dold_mAh; //Depth of Last Discharge
  message += "\x0D\nH3\t" + (String) + average_dod_mAh; // Average DoD
  message += "\x0D\nH4\t" + (String) + charge_cycles; // Number of Charge Cycles
  message += "\x0D\nH5\t" + (String) + full_cycles; // Number of full Cycles
  message += "\x0D\nH6\t" + (String) + cumulative_mAh; // Cumulative Ah drawn
  message += "\x0D\nH7\t" + (String) + min_voltage_mV; // Minimum Voltage
  message += "\x0D\nH8\t" + (String) + max_voltage_mV; // Maximum Voltage
  message += "\x0D\nH9\t" + (String) + (int) time_since_full; // Number of Seconds since last full charge
  message += "\x0D\nH10\t" + (String) + automatic_syncs; // Number of automatic synchronisations
  message += "\x0D\nH11\t" + (String) + low_alarms; // Number of low main voltage alarams
  message += "\x0D\nH12\t" + (String) + high_alarms; // Number of high main voltage alarms
  message += "\x0D\nH17\t" + (String) + discharged_energy; // Amount od discharged Energy
  message += "\x0D\nH18\t" + (String) + charged_energy; // Amount of charged Energy
  //byte message_bytes_2[message.length()];
  //message.getBytes(message_bytes_2, message.length());
  //byte checksum_2 = calcChecksum(message_bytes_2);
  
  //byte checksum_2 = calcChecksum(message);
  
  message += "\x0D\nChecksum\t";
  checksum = 0;
  for (int i = 0; i < message.length(); i++) {
    //Serial.println("summing");
    checksum = (checksum - (byte) message.charAt(i));
  }
  //checksum = calcChecksum((String) message);
  Serial.write(message.c_str());
  Serial.write(checksum);
  //Serial.print(checksum, HEX);
  //Serial.print("\x0D\n");



}


//byte calcChecksum(byte message []) {
//  byte checksum = 0x00;
//  for (int i = 0; i < sizeof(message) / sizeof(message[0]); i++) {
//    checksum -= message[i];
//  }
//  return checksum;
//}

//byte calcChecksum(String input) {
//  char checksum = 0;
//  for (int i = 0; i < input.length(); i++) {
//    checksum += input.charAt(i);
//  }
//  //checksum -= 139;
//  checksum = 0 - checksum;
//  return (checksum);
//}

byte calcChecksum(String input) {
  char checksum = 0;
  Serial.println("calculating checksum");
  Serial.println("input string: ");
  Serial.print(input);
  Serial.println("end of input");
  for (int i = 0; i < input.length(); i++) {
    Serial.println("summing");
    checksum = (checksum - (byte) input.charAt(i));
  }
  //checksum -= 139;
  return (checksum);
}

//rotary Encoder
Rotary rotary = Rotary(2, 3);


/**
 * Menu items:
 * 
 * calibrate Voltage
 * calibrate current
 * set capacity
 * set shunt size (500, 1000, 2000)
 * set soc
 * peukert        float peukert_factor
 * high-voltage   float high_voltage_mV
 * low-voltage    float low_voltage_mV 
 * charge-voltage float charge_voltage_mV
 * tail-current   float tail_current_mA
 * charge-detection-time  int charge_detection_time
 * screen timeout int screen_timeout (in s)
 * save
 * back
 */
void rotate() {
  //Serial.println("rotating!");
  unsigned char result = rotary.process();
  boolean cw;
  if(result == DIR_CW){
    //Serial.println("cw");
    cw = true;
  } else if (result == DIR_CCW){
    //Serial.println("ccw");
    cw = false;
  } else {
    return;
  }

  //Serial.println(update_menu);

  switch(screenState){
    case 0:
      break;
    case 1:
      update_menu = true;
      if(item_selected){
        switch(menu_item){
          case 0: // voltage calib
            temp_voltage_mV += cw ? + 10 : - 10;
            temp_voltage_changed = true;
            break;
          case 1: // current calib
            temp_current_mA += cw ?  10 :  -10;
            temp_current_changed = true;
            break;
          case 2: // capacity
            //Serial.println("here!");
            capacity_temp += cw ? 1000 : (capacity_temp > 0) ? -1000 : 0;
            //capacity_temp += 1000;
            //Serial.println(capacity_temp);
            //Serial.println(((String) ((String) (int) (capacity_temp / 1000)) + "Ah").c_str());
            capacity_changed = true;
            break;
          case 3: // shunt size
            switch(shunt_size){
              case 500:
                shunt_size = cw ? 1000 : 2000;
                break;
              case 1000:
                shunt_size = cw ? 2000 : 500;
                break;
              case 2000:
                shunt_size = cw ? 500 : 1000;
                break;
              default:
                shunt_size = 500;
                break;
            }
            break;
          case 4: // set soc
            soc_temp += cw ? ((soc_temp < 100) ? +1 : 0) : ((soc_temp > 0) ? - 1 : 0);
            soc_changed = true;
            break;
          case 5: // peukert
            peukert_temp += cw ? + 0.01 : (peukert_temp > 0) ? - 0.01 : 0;
            peukert_changed = true;
            break;
          case 6:
            high_voltage_temp += cw ? + 10.0 : (high_voltage_temp > 0.0) ?  - 10.0 : 0.0;
            high_voltage_changed = true;
            break;
          case 7:
            low_voltage_temp += cw ?  + 10.0 : (low_voltage_temp > 0.0) ?  - 10.0 : 0.0;
            low_voltage_changed = true;
            break;
          case 8:
            charge_voltage_temp += cw ?  + 10.0 : (charge_voltage_temp > 0.0) ?  - 10.0 : 0.0;
            charge_voltage_changed = true;
            break;
          case 9:
            tail_current_temp += cw ?  + 10.0 : (tail_current_temp > 0.0) ?  - 10.0 : 0.0;
            tail_current_changed = true;
            break;
          case 10:
            charge_detection_time_temp += cw ?  + 10 : (charge_detection_time_temp > 0) ?  - 10 : 0;
            charge_detection_time_changed = true;
            break;
          case 11: // reset
            reset_values = !reset_values;
            break;
        }
      } else {
        menu_item = cw ? (menu_item + 1) : ((menu_item > 0) ? menu_item - 1: menu_item_count -1);
        menu_item %= menu_item_count;
      }
      break;
  }
}

//button
//int button_start = millis();
unsigned long last_pressed = millis();
int press_delay = 500;
//int button_delay = 200;
boolean button_active = true;
void button_click(){
  //Serial.println("Button clicked!");
  
  //if(button_active){
  if((millis() - last_pressed ) > press_delay){
    //Serial.println("Button click registered");
    //checkPress();
    //button_start = millis();
    
    switch(screenState){
      case 0:
        // update values for the menu
        temp_voltage_mV = voltage_mV;
        ina219.setCorrectionFactor(1.0);
        temp_current_mA = ina219.getCurrent_mA();
        temp_current_mA_saved = ina219.getCurrent_mA();
        ina219.setCorrectionFactor(correction_factor);
        soc_temp = soc;
        capacity_temp = capacity_mAh;
        peukert_temp = peukert_factor;
        high_voltage_temp = high_voltage_mV;
        low_voltage_temp = low_voltage_mV;
        charge_voltage_temp = charge_voltage_mV;
        tail_current_temp = tail_current_mA;
        charge_detection_time_temp = charge_detection_time;
        screen_timeout_temp = screen_timeout;
        
        menu_item = 0;
        screenState += 1;
        update_menu = true;
        display.clearDisplay();
        font_changed = false;
        break;
      case 1:
        update_menu = true;
        switch(menu_item){
          case 0:
          case 1:
          case 2:
          case 3:
          case 4:
          case 5:
          case 6:
          case 7:
          case 8:
          case 9:
          case 10:
          case 11:
            item_selected = !item_selected;
            break;
          case menu_item_count -2 :
            saveSettings();
            screenState +=1;
            screenState %= page_count;
            display.clearDisplay();
            font_changed = false;
            break;
          case menu_item_count -1:
            screenState += 1;
            screenState %= page_count;
            display.clearDisplay();
            font_changed = false;
            break;
        }
        break;
    }
    
    //button_delay += 200;
    //button_active = !button_active;
  }
  last_pressed = millis();
}



void serialEvent() {
  //Serial.println("!!!!!!!! ----- Serial received!");
  //negotiating = true;
    while (Serial.available()) {
        // get the new byte:
        char inChar = (char)Serial.read();
        //Serial.println("resp: "+inChar);
        //Serial.println(inChar, HEX);
        // add it to the inputString:
        inputString += inChar;
        if(inChar == ':'){ //begin of command
          inputString = "";
        }
        // if the incoming character is a newline, set a flag
        // so the main loop can do something about it:
        if (inChar == '\n') {
            //Serial.println(inputString);
            //stringComplete = true;
            if (inputString.indexOf("154") > -1){ //get device type
              //Serial.println("We are here!");
              
              Serial.print(":501440B");
              Serial.print("\n");
              //send_device_info = true;
              
            }
            if (inputString.indexOf("70C010041")>-1){ //get device description
              String comm = "70C0100";
              for(int  i = 0; i<20; i++){
                String letter = String(vrm_name[i], HEX);
                
                if (letter.length() == 1){
                  letter = "0" + letter;
                }
                letter.toUpperCase();
                comm += letter;
                  //comm += "41";
              }
              Serial.print(":");
              Serial.print(comm);
              if(checksum(comm)<16){
                Serial.print("0");
              }
              Serial.print(checksum(comm), HEX);
              Serial.print("\n");
            }
            if (inputString.indexOf("80C0100") > -1){ //set device description+
              int offset = inputString.indexOf("80C0100");
              int name_length = inputString.length();
              
              //Serial.println("input_string: " + inputString);
              for( int i = 0; i<40; i+=2){
                char c = 16*convertToDec(inputString.charAt(offset+7+i)) + convertToDec(inputString.charAt(offset+8+i));
                
                //vrm_name[i/2] = (isAlphaNumeric(c) || c == ' ')  ? c : 0x00;
                vrm_name[i/2] = (isAscii(c) || c == ' ')  ? c : 0x00;
              }
              EEPROM.put((int)vrm_name_EEPROM_ADDRESS,vrm_name); 
              String comm = "80C0100";
              for(int  i = 0; i<20; i++){
                String letter = String(vrm_name[i], HEX);
                
                if (letter.length() == 1){
                  letter = "0" + letter;
                }
                letter.toUpperCase();
                comm += letter;
                  //comm += "41";
              }
              Serial.print(":");
              Serial.print(comm);
              if(checksum(comm)<16){
                Serial.print("0");
              }
              Serial.print(checksum(comm), HEX);
              Serial.print("\n");
            }
            if (inputString.indexOf("704010049")>-1){ //PV-Stuff, not supported, "group id"
              String comm = "70401";
              comm += "00"; //not supported response - but seems to only work when enabled
              comm += "00"; //grou id 00
              Serial.print(":");
              Serial.print(comm);
              if(checksum(comm)<16){
                Serial.print("0");
              }
              Serial.print(checksum(comm), HEX);
              Serial.print("\n");
            }
            
            if (inputString.indexOf("74F0300FC")>-1){ //relay mode

              String comm = "74F03";
              comm += "00"; //response ok
              comm += "00"; //value
              Serial.print(":");
              Serial.print(comm);
              if(checksum(comm)<16){
                Serial.print("0");
              }
              Serial.print(checksum(comm), HEX);
              //Serial.print(":74F0300FC");
              Serial.print("\n");
            }
            if (inputString.indexOf("7B8EE00A8")>-1){
                
             String comm = "7B8EE";
              comm += "0100"; //response mode, setting
              Serial.print(":");
              Serial.print(comm);
              if(checksum(comm)<16){
                Serial.print("0");
              }
              Serial.print(checksum(comm), HEX);
              //Serial.print(":7B8EE00A8");
              Serial.print("\n");
            }
            if (inputString.indexOf("70A010043")>-1){ //get device serial number
              //digitalWrite(LED_BUILTIN, HIGH);
              String comm = "70A01";
              comm += "0049513134343135495A4657";
              for(int  i = 0; i<22; i++){
                comm += "00";
              }
              Serial.print(":");
              Serial.print(comm);
              if(checksum(comm)<16){
                Serial.print("0");
              }
              Serial.print(checksum(comm), HEX);
              Serial.print("\n");
                //Serial.print(":70A010048513134343044345135522C48513134343135495A46572C0D0A0000000000004A\n");
            }
            
            
//            switch(inputString.charAt(1)){
//              case '5':
//                command_position = 1;
//                break;
//              case '1':
//                Serial.print(":501440B");
//                Serial.print("\n");
//                break;
//            }
            inputString = "";
            
        }
    }

}

byte checksum(String input) {
  byte checksum = 0x55;
  byte command_ = convertToDec(input.charAt(0));
  checksum -= command_;
  //Serial.println(command_);
  for (int i = 1; i < input.length(); i+=2) {
     //Serial.println(convertToDec(input.charAt(i)));
     //Serial.println(convertToDec(input.charAt(i+1)));
     byte bytes = 16*convertToDec(input.charAt(i))+convertToDec(input.charAt(i+1));
     //Serial.println(bytes, HEX);
     checksum = (checksum - bytes) & 255;
  }  
  return (checksum);

}

int convertToDec(char c){
  int x  = 0;
  switch(c){
    case '0':
      x++;
    case '1':
      x++;
    case '2':
      x++;
    case '3':
      x++;
    case '4':
      x++;
    case '5':
      x++;
    case '6':
      x++;
    case '7':
      x++;
    case '8':
      x++;
    case '9':
      x++;
    case 'A':
    case 'a':
      x++;
    case 'B':
    case 'b':
      x++;
    case 'C':
    case 'c':
      x++;
    case 'D':
    case 'd':
      x++;
    case 'E':
    case 'e':
      x++;
    case 'F':
    case 'f':
    break;
  }

  return 15-x;
      
      
}


//Double press check
//int button_long_press_delay = 500;
//void checkPress() {
//  if((millis() - button_start < button_long_press_delay)){
//     Serial.println("Double press detected");
//  }
//}

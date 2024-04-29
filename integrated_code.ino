#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

#include "DHT.h"
#define RST_PIN  9 
#define SS_PIN   10
#define MQ_PIN                       (0)
#define DHTPIN 4 


#define RL_VALUE                     (5)
#define RO_CLEAN_AIR_FACTOR          (9.83)
#define CALIBARAION_SAMPLE_TIMES     (50)
#define CALIBRATION_SAMPLE_INTERVAL  (500)
#define READ_SAMPLE_INTERVAL         (50)
#define READ_SAMPLE_TIMES            (5)
#define GAS_LPG                      (0)
#define GAS_CO                       (1)
#define GAS_SMOKE                    (2)
#define DHTTYPE DHT22  

MFRC522 mfrc522(SS_PIN, RST_PIN);
DHT dht(DHTPIN, DHTTYPE); 
LiquidCrystal_I2C lcd(0x27,16,2) ;

float LPGCurve[3]  =  {2.3, 0.21, -0.47};
float COCurve[3]  =  {2.3, 0.72, -0.34};
float SmokeCurve[3] = {2.3, 0.53, -0.44};
float Ro =  10;

void setup() {

  lcd.init(); 
  lcd.backlight(); 
  
  lcd.setCursor(0,0);
  lcd.print("Calibrating ") ;
  lcd.setCursor(0,1);
  lcd.print("GAS Sensors ...") ;
  Ro = MQCalibration(MQ_PIN);
  lcd.clear() ;
  lcd.print("Calibration done...") ;

  delay (1000) ;
  
  lcd.clear() ;
  lcd.setCursor(0,0) ;
  lcd.print("Calibrating ") ;
  lcd.setCursor(0,1) ;
  lcd.print("RFID ... ") ;
  while (!Serial);                       
  SPI.begin();                           
  mfrc522.PCD_Init();             
  delay(4);                                             
  mfrc522.PCD_DumpVersionToSerial();
  lcd.clear() ;
  lcd.setCursor(0,0) ;
  lcd.print("Calibration RFID ") ; 
  lcd.setCursor(0,1) ;
  lcd.print("Done ..") ;

  delay (1000) ;
  
  lcd.clear() ;
  lcd.setCursor(0,0) ;
  lcd.print("Calibrating ");

  dht.begin();
  lcd.clear() ;
  lcd.setCursor(0,0) ;
  lcd.print("Callibrating ");
  lcd.setCursor(0,1) ;
  lcd.print("DHT done  .. ") ;

  delay (1000) ;
  

}

void loop(){

  gas_sensor_lcd() ;
  humidity_lcd() ;
  rf_id_card() ;
 
  delay(200);

}


//================== GAS Sensor  ================================//
float MQResistanceCalculation(int raw_adc){
  
  return ( ((float)RL_VALUE * (1023 - raw_adc) / raw_adc));
}

float MQCalibration(int mq_pin){
  
  int i;
  float val = 0;

  for (i = 0; i < CALIBARAION_SAMPLE_TIMES; i++) {      //take multiple samples
    val += MQResistanceCalculation(analogRead(mq_pin));
    delay(CALIBRATION_SAMPLE_INTERVAL);
  }
  val = val / CALIBARAION_SAMPLE_TIMES;                 //calculate the average   value

  val = val / RO_CLEAN_AIR_FACTOR;                      //divided   by RO_CLEAN_AIR_FACTOR yields the Ro
  //according   to the chart in the datasheet
  return val;
}


float MQRead(int mq_pin){
  
  int i;
  float rs = 0;

  for (i = 0; i < READ_SAMPLE_TIMES; i++)   {
    rs += MQResistanceCalculation(analogRead(mq_pin));
    delay(READ_SAMPLE_INTERVAL);
  }

  rs = rs / READ_SAMPLE_TIMES;

  return rs;
}

int MQGetGasPercentage(float rs_ro_ratio, int gas_id){
  
  if ( gas_id   == GAS_LPG ) {
    return MQGetPercentage(rs_ro_ratio, LPGCurve);
  } else   if ( gas_id == GAS_CO ) {
    return MQGetPercentage(rs_ro_ratio, COCurve);
  } else if ( gas_id == GAS_SMOKE ) {
    return MQGetPercentage(rs_ro_ratio, SmokeCurve);
  }

  return 0;
}


int  MQGetPercentage(float rs_ro_ratio, float *pcurve){
  
  return (pow(10, (((log(rs_ro_ratio) - pcurve[1]) / pcurve[2]) + pcurve[0])));
}

//================XXX GAS Sensor  XXX=============================//

// ===================== Humidity Sensor =================================== //

void humidity_lcd() {


  float h = dht.readHumidity();
  float t = dht.readTemperature();
  float f = dht.readTemperature(true);
  
  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }

  float hif = dht.computeHeatIndex(f, h);
  float hic = dht.computeHeatIndex(t, h, false);

  lcd.clear()  ;
  lcd.setCursor(0,0);
  lcd.print("Humidity :");
  lcd.setCursor(0,1) ;
  lcd.print(h);

  rf_id_card() ;
  
  delay (2000) ;

  lcd.clear() ;
  lcd.setCursor(0,0);
  lcd.print("Temperature : C");
  lcd.setCursor(0,1) ;
  lcd.print(t);

  rf_id_card() ;
  
  delay (2000) ;

  lcd.clear() ;
  lcd.setCursor(0,0);
  lcd.print("Heat Index : ");
  lcd.setCursor(0,1) ;
  lcd.print(hic);

  rf_id_card() ;
  
  delay (2000) ;
}


void gas_sensor_lcd() {

  lcd.clear() ;
  lcd.setCursor(0,0);
  lcd.print("LPG:");
  lcd.setCursor(0,1) ;
  lcd.print(MQGetGasPercentage(MQRead(MQ_PIN) / Ro, GAS_LPG));
  lcd.print(" ppm" );

  rf_id_card() ;
  
  delay (2000) ;
  
  lcd.clear() ;
  lcd.setCursor(0,0);
  lcd.print("CO:");
  lcd.setCursor(0,1) ;
  lcd.print(MQGetGasPercentage(MQRead(MQ_PIN) / Ro, GAS_CO));
  lcd.print(" ppm" );

  rf_id_card() ;
  
  delay (2000) ;
  
  lcd.clear() ;
  lcd.setCursor(0,0);
  lcd.print("SMOKE:");
  lcd.setCursor(0,1) ;
  lcd.print(MQGetGasPercentage(MQRead(MQ_PIN) / Ro, GAS_SMOKE));
  lcd.print(" ppm" );

  rf_id_card() ;
  
  delay (2000) ;
  
}

void rf_id_card() {

  if ( ! mfrc522.PICC_IsNewCardPresent()) return;
  lcd.clear() ;
  lcd.setCursor(0,0) ;
  lcd.print("Attandeance  ");
  lcd.setCursor(0,1) ;
  lcd.print("Card Registered ");
  delay(2000) ;
  
}



#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <PubSubClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#include <ESP32Servo.h>
#include "HX711.h"
#include <BH1750FVI.h>
BH1750FVI LightSensor(BH1750FVI::k_DevModeContLowRes);
#include "PCF8574.h"
PCF8574 pcf8574(0x38);
#include <ArduinoJson.h>
#include <Keypad_I2C.h>
#include <Keypad.h>
#include <SPI.h>
#include <SD.h>
#include <WiFi.h>

WiFiManager wm;
WiFiClient espClient;
PubSubClient client(espClient);



#define I2CADDR 0x20
const byte ROWS = 4; //four rows
const byte COLS = 4; //four columns
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {0, 1, 2, 3}; // เชื่อมต่อกับ Pin แถวของปุ่มกด
byte colPins[COLS] = {4, 5, 6, 7}; // เชื่อมต่อกับ Pin คอลัมน์ของปุ่มกด
Keypad_I2C keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS, I2CADDR ); 
LiquidCrystal_I2C lcd(0x27, 16, 4);




const char* mqtt_server = "apbcm.ddns.net";

//sd card
  int chipSelect = 5;
  File myLog;
//


//system variable main
  int MODE=0;
  int state_mode0 = 0;
  int state_mode2 = 0;
  String strkey;
  int c_tel; //count_keypad
  boolean call=false;
  int data_s=0;
  int data_m=0;
  int data_l=0;
  String mac_id = "A01";
  boolean status_process = false; //false : offline / true : online
  boolean status_process_check = false;
  int irBroke=0;
  boolean setupMAC = false;
//
//system variable process
      //loadcell
      float calibration_factor =17860.00; //25699 
      #define zero_factor 8471887
      #define DOUT 27
      #define CLK 25
      #define DEC_POINT 2
      float offset=0;
      float get_units_kg();
      float load;
      float baseload ;
      float finalload ;
      float limitLoad = 140;
      boolean load_status = true;
      HX711 scale(DOUT, CLK);
      //
      //process
      int irPin = P0;
      int irPinS = P2;
      int irPinM = P3;
      int irPinL = P4;
      int irs;
      int irm;
      int irl;
      int proxi_pin = P1;
      String roomsizefull;
      int state = 0;
      boolean ir_Action = false;
      String proxi_Action = "";
      boolean proxi_status = true;
      String Size = "Unspec";
      boolean bottle_status = false; 
      boolean fullcheck = false; 
      int roomL;
      int roomM;
      int roomS;
      int wss;
      int fss;
      int wms;
      int fms;
      int wls;
      int fls;
      boolean start_p = false;
      boolean start_p_open = false;
      boolean ledStatus = false;
      
      uint16_t lux;
      int limitLux = 16;
      boolean lux_status = true;
      //
      //servo
      Servo servoMotorA1;
      Servo servoMotorA2;
      Servo servoMotorB1;
      Servo servoMotorB2;
      Servo servoMotorB3;
      int angleServo_A1 = 178;
      int angleServo_A2 = 20;
      int angleServo_B1 = 0;
      int angleServo_B2 = 30;
      int angleServo_B3 = 10;
      //
      //RGB
      #define S2 32
      #define S3 33
      #define sensorOut 34
      int redMin = 36; // Red minimum value
      int redMax = 544; // Red maximum value
      int greenMin = 40; // Green minimum value
      int greenMax = 631; // Green maximum value
      int blueMin = 33; // Blue minimum value
      int blueMax = 546; // Blue maximum value
      int redPW = 0;
      int greenPW = 0;
      int bluePW = 0;
      int redValue;
      int greenValue;
      int blueValue;
      boolean cx = false;
      //
//



//timer
#define INTERVAL_MESSAGE1 15000
unsigned long time_1 = 0;
#define INTERVAL_MESSAGE2 1000
unsigned long time_main = 0;
unsigned long time_main2 = 0;
int time_out = 180;
int time_out_using = 180;
//

String tp; //testping topic
String callU = "";
void setup() {
    delay(3000);
    setup_config();                
    keypad.begin(makeKeymap(keys));
    lcd.begin();
    lcd.backlight();
    setup_wifi();
    client.setServer(mqtt_server, 1883);
    client.setCallback(callback);
    reconnect();
    testPing();
    timer_reset();
    lcd.clear();
}

void callback(String topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;  
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  if(topic=="pingNode"){
    tp = messageTemp;
  }
  if(topic=="ping2Node"){
      client.publish("ping2Esp", "pong");
  }
  if(topic=="calluserNode"){
    lcd.clear();
    callU = messageTemp;
    
    if(callU!="n/a" && callU!=""){
      String Data = "User "+strkey;
      String Data2 = callU+" Bath";
      lcd.setCursor(0, 0);
      lcd.print("Account balance");
      lcd.setCursor(0, 1);
      lcd.print(Data);
      lcd.setCursor(-2, 2);
      lcd.print(Data2);
      callU ="";
    }else if(callU=="n/a"){
      lcd.setCursor(0, 0);
      lcd.print("Unable to view");
      lcd.setCursor(0, 1);
      lcd.print("account balance!");
      lcd.setCursor(-4, 2);
      lcd.print("Please register");
      Serial.println("Callu-is-not register");
      callU = "";
    }else if(callU==""){
      lcd.setCursor(0, 0);
      lcd.print("The latest");
      lcd.setCursor(0, 1);
      lcd.print("balance cannot");
      lcd.setCursor(-4, 2);
      lcd.print("be displayed");
      lcd.setCursor(-1, 3);
      lcd.print("[Offline]");
      Serial.println("Callu-is-offline");
      callU = "";
    }
  }
  Serial.println();
  delay(10);
}

void loop() {
  //  Serial.println(pcf8574.digitalRead(irPin));
    /*lcd.setCursor(0, 0);
    lcd.print("MAIN");*/
    /*if(setupMAC == false){
        if (pcf8574.begin()){
            Serial.println("pcf8574-OK");
        }else{
             Serial.println("pcf8574-ERROR");
        }
        setupMAC = true;
    }*/
    char key = keypad.getKey();
    if (client.connected()) {
      client.loop();
    }
    if(millis() - time_main >= INTERVAL_MESSAGE2){
                        irs = pcf8574.digitalRead(P2);
                        irm = pcf8574.digitalRead(P3);
                        irl = digitalRead(13);
                        Serial.print(irs);
                        Serial.print(irm);
                        Serial.println(irl);
          if(MODE==2 && state_mode2==1 && state!=0){
            
          }else if(MODE==2 && state_mode2==1 && state==0){
            time_out --;
              //Serial.println(time_out);
              if(time_out==0){
                  actionServo(servoMotorA1,angleServo_A1,20,175,"close-",2);
                  Serial.println("TIME_OUT RESET");
                  time_out = time_out_using;
                  MODE=3;
              }
            
          }else if((MODE==0&&state_mode0==0) || MODE==3){
            
          }else if(MODE==0&&state_mode0==1){
              time_out --;
              //Serial.println(time_out);
              if(time_out==0){
                  Serial.println("TIME_OUT RESET");
                  actionServo(servoMotorA1,angleServo_A1,20,175,"close-",2);
                  time_out = time_out_using;
                  MODE=3;
              }
          }else{
              time_out --;
             // Serial.println(time_out);
              if(time_out==0){
                  Serial.println("TIME_OUT RESET");
                  actionServo(servoMotorA1,angleServo_A1,20,175,"close-",2);
                  time_out = time_out_using;
                  MODE=3;
              }
          }
          //Serial.println("timmer-");
      time_main = millis();
    }
    
    wss = pcf8574.digitalRead(P5);
    fss = pcf8574.digitalRead(P6);
    wms = pcf8574.digitalRead(P7);
    fms = digitalRead(3);
    wls = pcf8574.digitalRead(P0);
    fls = digitalRead(14);
    /*irs = pcf8574.digitalRead(P2);
    irm = pcf8574.digitalRead(P3);
    irl = digitalRead(13);*/
   // int bre = pcf8574.digitalRead(irPin);
    if(MODE==0){
        
        if(state_mode0==0){
            timer(true);
            /*lcd.setCursor(2, 0);
            lcd.print(irs);
            lcd.setCursor(4, 1);
            lcd.print(irm);
            lcd.setCursor(-3, 2);
            lcd.print(irl);*/
            
            lcd.setCursor(2, 0);
            lcd.print("APBC Machine");
            lcd.setCursor(4, 1);
            lcd.print("Welcome");
            lcd.setCursor(-3, 2);
            lcd.print("Please any key");
            if(status_process_check == status_process){
                lcd.setCursor(status_process==true ? 0 : -1, 3);
                lcd.print(status_process==true ? "[Online]" : "[Offline]");
            }else{
                lcd.clear();
                status_process = status_process_check;
            }
            //Serial.println(status_process_check);
            if (key){
              lcd.clear();
              state_mode0++;
              time_out=time_out_using;
              key = NO_KEY;
            }
        }//st0
        if(state_mode0==1){
            timer(false); //Timeout??
            lcd.setCursor(0,0);
            lcd.print("Enter your phone");
            lcd.setCursor(5,1);
            lcd.print("number");
            if(key){
              if(key!='A'&&key!='B'&&key!='C'&&key!='D'&&key!='*'&&key!='#'){  
                    if(c_tel<10){  
                        c_tel++;
                        lcd.clear();
                        strkey += key;
                        lcd.setCursor(-1, 2);
                        lcd.print(strkey);
                    }
                }else if(key=='#'){
                    lcd.clear();
                    strkey.remove(c_tel-1);
                    lcd.setCursor(-1, 2);
                    lcd.print(strkey);
                    if(c_tel>0) c_tel--;
                }else if(key=='*'&&c_tel==10){
                    c_tel = 0;
                    state_mode0 = 0;
                    MODE=1;
                    call=true;
                    lcd.clear();
                    if(strkey=="1233210000"){
                      wm.resetSettings();
                      ESP.restart();
                    }
                }
                key = NO_KEY;
            }//key
        }//st1
    }//MODE0
    if(MODE==1){  //* passmode #logout
      if(call==true){
      lcd.setCursor(0, 0);
      lcd.print("The latest");
      lcd.setCursor(0, 1);
      lcd.print("balance cannot");
      lcd.setCursor(-4, 2);
      lcd.print("be displayed");
      lcd.setCursor(-1, 3);
      lcd.print("[Offline]");
          Calluser(strkey);
          call = false;
      }
      if(key){
          if(key=='*'){
              MODE++;
              lcd.clear();
          }
          if(key=='#'){
            MODE=0;
            strkey = "";
            timer_reset();
            lcd.clear();
          }
          key = NO_KEY;
      }
    }//MODE1
    if(MODE==2){
        if(state_mode2==0){
             String disp = "Size S: "+String(data_s);
             lcd.setCursor(0, 0);
             lcd.print("Exchange results");
             lcd.setCursor(0, 1);
             lcd.print(disp);
             lcd.setCursor(-4, 2);
             lcd.print("Size M: "+String(data_m));
             lcd.setCursor(-4, 3);
             lcd.print("Size L: "+String(data_l));
             if(key){
                if(key=='*'){
                    start_p_open = true;
                    state_mode2++;
                    Serial.println("GO TO PROCESS");
                    lcd.clear();
                    baseload = get_units_kg();
                }
                 if(key=='#'){
                  lcd.clear();
                      if((data_s==0&&data_m==0&&data_l==0)==true){
                            lcd.setCursor(3, 0);
                            lcd.print("Thank you");
                            lcd.setCursor(2,1);
                            lcd.print("Please check");      
                            lcd.setCursor(-2,2);
                            lcd.print("your account");
                            lcd.setCursor(0,3);
                            lcd.print("balance");
                            delay(3000);
                            MODE=0;
                            state_mode2=0;
                            strkey = "";
                            timer_reset();
                            lcd.clear();
                      }else{
                          state_mode2=0;
                          MODE++;
                      }
                  }
                key = NO_KEY;
             }
        }
        if(state_mode2==1){
           
                  if(start_p_open){
                         actionServo(servoMotorA1,angleServo_A1,20,175,"open-",2);
                         Serial.println("Servo A1 is open now");
                         //pcf8574.digitalWrite(LED,1);
                         start_p_open = false;
                          lcd.setCursor(1,0);
                          lcd.print("Ready for work");
                          lcd.setCursor(3,1);
                          lcd.print("Please put");      
                          lcd.setCursor(-1,2);
                          lcd.print("The bottle");
                  }
                  
                      
                  
                  if(state == 0){
                    /*int ir = pcf8574.digitalRead(irPinS);
                    delay(500);
                    if(ir==0){
                          lcd.clear();
                          lcd.setCursor(0,0);
                          lcd.print("Please wait....");
                          Serial.println("state = 0");
                          actionServo(servoMotorA1,angleServo_A1,20,157,"close-",2);
                          state++;
                          time_out = time_out_using;
                        delay(1500);
                    }*/
                    if(millis() - time_main2 >= INTERVAL_MESSAGE2){
                      load = get_units_kg();
                      Serial.println("SW");
                      time_main2 = millis();
                    }
                    
                          
                           
                   float fl = load - baseload;
                   Serial.println(fl);
                   if(fl>=0.01){
                          lcd.clear();
                          lcd.setCursor(1,0);
                          lcd.print("The machine is");
                          lcd.setCursor(4,1);
                          lcd.print("working");      
                          lcd.setCursor(-2,2);
                          lcd.print("please wait a");
                          lcd.setCursor(1,3);
                          lcd.print("moment");
                          Serial.println("state = 0");
                          actionServo(servoMotorA1,angleServo_A1,20,175,"close-",2);
                          state++;
                          time_out = time_out_using;
                          delay(1500);
                   }
                  }
            
                  if(state == 1){
                      Serial.println("state = 1");
                      int proxi =1;
                      //Serial.println(proxi);
                      for(int i=0;i<=3;i++){
                        proxi = pcf8574.digitalRead(P1);
                        delay(30);
                      }
                      if(proxi == 0){
                          state++;
                          proxi_Action = "ไม่ใช่โลหะ";
                          proxi_status = false;
                      }else{
                          state++;
                          proxi_Action = "ไม่ใช่โลหะ";
                          proxi_status = false;
                      }
                  } 
                  if(state == 2){
                      int i;
                      delay(2000);
                      Serial.println("state = 2");
                      for(int i =0; i<10; i++){
                        irs = pcf8574.digitalRead(P2);
                        irm = pcf8574.digitalRead(P3);
                        irl = digitalRead(13);
                        Serial.print(irs);
                        Serial.print(irm);
                        Serial.println(irl);
                        delay(170);
                      }
                      
                      if(irs==1 && irm==0 && irl==0){
                          Size = "s";    
                          Serial.println("S");    
                      }else if(irs==1 && irm==1 && irl==0){
                          Size = "m";
                          Serial.println("M");
                      }else if(irs==1 && irm==1 && irl==1){
                          Size = "l";
                          Serial.println("L");
                      }else{
                          Size = "Unspec";
                      }
                      for(int i =0; i<3; i++){
                        if(Size == "s"){
                            if(digitalRead(3)==0){
                                fullcheck = true;
                            }else{
                                fullcheck = false;
                            }
                        }else if(Size == "m"){
                            if(pcf8574.digitalRead(P6)==0){
                                fullcheck = true;
                            }else{
                                fullcheck = false;
                            }
                        }else if(Size == "l"){
                            if(digitalRead(14)==0){
                                fullcheck = true;
                            }else{
                                fullcheck = false;
                            }
                        }
                          delay(100);
                      }
                     
                        state++;
                      
                  }
            
                  if(state == 3){
                      int i;
                      Serial.println("state = 3");
                      for(i=0;i<3;i++){
                          lux = LightSensor.GetLightIntensity();
                          delay(1);   
                      }
                      if(i==3){
                        state++;
                        //pcf8574.digitalWrite(LED,0);
                      }
                  }
                  
                  if(state == 4){
                      int i;
                      Serial.println("state = 4");
                      for(i=0;i<2;i++){
                          // Read Red value
                          redPW = getRedPW();
                          redValue = map(redPW, redMin,redMax,255,0);
                          delay(50);
                          // Read Green value
                          greenPW = getGreenPW();
                          greenValue = map(greenPW, greenMin,greenMax,255,0);
                          delay(50);
                          // Read Blue value
                          bluePW = getBluePW();
                          blueValue = map(bluePW, blueMin,blueMax,255,0);
                          delay(50);      
                          
                      }
                      if(i==2){
                        state++;
                      }
                  }
            
                   if(state == 5){
                      int i;
                      Serial.println("state = 5");
                      for(i=0;i<3;i++){
                          load = get_units_kg();
                          
                          delay(10);   
                      }
                      if(i==3){
                        /*if(load <=0){
                          load = 0.01;
                        }*/
                        finalload = load - baseload;
                        state++;
                      }
                  }
                  
                  if(state == 6){
                      
                      
                      //สรุป
                      //bottleDistance = bottleDistance+offer;
                        Serial.println("###########################");
                        Serial.print("Metallicity : ");
                        Serial.println(proxi_Action);
                        /*Serial.print("bottleDistance : ");
                        Serial.println(bottleDistance);*/
                        Serial.print("RGB : ");
                        Serial.print("Red = ");
                        Serial.print(redValue);
                        Serial.print(" - Green = ");
                        Serial.print(greenValue);
                        Serial.print(" - Blue = ");
                        Serial.println(blueValue);
                        Serial.print("cx1 : ");
                        Serial.println(redValue - greenValue);
                        Serial.print("cx2 : ");
                        Serial.println(redValue - blueValue);
                        Serial.print("cx3 : ");
                        Serial.println(greenValue - blueValue);
                        Serial.print("B-LOAD : ");
                        Serial.println(baseload);
                        Serial.print("LOAD : ");
                        Serial.println(load);
                        Serial.print("F-LOAD : ");
                        Serial.println(finalload);
                        //Serial.print("LOAD-true : ");
                        //Serial.println(load - (baseLoad));
                        Serial.print("LUX : ");
                        Serial.println(lux);
                        Serial.print("full? : ");
                        Serial.println(fullcheck);
                        Serial.println("###########################");
                        Serial.println(Size);
                        
                       /* if((redValue - greenValue)>=-20 && (redValue - greenValue)<=20 && (redValue - blueValue)>=-20 && (redValue - blueValue)<=20 && (greenValue - blueValue)>=-20 && (greenValue - blueValue)<=20){
                          cx = false;
                          Serial.println("สี : ไม่มีสี");
                        }else{
                          cx = true;
                        }*/
                        if((redValue - greenValue)<=10 &&  (redValue - blueValue)<=10 && (greenValue - blueValue)<=10){
                          cx = false;
                          Serial.println("สี : ไม่มีสี");
                        }else{
                          cx = true;
                          Serial.println("สี : มีสี");
                        }

                        
                        if(lux>=limitLux && lux <= 26){
                          lux_status = false;
                        }else{
                          lux_status = true;
                        }
                     
                        if(Size == "s"){
                            if(finalload <= 0.15){
                              load_status = false;
                            }else{
                              load_status = true;
                            }
                        }else if(Size == "m"){
                            if(finalload <= 0.15){
                              load_status = false;
                            }else{
                              load_status = true;
                            }
                        }else if(Size == "l"){
                            if(finalload <= 0.15){
                              load_status = false;
                            }else{
                              load_status = true;
                            }
                        }
            roomsizefull = Size;
                        if(Size != "Unspec" && proxi_status != true && cx != true && lux_status != true && load_status != true && fullcheck!= true){
                          bottle_status = true;
                          Serial.println("เก็บขวดไว้");
                          proxi_status = true;
                          cx = true;
                          lux_status = true;
                          load_status = true;
                          if(Size=="s"){
                              data_s++;
                          }else if(Size=="m"){
                              data_m++;
                          }else if(Size=="l"){
                              data_l++;
                          }
                        }else{
                          Serial.println("คืนขวด");
                          bottle_status = false;
                          Size = "Unspec";
                          proxi_status = true;
                          cx = true;
                          lux_status = true;
                          load_status = true;
                          //Serial.println(Size);
                        }
                        
                        Serial.println("###########################");
                      if(Size=="l"){
                        actionServo(servoMotorB2,angleServo_B2,180,30,"open",1);
                      }else if(Size=="s"){
                        actionServo(servoMotorB3,angleServo_B3,180,0,"open",1);
                      }else if(Size=="Unspec"){
                        actionServo(servoMotorB1,angleServo_B1,180,0,"open",1);
                      }
                      servoMotorA2.attach(2);
                      actionServo(servoMotorA2,angleServo_A2,20,140,"open-",2);
                      delay(1000);
                      actionServo(servoMotorA2,angleServo_A2,20,140,"close-",2);
                      delay(300);
                      servoMotorA2.detach();
                      state++;
                  }
                  if(state==7){
                    if(Size!="Unspec"){
                       String size_disp = Size;
                       size_disp.toUpperCase();
                       String disp = "Bottle Size ";
                       String disp2 = size_disp;
                       lcd.clear();
                       lcd.setCursor(0,0);
                       lcd.print(disp); 
                       lcd.setCursor(15,0);
                       lcd.print(disp2);
                    }else{
                      if(roomsizefull=="Unspec"){
                           lcd.clear();
                           lcd.setCursor(0,0);
                           lcd.print("Reject !!!"); 
                           lcd.setCursor(0,1);
                           lcd.print("Please Try Again");
                      }else{
                        if(fullcheck!= true){
                           lcd.clear();
                           lcd.setCursor(0,0);
                           lcd.print("Reject !!!"); 
                           lcd.setCursor(0,1);
                           lcd.print("Please Try Again");
                        }else{
                          roomsizefull.toUpperCase();
                          String dis = "Size "+roomsizefull+" Storage";
                           lcd.clear();
                           lcd.setCursor(0,0);
                           lcd.print("Reject !!!"); 
                           lcd.setCursor(0,1);
                           lcd.print("Please Try Again");
                           lcd.setCursor(-4,2);
                          lcd.print(dis);
                          lcd.setCursor(-4,3);
                          lcd.print("is Full");
                        }
                      }
                    }
                      state = 0;
                      state_mode2=0;
                      delay(3000);
                      if(Size=="l"){
                        actionServo(servoMotorB2,angleServo_B2,180,30,"close",1);
                      }else if(Size=="s"){
                        actionServo(servoMotorB3,angleServo_B3,180,0,"close",1);
                      }else if(Size=="Unspec"){
                        actionServo(servoMotorB1,angleServo_B1,180,0,"close",1);
                      }
                      for(int i =0; i<3; i++){
                        irBroke = pcf8574.digitalRead(irPinS);
                        delay(200);
                      }
                      Serial.println("irBreak:"+irBroke);
                      if(irBroke==1){
                        client.publish("break", "1");
                      }else{
                        client.publish("break", "0");
                      }
                      lcd.clear();
                  }
               
              }//state_mode2
    }//MODE2
    if(MODE==3){
      if((data_s==0&&data_m==0&&data_l==0)==true){
        lcd.clear();
        lcd.setCursor(3, 0);
                            lcd.print("Thank you");
                            lcd.setCursor(2,1);
                            lcd.print("Please check");      
                            lcd.setCursor(-2,2);
                            lcd.print("your account");
                            lcd.setCursor(0,3);
                            lcd.print("balance");
                            delay(3000);
                            MODE=0;
                            state_mode2=0;
                            strkey = "";
                            timer_reset();
                            lcd.clear();
                            MODE=0;
        state = 0;
        state_mode2=0;
        strkey = "";
        timer_reset();
        state_mode0=0;
      }else{
        char out[128];
        DynamicJsonDocument doc(1024);
        doc["userdata_Tel"] = strkey;
        doc["userdata_S"] = data_s;
        doc["userdata_M"] = data_m;
        doc["userdata_L"] = data_l;
        doc["userdata_machine"] = mac_id;
        serializeJson(doc, out);
        sendtoServer(out);
        lcd.setCursor(3, 0);
                            lcd.print("Thank you");
                            lcd.setCursor(2,1);
                            lcd.print("Please check");      
                            lcd.setCursor(-2,2);
                            lcd.print("your account");
                            lcd.setCursor(0,3);
                            lcd.print("balance");

        delay(3000);
        lcd.clear();
        MODE=0;
        state = 0;
        state_mode2=0;
        strkey = "";
        state_mode0=0;
        timer_reset();
        strkey = "";
        data_s = 0;
        data_m = 0;
        data_l = 0;
      }
    }//MODE3
    
    
    
    
    
}//LOOP

void setup_config(){
    Serial.begin(9600);
    if (pcf8574.begin()){
        Serial.println("pcf8574-OK");
    }else{
         Serial.println("pcf8574-ERROR");
    }
    
    if(!SD.begin(chipSelect)) {
        Serial.println("initialization failed!");
        return;
    }
    Wire.begin();
    LightSensor.begin(); 
    scale.set_scale(calibration_factor); 
    scale.set_offset(zero_factor);
    
    servoMotorA1.attach(4);
    servoMotorA2.attach(2);
    servoMotorB1.attach(16);  
    servoMotorB2.attach(17); 
    servoMotorB3.attach(15);
    
     pcf8574.pinMode(irPin,INPUT);
    pcf8574.pinMode(irPinS,INPUT);
    pcf8574.pinMode(irPinM,INPUT);
    pcf8574.pinMode(irPinL,INPUT);
    //pcf8574.digitalWrite(irPinS,LOW);
    //pcf8574.digitalWrite(irPinM,LOW);
    //pcf8574.digitalWrite(irPinL,LOW);
    pcf8574.pinMode(proxi_pin,INPUT);
    pcf8574.pinMode(P5, INPUT);
    pcf8574.pinMode(P6, INPUT);
    pcf8574.pinMode(P7, INPUT);
    servoMotorA1.write(175);
    pinMode(14, INPUT);
    pinMode(3, INPUT);
    pinMode(13, INPUT);
   // pinMode(14,INPUT_PULLUP);//sw
    pinMode(S2, OUTPUT);
    pinMode(S3, OUTPUT);
    pinMode(sensorOut, INPUT);
    actionServo(servoMotorA1,angleServo_A1,20,175,"close-",2);
    actionServo(servoMotorA2,angleServo_A2,20,140,"close-",2);
    actionServo(servoMotorB1,angleServo_B1,180,0,"close",1);
    actionServo(servoMotorB2,angleServo_B2,180,30,"close",1);
    actionServo(servoMotorB3,angleServo_B3,180,0,"close",1);
}
void actionServo(Servo &s,int &pos,int A,int B,String action,int d){
  if(action == "open"){
      for (; pos <= A; pos += 1) {
        s.write(pos);
        //Serial.println(angleServo_A1);
        delay(d); 
      }
      
  }
  if(action == "close"){
      for (; pos >= B; pos -= 1) {
        s.write(pos);
        //Serial.println(angleServo_A1);
        delay(d); 
      }
  }
  if(action == "open-"){
      for (; pos >= A; pos -= 1) {
        s.write(pos);
        //Serial.println(angleServo_A1);
        delay(d); 
      }
      
  }
  if(action == "close-"){
      for (; pos <= B; pos += 1) {
        s.write(pos);
        //Serial.println(angleServo_A1);
        delay(d); 
      }
  }
}

void setup_wifi(){
    lcd.clear();
    lcd.setCursor(1, 0);
    lcd.print("Wi-Fi Setting !");
    lcd.setCursor(1, 1);
    lcd.print("Please Connect");
    lcd.setCursor(-1, 2);
    lcd.print("The Wi-Fi");
    WiFi.mode(WIFI_STA); 
    bool res;
    wm.setConfigPortalBlocking(true);
    wm.setConfigPortalTimeout(120);
    res = wm.autoConnect("APBCM_PROJECT","123456789"); 
   /* if(digitalRead(13)==0){
      wm.resetSettings();
    }*/
    if(!res) {
        Serial.println("Failed to connect");
        ESP.restart();
    } 
    else {
        //if you get here you have connected to the WiFi    
        Serial.println("connected...WiFi :)");
        wm.setConfigPortalBlocking(false);
    }

}

void wificheck(){
  lcd.setCursor(0, 0);
  lcd.print("Reconnect..");
  
  if(WiFi.status() != WL_CONNECTED) {
      for(int i=0;i<5;i++){
        wm.autoConnect();
        delay(1000);
      }
    }
   lcd.clear();
}

void timer(boolean en){
  if(en){
      if(millis() - time_1 > INTERVAL_MESSAGE1){
            reconnect();
            wificheck();
            send_data();
            roomStatus();
            
            
            
            time_1 = millis();
        }
  }
}
void timer_reset(){
  time_1 = millis();
}
void reconnect() {
 
  Serial.println("reconnect");
  //client.subscribe("pingNode");
  if (!client.connected()) {
    lcd.clear();
    lcd.setCursor(0, 0);
    
    lcd.print("Reconnect..");
    Serial.println("test-> " +client.connect("ESP8266Client"));
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");  
      client.subscribe("pingNode");
      client.subscribe("ping2Node");
      client.subscribe("calluserNode");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(1000);
    }
  }
  lcd.clear();
}
boolean testPing(){
  client.publish("pingEsp", "ping");
  Serial.print(client.state());
  delay(100);
  if(tp=="pong"){
    tp = "";
    status_process_check = true;
    return true;
  }else{
    tp = "";
    status_process_check = false;
    return false;
  }
}
void Calluser(String tel){
  char TEL[11];
  tel.toCharArray(TEL,tel.length()+1);
  if (client.connected()) {
        client.connect("ESP8266Client");
      }
  if(testPing()){
    for(int i=0;i<2;i++){
      Serial.print(client.state());
      client.publish("callUser", TEL);
      delay(200);
    }
  }else{
      lcd.setCursor(0, 0);
      lcd.print("The latest");
      lcd.setCursor(0, 1);
      lcd.print("balance cannot");
      lcd.setCursor(-4, 2);
      lcd.print("be displayed");
      lcd.setCursor(-1, 3);
      lcd.print("[Offline]");
      Serial.println("callu can't ping");
  }
  
}

int getRedPW() {
  digitalWrite(S2,LOW);
  digitalWrite(S3,LOW);
  int PW;
  PW = pulseIn(sensorOut, LOW);
  return PW;
}

int getGreenPW() {
  digitalWrite(S2,HIGH);
  digitalWrite(S3,HIGH);
  int PW;
  PW = pulseIn(sensorOut, LOW);
  return PW;
}

int getBluePW() {
  digitalWrite(S2,LOW);
  digitalWrite(S3,HIGH);
  int PW;
  PW = pulseIn(sensorOut, LOW);
  return PW;
}

float get_units_kg(){return(scale.get_units()*0.453592);}

void sendtoServer(char str[]){
  /*if(testPing()){
    client.publish("insertData1", str);
    Serial.println("Send Data to server");
  }else{*/
    Serial.println("s:off, save to sdcard");
    myLog = SD.open("/log.txt", FILE_APPEND);
    if(myLog){
      if(myLog.size()>0){
        myLog.print("/");
      }
      myLog.println(str);
      Serial.println(str);
    }
    myLog.close();
  
}

void roomStatus(){
    for(int i=0;i<3;i++){
        if(fss==0 && wss==0){
          roomM = 2;
        }else if(fss==0 &&wss==1){
          roomM = 2;
        }else if(fss==1 &&wss==0){
          roomM = 1;
        }else if(fss== 1 &&wss== 1){
          roomM = 0;   
        }
        Serial.print(pcf8574.digitalRead(P5));
        Serial.println(pcf8574.digitalRead(P6));
      ////////////////////////m
        if(fms==0 &&wms==0){
          roomS = 2;
        }else if(fms==0 &&wms==1){
          roomS = 2;
        }else if(fms==1 &&wms==0){
          roomS = 1;
        }else if(fms==1 &&wms==1){
          roomS = 0;   
        }
        Serial.print(digitalRead(3));
        Serial.println(pcf8574.digitalRead(P7));
        //L

        if(fls==0 && wls==0){
          roomL = 2;
        }else if(fls==0 &&wls==1){
          roomL = 2;
        }else if(fls==1 &&wls==0){
          roomL = 1;
        }else if(fls==1 && wls==1){
          roomL = 0;   
        }
        Serial.print(digitalRead(13));
        Serial.println(digitalRead(14));
        delay(100);
    }
    
        Serial.println("sent-roomstatus");
        char out[128];
        DynamicJsonDocument doc(1024);
        doc["RoomS"] = roomS;
        doc["RoomM"] = roomM;
        doc["RoomL"] = roomL;
        serializeJson(doc, out);
        Serial.println(out);
        client.publish("RoomStatus",out);
    
}

void send_data(){
      String strTemp;
      myLog = SD.open("/log.txt", FILE_APPEND);
      myLog.close();
      
      myLog = SD.open("/log.txt");
      if(myLog){ 
        while(myLog.available()) {
         strTemp += (char)myLog.read();
        }
      }
       
      if(testPing()){
        if(myLog.size()>0){
       String  str = "";
       String strs[50];
       int StringCount = 0;
       str = strTemp;
       while (str.length() > 0)
      {
        int index = str.indexOf('/');
        if (index == -1) // No space found
        {
          strs[StringCount++] = str;
          break;
        }
        else
        {
          strs[StringCount++] = str.substring(0, index);
          str = str.substring(index+1);
        }
      }
      for (int i = 0; i < StringCount; i++){
        int Length = strs[i].length()+1;
        char out[Length];
        strs[i].toCharArray(out, Length);
        client.publish("insertSD",out); 
        delay(200);
      }
     
          SD.remove("/log.txt");
          Serial.println("send complet");
        }else{
          Serial.println("notting to send");
        }
      }else{
        Serial.println("Can't not send to server");
      }
      myLog.close();
}

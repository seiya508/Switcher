//2021.03.26
// Upload to github 2022.02.09
//AlexaとESP32を連携
//壁スイッチの操作を行う
//サーボモータ誤動作回避のため、サーボライブラリを使ってみる

#include <Servo.h>

#include <Espalexa.h>
#ifdef ARDUINO_ARCH_ESP32
#include <WiFi.h>
#else
#include <ESP8266WiFi.h>
#endif

//チャタリング防止のためのタイマ割り込み用
#include <Ticker.h>


//----------Espalexa設定部----------//
// prototypes
boolean connectWifi();

void onTimer();

void turnOnSwitch1();
void turnOffSwitch1();
void turnOnSwitch2();
void turnOffSwitch2();

void switchSwitch1();
void switchSwitch2();

void printHtml();

//callback functions
//スイッチ
void controlSwitch1(EspalexaDevice* d);
void controlSwitch2(EspalexaDevice* d);

// WiFi設定
const char* ssid = "hogehoge";
const char* password = "hogehoge";
IPAddress ip(hogehoge);         // for fixed IP Address
IPAddress subnet(hogehoge);     //

boolean wifiConnected = false;

Espalexa espalexa;
#ifdef ARDUINO_ARCH_ESP32
WebServer server(80);
#else
ESP8266WebServer server(80);
#endif

//----------サーボモータ設定部----------
Servo onServo, offServo;

// ※以下，適宜変更

const int SERVO_PIN_ON = 25;
const int SERVO_PIN_OFF = 32;

//サーボモータ位置(個体差あり：調整済み)
const int POS_NEUTRAL_ON  = 97;
const int POS_SWITCH1_ON  = POS_NEUTRAL_ON + 36;
const int POS_SWITCH2_ON  = POS_NEUTRAL_ON - 36;
//誤動作防止のため改良
//const int POS_SWITCH2_ON  = 12;
//const int POS_NEUTRAL_ON  = POS_SWITCH2_ON + 17;
//const int POS_SWITCH1_ON  = POS_SWITCH2_ON + 17*2 + 1;

const int POS_NEUTRAL_OFF = 91;
const int POS_SWITCH1_OFF = POS_NEUTRAL_OFF - 28;
const int POS_SWITCH2_OFF = POS_NEUTRAL_OFF + 28;

//回転待ち時間
const int WAIT_TIME = 350;


//タクトスイッチ
const int SW1_PIN = 15; //0番ピンはデフォルトでPULLUP(ノイズ防止)
const int SW2_PIN = 0;  //15番ピンはデフォルトでPULLUP(ノイズ防止)
int chat_count1 = 0;    //チャタリング防止用
int chat_count2 = 0;
Ticker timer;


//スイッチ1,2の状態を保持
boolean switch1 = true;
boolean switch2 = true;


void setup(){
  Serial.begin(115200);

  //サーボモーターのセットアップ
  onServo.attach(SERVO_PIN_ON);
  offServo.attach(SERVO_PIN_OFF);
  onServo.write(POS_NEUTRAL_ON);
  offServo.write(POS_NEUTRAL_OFF);
  onServo.detach();
  offServo.detach();

  //ボタンセットアップ
  pinMode(SW1_PIN, INPUT_PULLUP);
  pinMode(SW2_PIN, INPUT_PULLUP);

  //ボタン割り込み処理
  timer.attach_ms(2, onTimer);
  
  // Initialise wifi connection
  wifiConnected = connectWifi();

  //ここにブラウザでの処理を書く
  if(wifiConnected){
    server.on("/", HTTP_GET, [](){
      //現在の状態を表示するのみ
      printHtml();
    });

    server.on("/switch1/on", HTTP_GET, [](){
      turnOnSwitch1();
      printHtml();
    });
    server.on("/switch1/off", HTTP_GET, [](){
      turnOffSwitch1();
      printHtml();
    });
    server.on("/switch1/switch", HTTP_GET, [](){

      switchSwitch1();
      printHtml();
    });

    server.on("/switch2/on", HTTP_GET, [](){
      turnOnSwitch2();
      printHtml();
    });
    server.on("/switch2/off", HTTP_GET, [](){
      turnOffSwitch2();
      printHtml();
    });
    server.on("/switch2/switch", HTTP_GET, [](){
      switchSwitch2();
      printHtml();
    });

    //スイッチ1, 2を入れる
    server.on("/switch12/on", HTTP_GET, [](){
      turnOnSwitch2();
      turnOnSwitch1();
      printHtml();
    });
    //スイッチ1, 2を消す
    server.on("/switch12/off", HTTP_GET, [](){
      turnOffSwitch2();
      turnOffSwitch1();
      printHtml();
    });
    
    server.onNotFound([](){
      if (!espalexa.handleAlexaApiCall(server.uri(),server.arg(0))) //if you don't know the URI, ask espalexa whether it is an Alexa control request
      {
        //whatever you want to do with 404s
        server.send(404, "text/plain", "Not found");
      }
    });
    
    // Define your devices here. 
    //スイッチ
    espalexa.addDevice("Switch1", controlSwitch1, EspalexaDeviceType::onoff, 0);   //non-dimmable device
    espalexa.addDevice("Switch2", controlSwitch2, EspalexaDeviceType::onoff, 0);   //non-dimmable device
   
    espalexa.begin(&server); //give espalexa a pointer to your server object so it can use your server instead of creating its own
    //server.begin(); //omit this since it will be done by espalexa.begin(&server)
    
  }else{
    while (1) {
      Serial.println("Cannot connect to WiFi. Please check data and reset the ESP.");
      delay(2500);
    }
  }
}
 
void loop(){
   //server.handleClient() //you can omit this line from your code since it will be called in espalexa.loop()
   espalexa.loop();
   delay(1);
}

void turnOnSwitch1(){
  onServo.attach(SERVO_PIN_ON);
  onServo.write(POS_SWITCH1_ON);  //Switch1をON
  delay(WAIT_TIME);
  onServo.write(POS_NEUTRAL_ON);
  delay(WAIT_TIME);
  onServo.detach();
  
  switch1 = true;
}

void turnOffSwitch1(){
  onServo.attach(SERVO_PIN_OFF);
  onServo.write(POS_SWITCH1_OFF);  //Switch1をOFF
  delay(WAIT_TIME);
  onServo.write(POS_NEUTRAL_OFF);
  delay(WAIT_TIME);
  onServo.detach();

  switch1 = false;
}

void turnOnSwitch2(){
  onServo.attach(SERVO_PIN_ON);
  onServo.write(POS_SWITCH2_ON);  //Switch2をON
  delay(WAIT_TIME);
  onServo.write(POS_NEUTRAL_ON);
  delay(WAIT_TIME);
  onServo.detach();

  switch2 = true;
}

void turnOffSwitch2(){
  onServo.attach(SERVO_PIN_OFF);
  onServo.write(POS_SWITCH2_OFF);  //Switch2をOFF
  delay(WAIT_TIME);
  onServo.write(POS_NEUTRAL_OFF);
  delay(WAIT_TIME);
  onServo.detach();

  switch2 = false;
}

void switchSwitch1(){
  if(!switch1){
    turnOnSwitch1();
  }else{
    turnOffSwitch1();
  }
}

void switchSwitch2(){
  if(!switch2){
    turnOnSwitch2();
  }else{
    turnOffSwitch2();
  }
}

void onTimer(){
  if( digitalRead(SW1_PIN) == LOW ){
    chat_count1++;

    //20ms LOWだったらドアの解錠/施錠
    //※2ms周期なので 2 * 10 = 20ms
    if( chat_count1 == 10 ){
      switchSwitch1();
    }
  }else{
    chat_count1 = 0;
  }

  if( digitalRead(SW2_PIN) == LOW ){
    chat_count2++;

    //20ms LOWだったらドアの解錠/施錠
    if( chat_count2 == 10 ){
      switchSwitch2();
    }
  }else{
    chat_count2 = 0;
  }
  
}


// HTML
void printHtml(){
  
  char temp[1000];  //HTML文格納用
  char switch1Button[100];
  char switch1State[10];
  char switch2Button[100];
  char switch2State[10];

  // switch1がoffなら、onにするボタンを表示
  if(!switch1){
    snprintf(switch1Button, 100, "<p><a href=\"/switch1/on\"><button class=\"button\">ON</button></a></p>");
    snprintf(switch1State, 10, "OFF");
  }else{
    snprintf(switch1Button, 100, "<p><a href=\"/switch1/off\"><button class=\"button button2\">OFF</button></a></p>");
    snprintf(switch1State, 10, "ON");
  } 

  // switch2がoffなら、onにするボタンを表示
  if(!switch2){
    snprintf(switch2Button, 100, "<p><a href=\"/switch2/on\"><button class=\"button\">ON</button></a></p>");
    snprintf(switch2State, 10, "OFF");
  }else{
    snprintf(switch2Button, 100, "<p><a href=\"/switch2/off\"><button class=\"button button2\">OFF</button></a></p>");
    snprintf(switch2State, 10, "ON");
  } 
  
  //HTML文の代入 (HTMLの開始とOPEN/LOCKボタンのCSS, HTMLの終了)
  snprintf(temp, 1000,
         "<!DOCTYPE html><html>\
            <head>\
              <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\
              <link rel=\"icon\" href=\"data:,\">\
              <style>\
                html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\
                .button { background-color: #FFCC66; border: none; color: white; padding: 16px 40px;\
                text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}\
                .button2 {background-color: #555555;}\
              </style>\
            </head>\
            <body>\
              <h1>Switcher</h1>\
              <p>Switch1: %s</p>\
              %s\
              <p>Switch2: %s</p>\
              %s\
            </body>\
          </html>", switch1State, switch1Button, switch2State, switch2Button );
  
  server.send(200, "text/html", temp);
}

//our callback functions
//スイッチ
void controlSwitch1(EspalexaDevice* d){
  if (d == nullptr) return; //this is good practice, but not required

  Serial.print("Switch1 changed to ");
  if (d->getValue()){
    turnOnSwitch1();
    Serial.println("ON");
  }else {
    turnOffSwitch1();
    Serial.println("OFF");
  }
}

void controlSwitch2(EspalexaDevice* d){
  if (d == nullptr) return; //this is good practice, but not required

  Serial.print("Switch2 to ");
  if (d->getValue()){
    turnOnSwitch2();
    Serial.println("ON");
  }else {
    turnOffSwitch2();
    Serial.println("OFF");
  }
}



// connect to wifi – returns true if successful or false if not
boolean connectWifi(){
  boolean state = true;
  int i = 0;

// Set fixed IP address
  if (!WiFi.config(ip,ip,subnet)){
     Serial.println("Failed to configure!");
 }
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");
  Serial.println("Connecting to WiFi");

  // Wait for connection
  Serial.print("Connecting...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (i > 20){
      state = false; break;
    }
    i++;
  }
  Serial.println("");
  if (state){
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }
  else {
    Serial.println("Connection failed.");
  }
  delay(100);
  return state;
}

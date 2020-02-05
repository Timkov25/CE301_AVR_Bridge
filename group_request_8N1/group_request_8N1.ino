#include <ModbusRtu.h>
#include <GyverTimer.h>
#include <SoftwareSerial.h>

GTimer requestTimer(MS);
long Timer_val = 0;

int switch_case_val = 0;
int Time_1 = 1;
int Time_2 = 2;
int Time_3 = 3;
int Time_10 = 10;

// открываем сессию
byte init_message[] = {0x2f, 0x3f, 0x21, 0x0d, 0x0a}; //   /?!<CR><LF>
// задаем режим считывания
byte mode_message[] = {0x06,  0x30,  0x35,  0x31,  0x0d, 0x0a};   // <ACK>VZY<CR><LF>
// снимаем показания      SOH C D STX <DATA()> ETX BCC
byte data_message[] = {0x01, 0x52,  0x31,  0x02,  0x45, 0x54, 0x30, 0x50, 0x45, 0x28, 0x29,  0x03,  0x37};


#define RS485_PIN 8 // пин управления прием/передача

//--------Строка для software serial---------------
//SoftwareSerial RS485 (7, 6); // RX, TX

void setup() {
  //------Инициализация Serial------------------------
  Serial.begin(9600, SERIAL_7E1);

  //---------Инициализация Таймера--------------------
  requestTimer.setInterval(1000);

  //----------Инициализация пина RS-485----------------
  //Вторая строка, возможно, не нужна
  pinMode(RS485_PIN, OUTPUT);
  digitalWrite(RS485_PIN, HIGH);
}
void loop() {
  //Запрашиваем информацию
  request();
  
  
  read_bufer();

  Request_string
  
  
}
/*   Тот же switch case только через if (с форума)

    if (val == Time_1) {
      transmitt_on();
      RS485.write (init_message, 5);
      transmitt_off();
    }
    if (val == Time_2) {
      transmitt_on();
      RS485.write (mode_message, 6);
      transmitt_off();
    }
    if (val == Time_3) {
      transmitt_on();
      RS485.write (data_message, 13);
      transmitt_off();
    }
    if (val == Time_10) {
      val = 0;
    }

*/

void transmitt_on() {
  digitalWrite(RS485_PIN, HIGH);
}
void transmitt_off() {
  digitalWrite(RS485_PIN, LOW);
}

//----------Запрашиваем у счетчика информацию--------------
void request(){
  if (requestTimer.isReady()){
    switch_case_val ++;
    switch  (val) {
      case 1:
        transmitt_on();
        Serial.write (init_message, 5);
        transmitt_off();
        break;
      case 2:
        transmitt_on();
        Serial.write (mode_message, 6);
        transmitt_off();
        break;
      case 3:
        transmitt_on();
        Serial.write (data_message, 13);
        transmitt_off();
        break;
      case 10:
        switch_case_val = 0;
        break;
    }
  }
}

//-----------Собираем принятые данные по символу------------
char read_bufer () {
  if (Serial.available()) {
    char response = Serial.read();
    //    response &= 0x7F;// convert 8N1 to 7E1
    //    Serial.print(response);
    return response;
  }
}

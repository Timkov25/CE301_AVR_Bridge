#include <iostream>
#include <ModbusRtu.h>
#include <GyverTimer.h>
#include <SoftwareSerial.h>

//----- Инициализация таймеров -----
GTimer requestTimer(MS); //Таймер запросов
GTimer stringCollectTimer(MS);  //Таймаут сбора строки после запроса


int switch_case_val = 0;   //Переменная для запроса
bool  flag_string_collect = 0; //флаг сбора строки
bool flag_breaket_parse = 0; //флаг парсинга скобок

// Константы для таймера запросов
#define Time_init  1
#define Time_mode  2
#define Time_data  3
#define Time_end   10

String AnswerString;  //Строка для сбора ответа

//---------- Данные для парсера ----------
#define value 10
String buf;                     //Буферная строка для преобразования во float
float Ans_float[value] = {};    //Массив с ответами во float
String Ans_string[value] = {}; //Массив с ответами в string
int left_breaket[value] = {};   //Массив с позициями '('
int right_breaket[value] = {};  //Массив с позициями ')'
int left_breaket_counter = 0;  //Счетчик левых
int right_breaket_counter = 0; //              и правых скобок в строке


// открываем сессию групового опроса
byte init_message[] = {0x2f, 0x3f, 0x21, 0x0d, 0x0a}; //   /?!<CR><LF>
// задаем режим считывания
byte mode_message[] = {0x06,  0x30,  0x35,  0x31,  0x0d, 0x0a};   // <ACK>VZY<CR><LF>
// снимаем показания      SOH C D STX <DATA()> ETX BCC
byte data_message[] = {0x01, 0x52,  0x31,  0x02,  0x45, 0x54, 0x30, 0x50, 0x45, 0x28, 0x29,  0x03,  0x37};

//----- пин управления прием/передача для модуля ttl-rs485 -----
#define RS485_PIN 8

/*
   --------Строка для использования software serial---------------
  SoftwareSerial RS485 (7, 6); // RX, TX
*/
void setup() {
  //------------Инициализация Serial------------
  Serial.begin(9600, SERIAL_7E1);

  //---------Инициализация Таймера для запросов--------------------
  requestTimer.setInterval(1000);


  //----------Инициализация пина для модуля ttl-RS-485----------------
  //Вторая строка, возможно, не нужна
  pinMode(RS485_PIN, OUTPUT);
  digitalWrite(RS485_PIN, HIGH);
}
void loop() {

  request();

  string_collect();

  breaket_parser();


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
// ----------- Функции для корректной работы модуля ttl-rs485 -----------
void transmitt_on() {
  digitalWrite(RS485_PIN, HIGH);
}
void transmitt_off() {
  digitalWrite(RS485_PIN, LOW);
}

//----------Функция отправления запроса счетчику--------------
void request() {
  if (requestTimer.isReady()) {
    switch_case_val ++;
    switch  (switch_case_val) {
      case Time_init:
        transmitt_on();
        Serial.write (init_message, 5);
        transmitt_off();
        break;
      case Time_mode:
        transmitt_on();
        Serial.write (mode_message, 6);
        transmitt_off();
        break;
      case Time_data:
        transmitt_on();
        Serial.write (data_message, 13);
        transmitt_off();
        flag_string_collect = true;     //Устанавливаем флаг сбора строки
        stringCollectTimer.setTimeout(500);   //Запускаем таймаут сбора строки
        break;
      case Time_end:
        switch_case_val = 0;
        break;
    }
  }
}

//----------- Функция формирования ascii символа из принятых данных------------
char read_bufer () {
  if (Serial.available()) {
    char response = Serial.read();
    /*    Если передаваемый при Serial.begin параметр о паритете 7E1
           не заработает, то эта строка может заработать как кастыльная
           альтернатива

            response &= 0x7F;// convert 8N1 to 7E1
    */
    return response;
  }
}
//---------- Функция сбора принимаемых символов в строку ----------
void string_collect() {
  //  Собираем строку пока поднят флаг
  if (flag_string_collect) {
    AnswerString += read_bufer();
  }
  //  Опускаем флаг когда тайаут заканчивается
  if (stringCollectTimer.isReady()) {
    flag_string_collect = false;
    flag_breaket_parse = true;

  }
}

void breaket_parser() {
  if (flag_breaket_parse) {
    //----Ищем позиции скобок и сохряняем их в массивы
    for (int i = 0; i < AnswerString.length(); i++) {
      if (AnswerString[i] == '(') {
        left_breaket[left_breaket_counter] = i;
        left_breaket_counter++;
      }
      if (AnswerString[i] == ')') {
        right_breaket[right_breaket_counter] = i;
        right_breaket_counter++;
      }
    }
    //----Сохраняем данные из скобок в массивы с ответами----
    for (int i = 0; i < left_breaket_counter; i++) {
      Ans_string[i] = AnswerString.substring(left_breaket[i] + 1, right_breaket[i]);
      buf = AnswerString.substring(left_breaket[i] + 1, right_breaket[i]);
      Ans_float[i] = buf.toFloat();
    }


    //----Обнуляем счетчики скобок----
    left_breaket_counter = 0;
    right_breaket_counter = 0;

    //-----Опускаем флаг парсера-----
    flag_breaket_parse = false;
  }
}

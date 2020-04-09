#include <GyverTimer.h>
#include <SoftwareSerial.h>
#include <KRModbusRTUSlaveRS485.h>
#include <KRTON.h>
#include <KRRegisters.h>

class Data : public KRRegisters {
  public:
    int modbus_transmit[12] = { 0 };
    virtual uint16_t read(uint16_t index);
    virtual void write(uint16_t index, uint16_t value);
};

uint16_t Data::read(uint16_t index) {
  return modbus_transmit[index];
}

void Data::write(uint16_t index, uint16_t value) {
  modbus_transmit[index] = value;
}

//----- пины управления прием/передача для модуля ttl-rs485 со стороны счетчика и PLC -----
#define RS485_PIN 8
#define PIN_RS_MODBUS 3

//----- Инициализация таймеров -----
GTimer requestTimer(MS); //Таймер запросов
GTimer stringCollectTimer(MS);  //Таймаут сбора строки после запроса
int switch_case_val = 0; //Переменная для запроса
bool  flag_string_collect = false; //флаг сбора строки
bool flag_breaket_parse = false; //флаг парсинга скобок
bool flag_modbus_send = false; // флаг отправки на PLC

// -----  Константы для таймера запросов -----
#define Time_init  1
#define Time_mode  2
#define Time_data  3
#define Time_end   10

//  -----  Строка для сбора ответа  -----
String AnswerString;

//---------- Данные для парсера ----------
#define value_elements 10
String buf;                     //Буферная строка для преобразования во float
float Ans_float[value_elements] = {};    //Массив с ответами во float
String Ans_string[value_elements] = {}; //Массив с ответами в string
int left_breaket[value_elements] = {};   //Массив с позициями '('
int right_breaket[value_elements] = {};  //Массив с позициями ')'

//-------------- Чистые байты в запросах  --------------
//// открываем сессию групового опроса
//byte init_message[] = {0x2f, 0x3f, 0x21, 0x0d, 0x0a}; //   /?!<CR><LF>
//// задаем режим считывания
//byte mode_message[] = {0x06,  0x30,  0x35,  0x31,  0x0d, 0x0a};   // <ACK>VZY<CR><LF>
//// снимаем показания      SOH C D STX <DATA()> ETX BCC
//byte data_message[] = {0x01, 0x52,  0x31,  0x02,  0x45, 0x54, 0x30, 0x50, 0x45, 0x28, 0x29,  0x03,  0x37};

//-------------- Вручную пересчитанная четность в запросах  --------------
// открываем сессию групового опроса
byte init_message[] = {0xaF, 0x3F, 0x21, 0x8D, 0x0A}; //   /?!<CR><LF>
// задаем режим считывания
byte mode_message[] = {0x06, 0x30, 0x35, 0xb1, 0x8d, 0x0a}; // <ACK>VZY<CR><LF>
// напряжение на фазах      SOH C D STX <DATA()> ETX BCC
byte data_message[] = {0x81, 0xd2, 0xb1, 0x82, 0xc5, 0xd4, 0x30, 0x50, 0xc5, 0x28, 0xa9, 0x03, 0xb7};

//----- Инициализация Modbus и Массив для него ------
Data data;
KRModbusRTUSlaveRS485 mb(Serial, data, PIN_RS_MODBUS, 1);
int arr_int_to_PLC [12] = {};
//float test_float_to_int [6] = {};

//-----Инициализация software serial -----------
SoftwareSerial RS485 (10, 11); // RX, TX

void setup() {
  //------------Инициализация Serial (стандартного?)------------
  Serial.begin(9600);
  RS485.begin(9600);

  //---------Инициализация Таймера для запросов--------------------
  requestTimer.setInterval(1000);

  //----------Инициализация пина для модуля TTL-RS485----------------
  pinMode(RS485_PIN, OUTPUT);
  digitalWrite(RS485_PIN, HIGH);
}
void loop() {
  mb._DO();

  request();

  string_collect();

  breaket_parser();

  modbus_send();
}

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
      case Time_init:                //(1)
        //        Serial.println("Открыта сессия группового опроса  ");
        transmitt_on();
        RS485.write (init_message, 5);
        transmitt_off();
        break;
      case Time_mode:                 //(2)
        //        Serial.println("Задан режим считывания ");
        transmitt_on();
        RS485.write (mode_message, 6);
        transmitt_off();
        break;
      case Time_data:                 //(3)
        //        Serial.println("Запрашиваем данные ");
        transmitt_on();
        RS485.write (data_message, 13);
        transmitt_off();
        flag_string_collect = true;     //Устанавливаем флаг сбора строки
        stringCollectTimer.setTimeout(700);   //Запускаем таймаут сбора строки
        break;
      case Time_end:               //(10)
        //        Serial.println("Запрос заново: ");
        switch_case_val = 0;
        AnswerString = "";
        for (int i = 0; i < value_elements; i++) {
          Ans_float[i] = 0;
          Ans_string[i] = "";
        }
        break;
    }
  }
}

//---------- Функция сбора принимаемых символов в строку ----------
void string_collect() {
  //  Собираем строку пока поднят флаг
  if (flag_string_collect) {
    //    Serial.println("Строка собирается");
    //    AnswerString += read_bufer();
    read_bufer();
  }
  //  Опускаем флаг когда тайаут заканчивается
  if (stringCollectTimer.isReady()) {
    flag_string_collect = false;
    flag_breaket_parse = true;
    //    Serial.println("Строка собрана,  Answerstring - " + AnswerString);
  }
}


//----------- Функция формирования ASCII символа из принятых данных------------
void read_bufer () {
  if (RS485.available()) {
    //    Serial.println("Символ есть");
    char response = RS485.read();
    response &= 0x7F;// convert 8N1 to 7E1
    //    Serial.print(response);
    AnswerString += response;
  }
}
// ------ Функция ищет данные в скобках и формирует из них 6 переменных float------
void breaket_parser() {
  int left_breaket_counter = 0;  //Счетчик левых
  int right_breaket_counter = 0; //              и правых скобок в строке
  if (flag_breaket_parse) {
    int ignore_L = false;
    int ignore_R = false;
    //    Serial.println("Парсим скобки");
    //----Ищем позиции скобок и сохряняем их в массивы
    for (int i = 0; i < AnswerString.length(); i++) {
      if ( AnswerString[i] == '(') {
        if (ignore_L) {           //Пропускаем первые скобки
          left_breaket[left_breaket_counter] = i;
          left_breaket_counter++;
        }
        ignore_L = true;
      }
      if ( AnswerString[i] == ')') {
        if (ignore_R) {
          right_breaket[right_breaket_counter] = i;
          right_breaket_counter++;
        }
        ignore_R = true;
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
    //-----Поднимаем флаг отправки на PLC -----
    flag_modbus_send = true;
  }
}

void modbus_send() {
  if (flag_modbus_send) {
    //    Serial.println("Отправляем данные: ");
    memcpy( &data.modbus_transmit , &Ans_float , sizeof(float) * 6 );
    //    memcpy( &test_float_to_int , &data.modbus_transmit , sizeof(float) * 6 );

    //Опускаем флаг отправки на PLC
    flag_modbus_send = false;


    //     --------- Отладочный вывод данных ---------
    //      Serial.println("Ans_float: ");
    //      for(int i = 0;i<6;i++){
    //        Serial.print("Ans_float ");
    //        Serial.print(i);
    //        Serial.print("  -  ");
    //        Serial.println(Ans_float[i]);
    //      }
    //      Serial.println("Test_float_to_int ");
    //        for(int i = 0;i<6;i++){
    //        Serial.print("Test_float_to_int ");
    //        Serial.print(i);
    //        Serial.print("  -  ");
    //        Serial.println(test_float_to_int[i]);
    //        }
    //      Serial.println("Test_int ");
    //        for(int i = 0;i<12;i++){
    //        Serial.print("Test_int ");
    //        Serial.print(i);
    //        Serial.print("  -  ");
    //        Serial.println(data.modbus_transmit[i]);
    //        }
  }
}

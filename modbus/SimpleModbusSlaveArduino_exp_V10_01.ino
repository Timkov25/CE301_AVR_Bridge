/* 
   SimpleModbusSlaveV10 supports function 3, 6 и 16.
   serial ring buffer 64 bytes or 32 registers.
   function 3       58 bytes or 29 registers.
   function 16      54 bytes or 27 registers.
*/
#include <SimpleModbusSlave.h>
//////////////////// Макроопределения портов и настройки программы  ///////////////////
#define TxEnablePin  2	   // Tx/Rx пин RS485
#define baud         9600  // скоростьобмена по последовательному интерфейсу. (UART)
#define timeout      1000  // Длительность ожидание ответа (таймаут modbus)
#define polling      200   // скорость опроса по modbus
#define retry_count  10    // количесво запросов modbus до ошибки и останова обмена
#define Slave_ID     1     // Адрес Slave устройсва
#define LED1         13    // светодиод 1
#define LED2         9     // светодиод 2
const int  buttonPin = 3;     // номер входа, подключенный к кнопке
// переменные
int buttonState = 0;         // переменная для хранения состояния кнопки
 

//////////////// Регистры вашего Slave ///////////////////
enum 
{     
//Просто добавьте или удалите регистры. Первый регистр начинается по адресу 0
  slave_to_master_val_1,          //  с адресом массива 0
  slave_to_master_val_2,          //  с адресом массива 1
  master_to_slave_val_1,          //  с адресом массива 2
  master_to_slave_val_2,          //  с адресом массива 3
  HOLDING_REGS_SIZE // Это не удалять размер массива HOLDING_REGS.
  //общее количество регистров для функции 3 и 16 разделяет тотже самый массив регистров
  //т.е. то же самое адресное пространство
};
unsigned int holdingRegs[HOLDING_REGS_SIZE]; // функции 3 и 16 массив регистров
////////////////////////////////////////////////////////////

void setup()
{
  // инициализируем пин, подключенный к кнопке, как вход
  pinMode(buttonPin, INPUT);    
  
  /* parameters(HardwareSerial* SerialPort,long baudrate,unsigned char byteFormat,unsigned char ID, 
     unsigned char transmit enable pin,unsigned int holding registers size,unsigned int* holding register array)
     SERIAL_8N2: 1 start bit, 8 data bits, 2 stop bits
     SERIAL_8E1: 1 start bit, 8 data bits, 1 Even parity bit, 1 stop bit
     SERIAL_8O1: 1 start bit, 8 data bits, 1 Odd parity bit, 1 stop bit
     SERIAL_8N1 option 
  */
	
   modbus_configure(&Serial, baud, SERIAL_8N1, Slave_ID, TxEnablePin, HOLDING_REGS_SIZE, holdingRegs);
   modbus_update_comms(baud, SERIAL_8N1, 1);  
   pinMode(LED1, OUTPUT);
   pinMode(LED2, OUTPUT);
}// конец void setup()

void loop()
{
  int temp;
 // считываем значения с входа кнопки
  buttonState = digitalRead(buttonPin);
  // проверяем нажата ли кнопка
  // если нажата, то buttonState будет HIGH:
  if (buttonState == HIGH){temp=255;}else{temp=0;} 
  //if (temp == 255){digitalWrite(LED1, HIGH);}else{digitalWrite(LED1, LOW);}
  modbus_update(); // запуск обмена по Modbus
  
  holdingRegs[slave_to_master_val_1] = analogRead(A0); // запись данных slave-master 
                                                       // (регистр 0), значение из аналогового входа 0.
  holdingRegs[slave_to_master_val_2] = temp;           // запись данных slave-master 
                                                       // (регистр 1), запись значения переменной temp. по нажатию кнопки.
  // чтение данных master-slave (регистр 2)
  if (holdingRegs[master_to_slave_val_1] == 255){digitalWrite(LED1, HIGH);}else{digitalWrite(LED1, LOW);}
  // если пришло 255 зажигаем светодиод 
  analogWrite(LED2, holdingRegs[master_to_slave_val_2]>>2);  // чтение данных master-slave (регистр 3) 
  // (ограничеть количесво бит данных числом 255), прочитаное значение выводим шимом на аналоговый выход pin9
}// конец void loop()




  /* 
     Использование enum инструкции не обязательно. Вы могли установить допустимый максимум
     размер для holdinRegs [], определяя HOLDING_REGS_SIZE, используя константу и затем доступ 
     holdingRegs[] обращением по "Index" массива. 
     holdingRegs[0] = analogRead(A0);
     analogWrite(LED, holdingRegs[1]/4);
     holdingRegs[ADC_VAL] = 250;                // данные об обновлении, которые будут прочитаны владельцем, чтобы приспособить PWM ШИМ
     analogWrite(LED, holdingRegs[PWM_VAL]>>2); // ограничьте АЦП arduino значением 255
     holdingRegs[ADC_VAL] = holdingRegs[PWM_VAL];
  */

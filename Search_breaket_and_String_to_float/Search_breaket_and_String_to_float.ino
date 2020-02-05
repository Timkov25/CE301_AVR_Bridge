#define value 10   //Максимальное значение ответов в скобках
//----Тестовая строка----
String string0 = String("Ща_будут_данные(123.456)и_еще_данные(34.234)а_сейчас_не_будет(544.456)Ха_ха_обманул(123.12)два_манула(654.321)три_манула");
String Ans_string[value] = {}; //Массив с ответами в string
String buf;                     //Буферная строка для преобразования во float
float Ans_float[value] = {};    //Массив с ответами во float
int left_breaket[value] = {};   //Массив с позициями '('
int right_breaket[value] = {};  //Массив с позициями ')'
int left_breaket_counter = 0;  //Счетчик левых 
int right_breaket_counter = 0; //              и правых скобок в строке

void setup() {
  Serial.begin(9600);


//----Ищем позиции скобок и сохряняем их в массивы
  for (int i = 0; i < string0.length(); i++) {
    if (string0[i] == '(') {
      left_breaket[left_breaket_counter] = i;
      left_breaket_counter++;
    }
    if (string0[i] == ')') {
      right_breaket[right_breaket_counter] = i;
      right_breaket_counter++;
    }
  }
//----Сохраняем данные из скобок в массивы с ответами----
  for (int i = 0; i < left_breaket_counter; i++) {
    Ans_string[i] = string0.substring(left_breaket[i] + 1, right_breaket[i]);
    buf = string0.substring(left_breaket[i] + 1, right_breaket[i]);
    Ans_float[i] = buf.toFloat();
  }


//---- Выводим ответы в float ----
  for (int i = 0; i < left_breaket_counter ; i++) {
    Serial.print("Ans: ");
    Serial.println(Ans_float[i]);
  }
//----Обнуляем счетчики скобок----
  left_breaket_counter = 0;
  right_breaket_counter = 0;

}
void loop() {
  // put your main code here, to run repeatedly:

}

#include "setup.h"
#include <stdint.h>
#include <util/crc16.h>
#include <PS2X_lib.h>
#include <Wire.h>
// Подключение библиотеки LCD дисплея. Подключается к А4 и А5 контактам на ардуино UNO
#include <LiquidCrystal_I2C.h>

unsigned long LastTimeControl; //Последнее время срабатывания опроса Джостиков
unsigned long LastTimeLCD; // Последнее время вывода на экран

// Создаем объекты:
// Экран
// Указываем I2C адрес (наиболее распространенное значение),
// а также параметры экрана (в случае LCD 1602 - 2 строки по 16 символов в каждой
LiquidCrystal_I2C lcd(0x27, 16, 2);
PS2X ps2x; // Обьект типа Джостик

//right now, the library does NOT support hot pluggable controllers, meaning
//you must always either restart your Arduino after you conect the controller,
//or call config_gamepad(pins) again after connecting the controller.
int error = 0;
byte type = 0;
byte vibrate = 0;

// Массив для приема значений по Serial1
//(пока только значения влажности и температуры, будет использоваться только 3 элемента созданного массива).
int ArrData[10]; //максимально 10 штук, включая CRC, можно до 100.

// Массив для передачи значений значений ходовым и дифферентным двигателям
// 4 ходовых и 2 дифферентных двигателя 2 сервы для камеры и манипулятора, и лампа и контрольная сумма,
// итого 10 элементов массива для передачи наутилусу.
int ArrMotor[10], ArrMotorOld[10];

//>===============================================================================
void setup() {

  Serial.begin(SerialSpeed); //Инициализируем встроенный com для отладки
  while (!Serial);
  Serial1.begin(SerialSpeed); // Инициализируем программный com для передачи и приема данных.
  while (!Serial1);

  //setup pins and settings:  GamePad(clock, command, attention, data, Pressures?, Rumble?) check for error
  error = ps2x.config_gamepad(PS_CLK_PIN, PS_D0_PIN, PS_CS_PIN, PS_D1_PIN, pressures, rumble);
  if (error == 0) {
    Serial.println("Found Controller, configured successful");
    Serial.println("Try out all the buttons, X will vibrate the controller, faster as you press harder;");
    Serial.println("holding L1 or R1 will print out the analog stick values.");
    Serial.println("Go to www.billporter.info for updates and to report bugs.");
  }
  else if (error == 1)
    Serial.println("No controller found, check wiring, see readme.txt to enable debug. visit www.billporter.info for troubleshooting tips");
  else if (error == 2)+-
    Serial.println("Controller found but not accepting commands. see readme.txt to enable debug. Visit www.billporter.info for troubleshooting tips");
  else if (error == 3)
    Serial.println("Controller refusing to enter Pressures mode, may not support it. ");
  Serial.print(ps2x.Analog(1), HEX);

  type = ps2x.readType();
  switch (type) {
    case 0:
      Serial.println("Unknown Controller type");
      break;
    case 1:
      Serial.println("DualShock Controller Found");
      break;
    case 2:
      Serial.println("GuitarHero Controller Found");
      break;
  }

  lcd.init();                      // Инициализация дисплея
  lcd.backlight();                 // Подключение подсветки
  lcd.setCursor(0, 0);             // Установка курсора в начало первой строки
  lcd.print("Hello");              // Набор текста на первой строке
  lcd.setCursor(0, 1);             // Установка курсора в начало второй строки
  lcd.print("NautilusMaster");     // Набор текста на второй строке

  LastTimeControl = millis();      // Начальные значения времени.
  LastTimeLCD = millis();

  delay(2000);
  lcd.clear();
}
//<===============================================================================

//>===============================================================================
void loop() {
  // Опрос джойстика
  //>-----------------------------------------------------------------------------
  if ( millis() - LastTimeControl >= TimeOutControl ) {

    CalculateArray();

    // Если значения полученные с джостика изменились, то отправляем данные
    int len = sizeof(ArrMotor);
    for (int i = 0; i < (len / 2) - 1; i++) {
      if (ArrMotor[i] != ArrMotorOld[i]) { // Если изменились данные с джостика, то
        //Serial.println("changed");
        SendData(); // передаем вычисленные значения на наутилус.
        memcpy(ArrMotorOld, ArrMotor, sizeof(ArrMotor));
        // Обнуляем значения изменения для Сервы.
        ArrMotor[6] = 0;
        ArrMotor[7] = 0;
        break;
      }
    }
    LastTimeControl = millis(); // Время окончания работы
  }
  //<-----------------------------------------------------------------------------

  //Вывод на дисплей полученной информации с датчиков на наутилусе
  //>-----------------------------------------------------------------------------
  if ( millis() - LastTimeLCD >= TimeOutLCD ) {
    // Получаем данные и выводим на экран показания полученные с Наутилуса.
    Display();
    LastTimeLCD = millis(); // Время окончания работы
  }
  //<-----------------------------------------------------------------------------
}
//<===============================================================================

//>===============================================================================
void CalculateArray() {
  int J1_Val_X, J1_Val_Y, J2_Val_X, J2_Val_Y, i, m;
  float k;
  // Пересчитаем в значения от -10 до 10
  /*
    Analog(PSS_LY) это будет движение вперед (0-255)
    Analog(PSS_LX) - поворот влево или враво.(0-255)
    Analog(PSS_RY) - управление дифферентом.(0-255)
    Analog(PSS_RX) - движение в бок?(0-255)
  */

  //Считали показания с джойстика.
  ps2x.read_gamepad(false, vibrate);

  // Значения становятся от -127 до 127
  // Задается вектор движения. Вопрос как его рассчитать? Непонятно пока :)
  // Зададим градацию векторов 0-7 ( 127/18 )
  J1_Val_X = (ps2x.Analog(PSS_LX) - 127) / 18; //вектор кручения
  J1_Val_Y = -(ps2x.Analog(PSS_LY) - 127) / 18; // вектор движения вперед
  J2_Val_X = (ps2x.Analog(PSS_RX) - 127) / 18; // вектор сдвига вправо
  J2_Val_Y = -(ps2x.Analog(PSS_RY) - 127) / 18; // вектор дифферентных моторов

  //========================================================
  // А теперь вопрос? Как расчитать вращение двигателей?
  // Расчет работы ходовых двигателей. Первый вариант расчета вектора движения.
  //  ArrMotor[0] = constrain(J1_Val_Y + J1_Val_X + J2_Val_X, -127, 127);
  //  ArrMotor[1] = constrain(J1_Val_Y - J1_Val_X - J2_Val_X, -127, 127);
  //  ArrMotor[2] = constrain(-J1_Val_Y - J1_Val_X + J2_Val_X, -127, 127);
  //  ArrMotor[3] = constrain(-J1_Val_Y + J1_Val_X - J2_Val_X, -127, 127);

  // здесь попробуем учесть значения выходяще за границы.,,,
  // Второй вариант рассчета. Если обозначить через a число 127, то проведя несложные математические преобразование из верхних многочленов получим
  ArrMotor[0] = J1_Val_Y + J1_Val_X + J2_Val_X;
  ArrMotor[1] = J1_Val_Y - J1_Val_X - J2_Val_X;
  ArrMotor[2] = -J1_Val_Y - J1_Val_X + J2_Val_X;
  ArrMotor[3] = -J1_Val_Y + J1_Val_X - J2_Val_X;
  //  // далее ищем максимальный по модулю элемент в массиве. Это будет ведущий мотор с максимальной мощностью
  //  // Далее вычисляем коэфициет маштабирования
  m = abs(ArrMotor[0]);
  for (i = 1; i < 4; i++) {
    if (abs(ArrMotor[i]) > m) {
      m = abs(ArrMotor[i]);
    }
  }
  if (m > 7) {
    // Вычисляем значения моторов относительно ведущего мотора и максимального значения вектора?
    for (i = 0; i < 4; i++) {
      ArrMotor[i] = round(7 * ArrMotor[i] / m);
    }
  }

  //Расчет работы дифферентных двигателей (делаем дифферент, если нужно будет движение вверх, нужно зажать кнопку или переключить как-то по другому режим)
  // Тут все просто - либо крутим либо нет
  //  ArrMotor[4] = constrain(J2_Val_Y, -127, 127);
  //  ArrMotor[5] = constrain(-J2_Val_Y, -127, 127);
  ArrMotor[4] = J2_Val_Y;
  ArrMotor[5] = -J2_Val_Y;


  // Выводим показания на дисплей
  //lcd.clear();
  char str[15];
  sprintf(str, "%2d%2d%2d%2d%3d%2d", ArrMotor[0], ArrMotor[1], ArrMotor[2], ArrMotor[3], ArrMotor[4], ArrMotor[5]);
  lcd.setCursor(0, 0);
  lcd.print(str);

  // Считываем нажатия кнопок. Это кол-во градусов на которое нужно сдвинуть серву. Это значение после отправки будем обнулять.
  /*
    Button(PSB_L1) - Сжать манипулятор
    Button(PSB_L2) - Разжать манипулятор

    Button(PSB_R1) - движение камерой в одну сторону
    Button(PSB_R2) - движение камерой в другую сторону

    ArrMotor[6]  серводвигатель камеры
    ArrMotor[7]  серводвигатель манипулятора
  */

  if (ps2x.Button(PSB_L1)) { //will be TRUE if button was JUST pressed
    ArrMotor[7] = ArrMotor[7] + IncServ;
    //    Serial.print ("ButtonServPlus");
  }
  if (ps2x.Button(PSB_L2)) {
    ArrMotor[7] = ArrMotor[7] - IncServ;
    //   Serial.print ("ButtonServMinus");
  }
  if (ps2x.Button(PSB_R1)) {
    ArrMotor[6] = ArrMotor[6] + IncServ;
  }
  if (ps2x.Button(PSB_R2)) {
    ArrMotor[6] = ArrMotor[6] - IncServ;
  }

}
//<===============================================================================

// Получаем данные с наутилуса и выводим на дисплей показания датчиков
//>===============================================================================
void Display() {

  int len, i; // переменные для служебных надобностей.
  uint16_t crc = 0; // для подсчета контрольной суммы.
  bool noerr = true; // флаг для контроля ошибок

  if (Serial1.available()) // если есть данные в буфере.
  {
    if (noerr = (noerr && Serial1.find("<data:"))) // проверка на наличие начального маркера
    {
      //   Serial.println("Found start tag");
      len = Serial1.parseInt();
      //     Serial.print("Ints to read: "); Serial.println(len / 2);
      noerr = noerr && len < 200; // проверка на количество переменных в буфере, каждое число 2 байта, таким образом можно закодировать числа от -32 768 до 32 767
      noerr = noerr && Serial1.find(">"); // Подобные конструкции упрощают написание условий... Используется алгебра логики...
      Serial1.readBytes((byte *)ArrData, len); // вот в этой строке массив целых принимается и записывается в массив ArrData
      noerr = noerr && Serial1.find("</data>"); // Проверка на получение завершающего маркера

      for (i = 0; i < len - 2; i++) crc = _crc16_update(crc, *((byte *)ArrData + i)); // Считаем контрольную сумму crc по значениям ArrData полученного массива
      if (noerr && crc == ArrData[len / 2 - 1]) // Сверяем ее с контрольной суммой полученной при передаче массива, в последней ячейке массива и если совпадает, то...
      {
        /*  Serial.print ("Received "); //Выводим данные в сомпорт. для дебага
          Serial.print (len / 2);
          Serial.print (" int Array. CRC - OK = ");
          Serial.println (crc);
          for (i = 0; i < len / 2 - 1; i++) {
            Serial.print("ArrData[");
            Serial.print(i);
            Serial.print ("]=");
            Serial.print(ArrData[i]);
            Serial.print ("; ");
          }
          Serial.println ();
          Serial.println ();
        */
        //Формируем и выводим на LCD экран строку
        char str[15];
        // [0] Датчик влажности
        // [1] Датчик температуры
        // [2] Датчик давления(глубины)
        sprintf(str, "%3d%6d%5d m", ArrData[0], ArrData[1], map(ArrData[2],0,1024,0,122)-14);
        lcd.setCursor(0, 1);
        lcd.print(str);
      }
      else // это обработка ошибок.
      { // выводим значения несовпадающих crc...
        Serial.print ("Syntax or CRC error!  ");
        Serial.print (noerr);
        Serial.print (" CRC recv/send");
        Serial.print(crc, 16);
        Serial.print (" / ");
        Serial.print(ArrData[len / 2 - 1], 16);
        Serial.println();
      }
    }
  }
}
//<===============================================================================

// Отправляем данные наутилусу.
//>===============================================================================
void SendData() {

  // Отправляем массив по программному ком порту.
  int len = sizeof(ArrMotor); // Считаем количество байтов, отданных под массив
  uint16_t  crc = 0; //CRC для массива (можно понимать как контрольная сумма)

  //Считаем контрольную сумму массива
  for (int i = 0; i < (len / 2) - 1; i++) {
    crc = _crc16_update(crc, ArrMotor[i] & 0xff); //.. считаем ЦРЦ
    crc = _crc16_update(crc, ArrMotor[i] >> 8);
  }

  ArrMotor[len / 2 - 1] = crc;
  len = sizeof(ArrMotor);

  /*Serial.print("Array complete CRC is: 0x");
    Serial.println(crc);
    Serial.print("Array length = ");
    Serial.println(len / sizeof(int));
  */

  Serial1.print("<data:");
  Serial1.print(len);
  Serial1.print(">");
  Serial1.write ((byte *)ArrMotor, len); //вот в этой строке массив целых передается
  Serial1.print("</data>");
  delay(2);
}
//<===============================================================================

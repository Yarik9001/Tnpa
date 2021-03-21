#include "setup.h"
#include <stdint.h>
#include <util/crc16.h>
#include <OneWire.h> // Для датчика температуры
#include <DallasTemperature.h> // Для датчика температуры

//#include <Servo.h> //Библиотека работы с моторами.
#include <Servo_Hardware_PWM.h> (2,3,7,8,44,45)

unsigned long LastTimeMotor; //Последнее время срабатывания принятия команд для двигателей
unsigned long LastTimeReceive; //Последнее время срабатывания принятия команд для двигателей
unsigned long LastTimeData; // ............... опроса датчиков

Servo motor[8]; // 8 Моторов, причем 2 сервы, и 6 бесколлекторников

OneWire oneWire(OneWirePin);
DallasTemperature sensors(&oneWire);

int ValueCam = 90;
int ValueMan = 90;

// Указываем количество передаваемых элементов, в число которых входит и CRC.
// Можно выбрать размер от 1 до 99  (числа от -32 768 до 32 767)
// нумерация элементов начиначется с 0, т.е ArrData[0] до ArrData[2]
int ArrData[4];
// 2 датчика - значит массив значений состоит из 2 элементов и еще crc значения.
int ArrMotor[10]; //Массив значений, которые приходят с пульта
int ToMotor[10]; // Значения в моторах будет стремится к этим значениям, туда записываются значения с правильной crc.
int ValMotor[N] = {0, 0, 0, 0, 0, 0}; // Значения скорости, которые подаем на моторы.

//>==========================================================================================
void setup() {
  // initialize serial communication at 9600 bits per second:
  Serial.begin(SerialSpeed);
  while (!Serial);
  Serial1.begin(SerialSpeed);
  Serial1.setTimeout(50);
  LastTimeMotor = millis(); // Начальное значение времени
  LastTimeData = millis();
  motor[4].attach(DiMotorFrontPin, MinDrive, MaxDrive);
  motor[5].attach(DiMotorBackPin, MinDrive, MaxDrive);
  motor[0].attach(MotorPin1, MinDrive, MaxDrive);
  motor[1].attach(MotorPin2, MinDrive, MaxDrive);
  motor[2].attach(MotorPin3, MinDrive, MaxDrive);
  motor[3].attach(MotorPin4, MinDrive, MaxDrive);
  delay(2000);
  for (int i = 0; i < N; i++) {
    motor[i].writeMicroseconds(MaxDrive); // Для инициализации моторов задаем сначала максимальное значение.
  }
  delay(2000);
  for ( int i = 0; i < N; i++) {
    motor[i].writeMicroseconds(MinDrive); // потом минимальное значение (ШИМ?)
  }
  delay(2000);
  for ( int i = 0; i < N; i++) {
    motor[i].writeMicroseconds(StopDrive); // И наконец среднее значение (ШИМ?)
  }
  delay(4000);
  motor[6].attach(ServoPinCam); // Инициализируем сервы управления манипулятором и камерой
  motor[7].attach(ServoPinMan);
  delay(500);
  motor[6].write(ValueCam);
  motor[7].write(ValueMan);
  //установим начальные положения бесколлекторных моторов
  for ( int i = 0; i < N; i++) {
    ValMotor[i] = StopDrive;
  }
}
//<==========================================================================================

/*
   1) Плавно изменяем значения оборотов двигателей через небольшой промежуток времени.
   2) Получаем данные с блока управления и записываем их в массив.
   3) через какое-то время опрашиваем датчики и шлем их показания суперпульту.
*/
//>==========================================================================================
void loop() {

  if ( millis() - LastTimeMotor >= TimeOutMotor ) {
    DriveMotor(); // здесь будем плавно повышать и понижать обороты
    LastTimeMotor = millis(); // Время окончания работы
  }

  // Получем значения для двигателей
  if ( millis() - LastTimeReceive >= TimeOutReceive ) {
    ReceiveData(); // Получаем Значения для моторов
    LastTimeReceive = millis(); // Время окончания работы
  }

  //Считываем показания датчиков и посылаем суперпульту
  if ( millis() - LastTimeData >= TimeOutData ) {
    ReadData(); // Читаем показания датчиков
    SendData(); // Посылаем считанное блоку управления
    LastTimeData = millis(); // Время окончания работы
  }
}

//>==========================================================================================
void DriveMotor() {
  // Значения моторов должно "догонять" значения которые прислали с пульта.
  for (int i = 0; i < N; i++) {
    if (ValMotor[i] != ToMotor[i]) {
      if (ValMotor[i] < ToMotor[i]) {
        ValMotor[i] = constrain(ValMotor[i] + 1, -7, 7);
      } else {
        ValMotor[i] = constrain(ValMotor[i] - 1, -7, 7);
      }
      motor[i].writeMicroseconds(map(ValMotor[i], -7, 7, StopDrive - 500, StopDrive + 500));
    }
  }
  // Серводвигатель камеры
  if (ToMotor[6] != 0) { //Если пришло ненулевое значение. Если ноль, то и дергаться нефиг
    ValueCam = ValueCam + ToMotor[6];
    ToMotor[6] = 0; // Обнуляем значение, чтобы двигатель подвинулся только на 2 деления
    ValueCam = constrain(ValueCam, 0, 180);
    motor[6].write(ValueCam);
    //Serial.print(ValueCam);
  }
  // Серводвигатель манипулятора
  if (ToMotor[7] != 0) { //Если пришло ненулевое значение. Если ноль, то и дергаться нефиг
    ValueMan = ValueMan + ToMotor[7];
    ToMotor[7] = 0; // Обнуляем значение, чтобы двигатель подвинулся только на 2 деления
    ValueMan = constrain(ValueMan, 0, 180);
    motor[7].write(ValueMan);
    delay(2);
    //Serial.print(ValueMan);
  }
}
//<==========================================================================================

// Получаем значения с блока управления
//>==========================================================================================
void ReceiveData() {

  int len, i; // переменные для служебных надобностей.
  uint16_t crc = 0; // для подсчета контрольной суммы.
  bool noerr = true; // флаг для контроля ошибок
  if (Serial1.available()) // если есть данные в буфере.
  {
    if (noerr = (noerr && Serial1.find("<data:"))) // проверка на наличие начального маркера
    {
      //    Serial.println("Found start tag");
      len = Serial1.parseInt();
      //    Serial.print("Ints to read: "); Serial.println(len / 2);
      noerr = noerr && len < 200; // проверка на количество переменных в буфере, каждое число 2 байта, таким образом можно закодировать числа от -32 768 до 32 767
      noerr = noerr && Serial1.find(">"); // Подобные конструкции упрощают написание условий... Используется алгебра логики...
      Serial1.readBytes((byte *)ArrMotor, len); // вот в этой строке массив целых принимается и записывается в массив ArrMotor
      noerr = noerr && Serial1.find("</data>"); // Проверка на получение завершающего маркера

      for (i = 0; i < len - 2; i++) crc = _crc16_update(crc, *((byte *)ArrMotor + i)); // Считаем контрольную сумму crc по значениям ArrMotor полученного массива
      if (noerr && crc == ArrMotor[len / 2 - 1]) // Сверяем ее с контрольной суммой полученной при передаче массива, в последней ячейке массива и если совпадает, то...
      {

        //
        memcpy(ToMotor, ArrMotor, sizeof(ArrMotor));

        //
        //         Serial.print ("Received ");
        //          Serial.print (len / 2);
        //          Serial.print (" int Array. CRC - OK = ");
        //          Serial.println (crc);
        //          for (i = 0; i < len / 2 - 1; i++) {
        //            Serial.print("ArrMotor[");
        //            Serial.print(i);
        //            Serial.print ("]=");
        //            Serial.print(ArrMotor[i]);
        //            Serial.println ("; ");
        //            Serial.println (ToMotor[i]);
        //          }
        //          Serial.println ();
        //
      }
      else // это обработка ошибок
      { // выводим значения несовпадающих crc...
        Serial.print ("Syntax or CRC error!  ");
        //        Serial.print (noerr);
        //        Serial.print (" CRC recv/send");
        //        Serial.print(crc, 16);
        //        Serial.print (" / ");
        //        Serial.print(ArrMotor[len / 2 - 1], 16);
        //        Serial.println();
      }
    }
  }


}
//<==========================================================================================

//>==========================================================================================
void ReadData() {
  // Получаем значение датчика влажности
  ArrData[0] = analogRead(WaterPin);

  // Получаем значение датчика температуры
  sensors.requestTemperatures(); // опрашиваем датчики на OneWire(Температура);
  ArrData[1] = sensors.getTempCByIndex(0);

  // Получаем и считаем значение датчика глубиры.
  int raw = analogRead(DepthPin);
//  float voltage = (float)raw * 5.0 / 1024.0; //тут не ровно 5 вольт, нужно смотреть как брать правильное напряжение....
//  float  pressure_kpa = (voltage - 0.5) / 4.0 * 2400.0;
//  float pressure_psi = pressure_kpa * 0.14503773773020923 / 14 + 1;
//  float glubina = pressure_psi - 1;
  ArrData[2] = round(raw);
}
//<==========================================================================================

//>==========================================================================================
void SendData() {
  int len = sizeof(ArrData); // Считаем количество байтов, отданных под массив
  uint16_t  crc = 0; //CRC для массива (можно понимать как контрольная сумма)

  //Считаем контрольную сумму массива
  for (int i = 0; i < (len / 2) - 1; i++) {
    crc = _crc16_update(crc, ArrData[i] & 0xff); //.. считаем ЦРЦ
    crc = _crc16_update(crc, ArrData[i] >> 8);
  }

  ArrData[len / 2 - 1] = crc;
  len = sizeof(ArrData);

  //Serial.print("Array complete CRC is: 0x");
  //Serial.println(crc);
  //Serial.print("Array length = ");
  //Serial.println(len / sizeof(int));

  Serial1.print("<data:");
  Serial1.print(len);
  Serial1.print(">");
  Serial1.write ((byte *)ArrData, len); //вот в этой строке массив целых собственно и передается
  Serial1.print("</data>");
}
//<==========================================================================================

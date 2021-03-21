
/*
  Пример работает в паре с SerialIntArrayTx

  Программа получает по софт-сериалу данные в формате
  <data:xy>zzzzzzzzzzzzzzzzzCRC16</data>
  где xy - байт в передаче, а zzz... собственно сами двубайтные целые числа подряд в формате "первый байт младший"

  Считаем ЦРЦ библиотечной функцикй от 0

*/
#include <stdint.h>
#include <util/crc16.h>
#include <SoftwareSerial.h>

#define SerialSpeed 9600
#define SerialPinRX 8 // Пин для подключения RX провода 
#define SerialPinTX 9 // Пин для подкючения TX провода


SoftwareSerial SoftSerial(SerialPinRX, SerialPinTX); // RX, TX


int ArrData[10]; //максимально 10 штук, включая CRC, можно до 100.

void setup()
{
  Serial.begin(SerialSpeed);
  while (!Serial);

  SoftSerial.begin(SerialSpeed);
  SoftSerial.setTimeout(500);
}

void loop()
{
  int len, i; // переменные для служебных надобностей.
  uint16_t crc = 0; // для подсчета контрольной суммы.
  bool noerr = true; // флаг для контроля ошибок
  if (SoftSerial.available()) // если есть данные в буфере.
  {

    if (noerr = (noerr && SoftSerial.find("<data:"))) // проверка на наличие начального маркера
    {
      Serial.println("Found start tag");
      len = SoftSerial.parseInt();
      Serial.print("Ints to read: "); Serial.println(len / 2);
      noerr = noerr && len < 200; // проверка на количество переменных в буфере, каждое число 2 байта, таким образом можно закодировать числа от -32 768 до 32 767
      noerr = noerr && SoftSerial.find(">"); // Подобные конструкции упрощают написание условий... Используется алгебра логики...
      SoftSerial.readBytes((byte *)ArrData, len); // вот в этой строке массив целых принимается и записывается в массив ArrData
      noerr = noerr && SoftSerial.find("</data>"); // Проверка на получение завершающего маркера

      for (i = 0; i < len - 2; i++) crc = _crc16_update(crc, *((byte *)ArrData + i)); // Считаем контрольную сумму crc по значениям ArrData полученного массива
      if (noerr && crc == ArrData[len / 2 - 1]) // Сверяем ее с контрольной суммой полученной при передаче массива, в последней ячейке массива и если совпадает, то...
      {
        Serial.print ("Received ");
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


      }
      else // это обработка ошибок. приучайтесь сразу закладывать ее в проект
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

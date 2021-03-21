/*
  сделано по http://arduino.ru/forum/programmirovanie/peredat-i-prinyat-integer-po-serial?page=1#comment-318007
*/
#include <stdint.h>
#include <util/crc16.h>
#include <SoftwareSerial.h>
#include <OneWire.h> // Для датчика температуры
#include <DallasTemperature.h> // Для датчика температуры

#define OneWirePin 10 // Пин для подключения датчика температуры

#define WaterPin A0 // Пин для подключения датчика влажности

#define SerialSpeed 9600
#define SerialPinRX 8 // Пин для подключения RX провода 
#define SerialPinTX 9 // Пин для подкючения TX провода

SoftwareSerial SoftSerial(SerialPinRX, SerialPinTX);
OneWire oneWire(OneWirePin);
DallasTemperature sensors(&oneWire);

// Указываем количество передаваемых элементов, в число которых входит и CRC.
int ArrData[3];  //можно выбрать размер от 1 до 99  (числа от -32 768 до 32 767) нумерация элементов начиначется с 0, т.е ArrData[0] до ArrData[2]
// 2 датчика - значит массив значений состоит из 2 элементов и еще crc значения.


// the setup routine runs once when you press reset:
void setup() {
  // initialize serial communication at 9600 bits per second:
  Serial.begin(SerialSpeed);
  while (!Serial);

  SoftSerial.begin(SerialSpeed);

}



// the loop routine runs over and over again forever:
void loop() {

  int len = sizeof(ArrData); // Считаем количество байтов, отданных под массив
  uint16_t  crc = 0; //CRC для массива (можно понимать как контрольная сумма)

  int WaterValue = analogRead(WaterPin);
  sensors.requestTemperatures(); // опрашиваем датчики на OneWire;
  int TemperatureValue = sensors.getTempCByIndex(0); // Выводим значение полученное с первого датчика

  ArrData[0] = WaterValue;
  crc = _crc16_update(crc, WaterValue & 0xff); //..считаем ЦРЦ
  crc = _crc16_update(crc,  WaterValue >> 8);

  ArrData[1] = TemperatureValue;
  crc = _crc16_update(crc, TemperatureValue & 0xff); //..считаем ЦРЦ
  crc = _crc16_update(crc, TemperatureValue >> 8);

  ArrData[len / 2 - 1] = crc;
  len = sizeof(ArrData);

  Serial.print("Array complete CRC is: 0x");
  Serial.println(crc);
  Serial.print("Array length = ");
  Serial.println(len/sizeof(int));
  
  SoftSerial.print("<data:");
  SoftSerial.print(len);
  SoftSerial.print(">");
  SoftSerial.write ((byte *)ArrData,len); //вот в этой строке массив целых собственно и передается
  SoftSerial.print("</data>");

  delay(1000);        // delay in between reads for stability
}

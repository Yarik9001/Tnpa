#include <OneWire.h> // Для датчика температуры
#include <DallasTemperature.h> // Для датчика температуры

#define SerialSpeed 57600

OneWire oneWire(OneWirePin);
DallasTemperature sensors(&oneWire);

int ArrData[4];

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


void setup(){
    Serial.begin(SerialSpeed);
}

void loop(){
    // Получем значения для двигателей
    if ( millis() - LastTimeReceive >= TimeOutReceive ) {
        ReceiveData(); // Получаем Значения для моторов
        LastTimeReceive = millis(); // Время окончания работы
    }
    Serial.println();
}
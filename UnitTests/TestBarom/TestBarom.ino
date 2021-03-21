#define DepthPin A1

  // Получаем и считаем значение датчика глубиры.
  int raw = analogRead(DepthPin);
//  float voltage = (float)raw * 5.0 / 1024.0; //тут не ровно 5 вольт, нужно смотреть как брать правильное напряжение....
//  float  pressure_kpa = (voltage - 0.5) / 4.0 * 2400.0;
//  float pressure_psi = pressure_kpa * 0.14503773773020923 / 14 + 1;
//  float glubina = pressure_psi - 1;
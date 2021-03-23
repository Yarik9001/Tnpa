#define SerialSpeed 57600

#define DepthPin A1 // Пин для подключения датчика давления.
#define TimeOutData 200 // Периодичность опроса датчиков
#define Crutch -14 // Подгоняем значение

unsigned long LastTimeData; // таймер опроса датчиков

void setup(){
    Serial.begin(SerialSpeed);
    pinMode(DepthPin, INPUT);
    Serial.println("start sensor test");
}

void loop(){
  
  if ( millis() - LastTimeData >= TimeOutData ) {
    Serial.print("Depth: ");
    Serial.println( map( analogRead(DepthPin), 0, 1024, 0, 122 ) + Crutch);
    LastTimeData = millis(); // Время окончания работы
  }
}

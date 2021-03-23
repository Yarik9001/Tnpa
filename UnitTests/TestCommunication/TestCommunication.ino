#include <stdint.h>
#include <util/crc16.h>


#define TimeOutReceive 300
#define SerialSpeed 57600

unsigned long LastTimeReceive; //Последнее время срабатывания принятия команд для двигателей

int ArrMotor[10];
int ToMotor[10]; // Значения в моторах будет стремится к этим значениям, туда записываются значения с правильной crc.


void ReceiveData() {
    int len, i; // переменные для служебных надобностей.
    uint16_t crc = 0; // для подсчета контрольной суммы.
    bool noerr = true; // флаг для контроля ошибок
    if (Serial1.available()){ // если есть данные в буфере.
        if (noerr = (noerr && Serial1.find("<data:"))){ // проверка на наличие начального маркера

            //    Serial.println("Found start tag");
            len = Serial1.parseInt();
            //    Serial.print("Ints to read: "); Serial.println(len / 2);

            noerr = noerr && len < 200; // проверка на количество переменных в буфере, каждое число 2 байта, таким образом можно закодировать числа от -32 768 до 32 767
            noerr = noerr && Serial1.find(">"); // Подобные конструкции упрощают написание условий... Используется алгебра логики...
            Serial1.readBytes((byte *)ArrMotor, len); // вот в этой строке массив целых принимается и записывается в массив ArrMotor
            noerr = noerr && Serial1.find("</data>"); // Проверка на получение завершающего маркера

            for (i = 0; i < len - 2; i++) crc = _crc16_update(crc, *((byte *)ArrMotor + i)); // Считаем контрольную сумму crc по значениям ArrMotor полученного массива
            
            for(int i=0; i<10; i++) Serial.print(ArrMotor[0]); Serial.print(" ");
            Serial.println();

            if (noerr && crc == ArrMotor[len / 2 - 1]){  // Сверяем ее с контрольной суммой полученной при передаче массива, в последней ячейке массива и если совпадает, то...
                memcpy(ToMotor, ArrMotor, sizeof(ArrMotor));
            }

            else{ // обработка ошибок
                Serial.print ("Syntax or CRC error!  ");
            }
        }
   }
}

void setup(){
    Serial.begin(SerialSpeed);
    Serial1.begin(SerialSpeed);
    Serial1.setTimeout(50);
}

void loop(){
    // Получем значения для двигателей
    if ( millis() - LastTimeReceive >= TimeOutReceive ) {
        ReceiveData(); // Получаем Значения для моторов
        LastTimeReceive = millis(); // Время окончания работы
    }
    Serial.println();
}

#include <Servo_Hardware_PWM.h> (2,3,7,8,44,45)



#define SerialSpeed 57600

// Ходовые моторы (сервы, управляются по 1 пину)
#define MotorPin1 3
#define MotorPin2 8
#define MotorPin3 2
#define MotorPin4 5

// Дифферентные моторы (коллекторные моторы требуют по 3 пина мощность, вперед, назад,)
#define DiMotorFrontPin 7 // Пин переднего дифферентного мотора 
#define DiMotorBackPin  6 // Пин заднего дифферентного мотора

// Минимальное и максимальное значение ШИМ для моторов (для их калибровки)
#define MinDrive 1000
#define MaxDrive 2000
#define StopDrive 1500

// Число бессколлекторых моторов
#define N 6

Servo motors[N];

void setup() {
    // init serial 
    Serial.begin(SerialSpeed);

    Serial.println("init motors..");

    // matching motors
    motors[0].attach(MotorPin1, MinDrive, MaxDrive);
    motors[1].attach(MotorPin2, MinDrive, MaxDrive);
    motors[2].attach(MotorPin3, MinDrive, MaxDrive);
    motors[3].attach(MotorPin4, MinDrive, MaxDrive);

    // DiMotorsaa
    motors[4].attach(DiMotorFrontPin, MinDrive, MaxDrive);
    motors[5].attach(DiMotorBackPin, MinDrive, MaxDrive);

    Serial.println("calibration motors..");

    for(int i=0; i<N; i++){
        motors[i].writeMicroseconds(MaxDrive);
    }

    delay(2000);

    for(int i=0; i<N; i++){
        motors[i].writeMicroseconds(MinDrive);
    }

    delay(2000);

    for(int i=0; i<N; i++){
        motors[i].writeMicroseconds(StopDrive);
    }

    delay(1000);
    
    Serial.println("calibration done");

    delay(1000);

    Serial.println("start test motors..");

    for(int i=0; i<N; i++){
        motors[i].writeMicroseconds(MaxDrive);
        delay(1500);
        motors[i].writeMicroseconds(StopDrive);
        delay(1500);
        motors[i].writeMicroseconds(MinDrive);
        delay(1500);
        motors[i].writeMicroseconds(StopDrive);
        delay(1500);
    }
}

void loop(){
    
}
    
    


#include <Servo.h>


#define SerialSpeed 57600

#define ServoPinCam 45
#define ServoPinMan 46
#define time_sleep 100

Servo cameraServo;
Servo manipulatorServo;


void setup(){
    Serial.begin(SerialSpeed);

    Serial.println("init servo");
    cameraServo.attach(ServoPinCam);
    manipulatorServo.attach(ServoPinMan);
    Serial.println("servo init done");

    cameraServo.write(0);
    manipulatorServo.write(0);
    delay(500);
}

void loop(){
    for(int i=0; i<180; i++){
        cameraServo.write(i);
        delay(time_sleep);
    }
    for(int i=180; i>0; i--){
        cameraServo.write(i);
        delay(time_sleep);
    }

    delay(2000);

    for(int i=0; i<180; i++){
        manipulatorServo.write(i);
        delay(time_sleep);
    }
    for(int i=180; i>0; i--){
        manipulatorServo.write(i);
        delay(time_sleep);
    }

    delay(2000);
}

#include "mbed.h"

// 1. PIN DEFINITIONS
DigitalOut trig(D8);
InterruptIn echo(D7);

DigitalOut in1(D4);
DigitalOut in2(D5);
PwmOut ena(D3);

DigitalOut in3(D2);
DigitalOut in4(D6);
PwmOut enb(D9);

Timer timer;
volatile int pulse_duration = 0;
volatile bool new_reading = false;

const float TARGET_DISTANCE = 20.0;
const float TOLERANCE = 5.0;
const float SPEED = 1.0f; // Boosted to 100% to overcome static friction of the gearboxes

// 2. INTERRUPT SERVICE ROUTINES (ISRs)
void start_timer() {
    timer.reset();
    timer.start();
}

void stop_timer() {
    timer.stop();
    // Capture the raw microsecond count safely inside the interrupt (Mbed OS 5 compatible)
    pulse_duration = timer.read_us();
    new_reading = true;
}

// 3. MOTOR CONTROL FUNCTIONS
void go_forward() {
    in1 = 1; in2 = 0;
    in3 = 1; in4 = 0;
    ena.write(SPEED);
    enb.write(SPEED);
}

void stop_car() {
    in1 = 0; in2 = 0;
    in3 = 0; in4 = 0;
    ena.write(0.0f);
    enb.write(0.0f);
}

// 4. MAIN PROGRAM LOOP
int main() {
    echo.rise(&start_timer);
    echo.fall(&stop_timer);

    ena.period(0.001f);
    enb.period(0.001f);

    float distance = 0.0;

    // Startup message to verify the board is alive
    printf("\n--- System Booted. Starting Distance Tracking ---\n");

    while (true) {
        // Trigger Sensor
        trig = 1;
        wait_us(10);
        trig = 0;

        // 50ms delay (Mbed OS 5 compatible)
        wait_us(50000);

        // Calculate and Print Distance
        if (new_reading) {
            distance = pulse_duration / 58.0f; // Calculate distance in main loop
            new_reading = false;
            printf("Target Distance: %.2f cm\n", distance);
        } else {
            printf("WARNING: Sensor Timeout or Wiring Issue. Distance = 0\n");
            distance = 0; // Force stop if sensor is disconnected
        }

        // Logic Check
        // Ignore wild noise readings (> 400cm is usually an error for HC-SR04)
        if (distance > (TARGET_DISTANCE + TOLERANCE) && distance < 400.0) {
            go_forward();
        } else {
            stop_car();
        }
    }
}
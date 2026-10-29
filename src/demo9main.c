// ****************************************************************
// * Authors: T. LUM and R. MARTIN
// * CPEG222 Demo9 Program, 10/28/2026
// * NucleoF446ZE CMSIS STM32F4xx example Program
// * Reads the BMI270 IMU and displays the accelerometer and gyroscope 
// * data on the SSH1106 OLED display.
// ****************************************************************
#include "stm32f4xx.h"
#include "SysClock180MHz.h"
#include "UART3.h"
#include "I2C.h"
#include "OLED.h"
#include "BMI270.h"
#include "NeoPixel.h"
#include <stdio.h> // for snprintf
#include <math.h> // for abs

volatile float accel_x = 0;
volatile float accel_y = 0;
volatile float accel_z = 0;
volatile float gyro_x = 0;
volatile float gyro_y = 0;
volatile float gyro_z = 0;
uint8_t g; // GRB order for standard WS2812B NeoPixels
uint8_t r;
uint8_t b;
char buffer[40];

// Map |acceleration| in G (0..2) to 0..255, clamped
static uint8_t scale_accel(float a) {
    float v = fabsf(a) / 2.0f * 255.0f;
    if (v > 255.0f) return 255;
    return (uint8_t)v;
}

int main(void) {
    // change the clock from 16MHz to 180MHz
    SystemClock_Config(); // Call the function to set system clock to 180MHz
    UART3_Init();
    NeoPixel_Init_PB11(); // Initialize NeoPixel on PB11
    uart3_send_string("\nCPEG222 BMI270 IMU Demo9 Program - 10/28/2026\n");
    I2C1_Init(); // Initialize I2C for the BME280 before using the Bosch driver 
    OLED_Init(); // Initialize the OLED display
    OLED_Print(0, 0, "CPEG222 - 10/28/2026");
    OLED_Print(1, 0, "BMI270 IMU Data ");
    OLED_Print(2, 0, "--------------------------------");
    OLED_Print(3, 0, " Acc(G's) Gyro(deg/s) ");
    BMI270_Init();
    NeoPixel_SetColor(0, 255, 0, 0); // Set NeoPixel color to red
    NeoPixel_SetColor(1, 255, 0, 0); // Set NeoPixel color to red
    NeoPixel_SetColor(2, 255, 0, 0); // Set NeoPixel color to red
    NeoPixel_SetColor(3, 255, 0, 0); // Set NeoPixel color to red
    NeoPixel_Show_PB11(); // Update NeoPixel to show the color

    while (1) {
        // Read accelerometer and gyroscope data from BMI270
        BMI270_Read_All_Data();// Read BMI270 sensor data
        //BMI270_ReadAccelGyro(&accel_x, &accel_y, &accel_z, &gyro_x, &gyro_y, &gyro_z);
        snprintf(buffer, sizeof(buffer), "  X:% 4.3f, % 4.3f",accel_x, gyro_x);
        OLED_Print(4, 0, buffer);
        snprintf(buffer, sizeof(buffer), "  Y:% 4.3f, % 4.3f", accel_y, gyro_y);
        OLED_Print(5, 0, buffer);
        snprintf(buffer, sizeof(buffer), "  Z:% 4.3f, % 4.3f", accel_z, gyro_z);
        OLED_Print(6, 0, buffer);
        // Map accleration to NeoPixel colors
        // Each axis drives one channel: 0g -> 0, +/-2g -> 255
        r = scale_accel(accel_x);
        g = scale_accel(accel_y);
        b = scale_accel(accel_z);
        // Update NeoPixel colors based on accelerometer data
        NeoPixel_SetColor(0, r, g, b);
        NeoPixel_SetColor(1, r, g, b);
        NeoPixel_SetColor(2, r, g, b);
        NeoPixel_SetColor(3, r, g, b);
        NeoPixel_Show_PB11(); // Update NeoPixel to show the new colors
        // Delay for a short period
        for (volatile int i = 0; i < 1000000; i++);
    }
    return 0;
}

#include <Arduino.h>
#include <math.h>

#define SENSOR_PIN 1  // GPIO1 for analog input (ADC1_CH0)

// Regression formula: convert sensor voltage to alcohol concentration in ppm (air)
float ppm_from_voltage(float voltage) {
    return exp((voltage + 1.53338f) / 0.95387f) - 9.84144f;
}

// Convert ppm (air) to mg/L based on MQ-3 datasheet linear mapping
float mgL_from_ppm(float ppm) {
    if (ppm < 25) ppm = 0;      // Clamp lower bound
    // if (ppm > 500) ppm = 500;    // Clamp upper bound
    // method 1. Linear mapping: 25-500 ppm -> 0.05-10 mg/L
    // return (ppm - 25.0f) * (10.0f - 0.05f) / (500.0f - 25.0f) + 0.05f;
    // Method 2. 0.4mg/L ( approximately 200ppm ) <-based on MQ-3 datasheet
    return (ppm / 500.0f);
}

void setup() {
    Serial.begin(115200);
}

void loop() {
    float sensorValue = 0.0;
    float sensor_volt = 0.0;
    float ppm = 0.0;
    float mg_L = 0.0;

    // Average 100 ADC readings for stability
    for (int i = 0; i < 100; i++) {
        sensorValue += analogRead(SENSOR_PIN);
        delay(5);
    }
    sensorValue /= 100.0;

    // Calculate sensor voltage (ADC to V conversion, with voltage divider compensation)
    sensor_volt = (sensorValue / 4095.0) * 3.3 * 1.50 + 0.05;

    // Convert voltage to alcohol concentration in ppm
    ppm = ppm_from_voltage(sensor_volt);

    // Convert ppm to mg/L using linear mapping (no clamps)
    mg_L = mgL_from_ppm(ppm);

    // Print results
    Serial.print("ADC: "); Serial.print(sensorValue, 0);
    Serial.print(" | V: "); Serial.print(sensor_volt, 2); Serial.print(" V");
    Serial.print(" | Alcohol: "); Serial.print(ppm, 2); Serial.print(" ppm");
    Serial.print(" | mg/L: "); Serial.println(mg_L, 3);
    Serial.println("---");

    delay(2000);
}

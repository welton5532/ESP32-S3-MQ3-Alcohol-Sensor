# Project: MQ-3 Alcohol Sensor with ESP32-S3

<img width="1242" height="337" alt="image" src="https://github.com/user-attachments/assets/68d8a9e1-8138-4013-b2ad-08e3b36e44ff" />


This project implements a high-precision alcohol concentration detector using the **MQ-3 Alcohol Gas Sensor** and the **Espressif ESP32-S3**.

Unlike standard tutorials, this project uses a **custom calibration regression model** derived from the sensor datasheet and implements a specific **hardware impedance matching circuit** to safely interface the 5V sensor with the 3.3V ESP32 while strictly maintaining the required 4.7kΩ load resistance.

  * **Hardware:** Espressif ESP32-S3-DevKitC-1U-N8R8 (8MB Flash/8MB PSRAM)
  * **Sensor:** MQ-3 Alcohol Gas Sensor
  * **Output Units:** ppm (Parts Per Million) and mg/L

<img width="600" height="600" alt="image" src="https://github.com/user-attachments/assets/bec52f05-8b97-4bce-8c53-d855934ba9ff" />

<img width="1024" height="682" alt="image" src="https://github.com/user-attachments/assets/5b4a83b1-a566-48ea-9203-aaa9dce450f2" />

<img width="1024" height="683" alt="image" src="https://github.com/user-attachments/assets/3d2d5a82-90eb-419c-9322-a01f0723bdea" />

<img width="565" height="302" alt="image" src="https://github.com/user-attachments/assets/0ad59e01-34e9-4915-ae01-0889d0cb010c" />
In order to match MQ-3 sensitivity curve, R2 should change to 4.7k Ohm (+ 75 OHM [= 4775 Ohm] if using 300k Ohm Voltage divider.)

-----


## 2\. Hardware Architecture & Impedance Matching

The MQ-3 sensor operates at 5V, but the ESP32-S3 GPIO is 3.3V tolerant. Furthermore, the sensor's calibration curve relies on a specific Load Resistance ($R_L$). A custom circuit was designed to solve both voltage leveling and impedance matching simultaneously.

### A. Voltage Divider (Level Shifting)

To protect the ESP32 ADC, the 0-5V analog output is scaled down using a 3-resistor divider network:

  * **R1 (Series):** $100k\Omega$
  * **R2 (Ground):** $200k\Omega$ (Implemented as two $100k\Omega$ resistors in series)

$$V_{GPIO} = V_{MQ3} \times \frac{R2}{R1 + R2} = V_{MQ3} \times \frac{200k}{300k} \approx 0.66 \times V_{MQ3}$$

*Result: A maximum 5.0V signal is stepped down to \~3.33V.*

### B. Impedance Compensation (Critical)

The MQ-3 sensitivity curve is valid **only** when the load resistance ($R_L$) is **4.7kΩ**.
However, connecting the voltage divider ($300k\Omega$ total) places it in **parallel** with the on-board $R_L$, lowering the effective resistance seen by the sensor.

To fix this, the module's on-board resistor was replaced to ensure the **Total Equivalent Resistance** ($R_{EQ}$) equals 4.7kΩ.

  * **Target:** $R_{EQ} = 4.7k\Omega$
  * **Divider Load:** $R_{DIV} = 300k\Omega$
  * **Correction Calculation:**
    $$R_{board} = \frac{R_{EQ} \times R_{DIV}}{R_{DIV} - R_{EQ}} = \frac{4700 \times 300000}{295300} \approx 4775\Omega$$

**Implementation:** The standard SMD resistor on the MQ-3 module was replaced with a series combination of **4.7kΩ + 75Ω**.

### C. Wiring Diagram

```text
       MQ-3 Sensor Module
      +------------------+
      |                  |
      |       [VCC] -----+-----> 5V Source
      |                  |
      |       [GND] -----+-----> GND
      |                  |
      |       [A0]       |
      +--------+---------+
               |
               | <--- Analog Signal (0-5V)
               |
      (Note: On-board RL modified to 4.7kΩ + 75Ω)
               |
               +-----------------------+
                                       |
                                    [100kΩ]  <-- R_Divider_Top
                                       |
ESP32-S3 GPIO 1 <----------------------+
(Max 3.33V)                            |
                                    [100kΩ]  \
                                       |      } R_Divider_Bottom (Total 200kΩ)
                                    [100kΩ]  /
                                       |
                                      GND
```

<img width="1075" height="767" alt="image" src="https://github.com/user-attachments/assets/1adf7f7b-b48b-4716-9905-c7588c01a989" />


-----

## 3\. Calibration Methodology

### Sensitivity Curve Analysis

1.  **Curve Extension:** The datasheet curve (Fig 5) typically stops at 50ppm. The curve was manually extended to finding the baseline voltage for clean air (0 ppm).
      * *Baseline:* $V_{RL} \approx 0.8V$ (at 0ppm with 4.7kΩ load).
2.  **Digitization:** **WebPlotDigitizer** was used to extract $(x,y)$ coordinates from the logarithmic sensitivity plot.
3.  **Regression:** A custom exponential regression formula was derived to convert Voltage ($V_{RL}$) to Concentration (ppm).

$$PPM = e^{\left(\frac{V_{RL} + 1.53338}{0.95387}\right)} - 9.84144$$

<img width="490" height="558" alt="image" src="https://github.com/user-attachments/assets/a4f9b88e-bedc-4f11-bfe5-634a5eaa9edf" />

Fig5.Sensitity Curve (https://www.winsen-sensor.com/product/mq-3b.html?campaign)

<img width="400" height="389" alt="image" src="https://github.com/user-attachments/assets/c7eddc5e-7d66-4fb1-93b0-c62dd769651f" />

Fig5.Sensitity Curve (CHT)


<img width="892" height="892" alt="image" src="https://github.com/user-attachments/assets/33b6e736-a027-460a-9a6a-3252aa075b55" />

manually extended curve

### Unit Conversion

Based on the MQ-3 datasheet, the linear mapping for unit conversion is:
$$1 \text{ mg/L} = 500 \text{ ppm}$$

<img width="613" height="230" alt="image" src="https://github.com/user-attachments/assets/6cacc3f3-2e19-4a74-9d59-37f54c60223e" />

-----

## 4\. Software Configuration

### PlatformIO Settings (`platformio.ini`)

Configured for **ESP32-S3-DevKitC-1U-N8R8** (8MB Flash / 8MB OPI PSRAM).

```ini
; -- Environment Definition --
[env:esp32-s3-devkitc-1]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino
monitor_speed = 115200

; -- MEMORY CONFIGURATION (Critical for N8R8) --
; The ESP32-S3 communicates with memory in different "modes".
; 'qio' (Quad I/O) uses 4 wires. 'opi' (Octal I/O) uses 8 wires.
; N8 (Flash) usually uses qio. R8 (PSRAM) MUST use opi.
board_build.arduino.memory_type = qio_opi

; -- FLASH SPEED --
; Set Flash frequency to 80MHz for best performance
board_build.f_flash = 80000000L
board_build.flash_mode = qio

; -- PARTITION SCHEME --
; Tells the ESP32 how to slice up the 8MB Flash chip.
; default_8MB.csv gives you ~3.3MB for Code and ~1.5MB for Files (LittleFS).
board_build.partitions = default_8MB.csv
board_upload.flash_size = 8MB

; -- COMPILER FLAGS --
; -DBOARD_HAS_PSRAM: Defines the "macro" that allows code to see PSRAM.
; -mfix...: A standard fix for S3 chips to prevent memory glitches.
build_flags = 
    -DBOARD_HAS_PSRAM
    -mfix-esp32-psram-cache-issue

```

-----

## 5\. Firmware Implementation

The code reads the ADC, restores the voltage (reversing the divider effect), applies the regression formula, and outputs the result.

```cpp
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
```

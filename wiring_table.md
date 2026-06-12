# Wiring Table for SmartDrip

| Component          | ESP32 Pin | Description                              |
|--------------------|-----------|------------------------------------------|
| Drip IR Sensor     | GPIO27    | Interrupt input – detects each drop (active LOW) |
| Low‑Level IR Sensor| GPIO26    | Digital input – goes LOW when fluid is low |
| Buzzer             | GPIO25    | Active HIGH output for alarm              |
| Status LED         | GPIO2     | Active HIGH – indicates alarm state      |
| (Optional) OLED    | SDA=GPIO21<br>SDL=GPIO22 | I²C interface for status display |

**Connection notes**
- Connect the IR sensors' VCC to 3.3 V and GND to GND.
- Use pull‑up resistors (10 kΩ) on sensor outputs or enable `INPUT_PULLUP` in code.
- Buzzer and LED connect to the indicated pins with a current‑limiting resistor (≈220 Ω for LED).
- For the optional OLED, connect SDA to GPIO21 and SCL to GPIO22, with 3.3 V and GND.

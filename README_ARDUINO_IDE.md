# Arduino IDE Quick Start - Arduino AI Robot

## **BEFORE YOU UPLOAD:**

### **Install Required Libraries (One-Time Setup)**

1. Open **Arduino IDE 2.x**
2. Go to **Sketch** → **Include Library** → **Manage Libraries**
3. Install these 2 libraries:
   - **Adafruit MPU6050** (by Adafruit)
   - **Adafruit Sensor** (by Adafruit)

4. Go to **Tools** → **Boards** → **Boards Manager**
5. Search and install: **Arduino Mbed OS Portenta Boards** (for Arduino AI)

### **Select Board Settings**

- **Tools** → **Board** → Select your Arduino AI variant (e.g., Arduino Portenta H7)
- **Tools** → **Port** → Select your USB COM port

### **Compile & Upload**

1. Open `arduino_microros_robot.ino`
2. Click **Verify** (✓) to check syntax
3. Click **Upload** (→) to flash

## **WHAT THE CODE DOES:**

✅ Initializes all 4 encoders (FR, BR, FL, BL)
✅ Initializes MPU6050 IMU sensor via I2C
✅ Initializes 2 ultrasonic sensors (top/bottom)
✅ Runs motor test on startup (forward, backward, turn left/right)
✅ Continuously reads sensor data at 20 Hz
✅ Outputs CSV data over Serial at 115200 baud

## **SERIAL OUTPUT FORMAT:**

```
dist_top, dist_bottom, wall_tilt, countFR, countBR, countFL, countBL, robot_angle
```

Example:
```
0.50,0.48,0.02,125,124,120,119,-2.34
0.51,0.47,0.04,130,129,125,124,-1.89
```

## **MONITOR OUTPUT:**

1. **Tools** → **Serial Monitor** (or Ctrl+Shift+M)
2. Set baud rate to **115200**
3. Watch real-time sensor data stream

## **PIN CONFIGURATION:**

```
Motor A (Left):   EN=10, IN1=32, IN2=34
Motor B (Right):  EN=12, IN1=36, IN2=45

Encoders:
  Front-Right:    A=28, B=22
  Back-Right:     A=26, B=24
  Front-Left:     A=50, B=46
  Back-Left:      A=51, B=40

Ultrasonic:
  Top:    Trigger=8, Echo=9
  Bottom: Trigger=6, Echo=7

IMU (MPU6050): I2C (SDA/SCL on standard Arduino AI pins)
```

## **TROUBLESHOOTING:**

| Error | Solution |
|-------|----------|
| "Adafruit_MPU6050.h: No such file" | Install Adafruit MPU6050 library |
| "Arduino Portenta not found" | Install "Arduino Mbed OS Portenta Boards" |
| Serial not working | Check COM port in Tools → Port |
| IMU not initializing | Check I2C connections (SDA/SCL) |
| Encoders not counting | Verify INPUT_PULLUP pins are soldered |

## **NEXT STEPS:**

Once verified working, you can:
1. Add ROS 2 support by installing micro_ros_arduino library
2. Send motor commands via Serial protocol
3. Log data to SD card
4. Integrate with Raspberry Pi 5 via MQTT or Serial protocol

---

**This is a HARDWARE TEST version without ROS 2 dependency.**
No external micro-ROS setup required - just USB serial communication!

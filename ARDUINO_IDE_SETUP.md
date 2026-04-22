# Arduino IDE Setup Guide for Arduino AI Robot

## **Step 1: Install Arduino IDE**
- Download from: https://www.arduino.cc/en/software
- Choose Arduino IDE 2.x (recommended)

## **Step 2: Add Arduino AI Board Support**
1. Open Arduino IDE → **File** → **Preferences**
2. Under "Additional boards manager URLs", add:
   ```
   https://downloads.arduino.cc/packages/mbed-os-boards-pr-index.json
   ```
3. Go to **Tools** → **Board** → **Boards Manager**
4. Search for "Arduino Mbed OS Portenta Boards"
5. Click **Install**
6. Select **Tools** → **Board** → **Arduino Mbed OS Portenta Boards** → **Arduino Portenta H7** (or your Arduino AI variant)

## **Step 3: Install Required Libraries**
1. Go to **Sketch** → **Include Library** → **Manage Libraries**
2. Search and install:
   - `micro_ros_arduino` (by Micro Robotics Operating System)
   - `Adafruit MPU6050` (by Adafruit)
   - `Adafruit Sensor` (by Adafruit)

## **Step 4: Select Correct Programmer**
1. **Tools** → **Programmer** → Select appropriate option for Arduino AI
2. **Tools** → **Port** → Select COM port for Arduino AI

## **Step 5: Setup Micro-ROS Agent (Raspberry Pi Side)**
The code requires a micro-ROS agent running on Raspberry Pi 5:

```bash
# On Raspberry Pi 5
docker run -it --rm --net=host microros/micro-ros-agent:latest serial --dev /dev/ttyUSB0 -b 115200
```

Or use native installation:
```bash
source /opt/ros/humble/setup.bash
ros2 run micro_ros_agent micro_ros_agent serial --dev /dev/ttyUSB0 -b 115200
```

## **Step 6: Upload Code**
1. Open `arduino_microros_robot.ino`
2. Click **Verify** to check syntax
3. Click **Upload** to flash to Arduino AI

## **Step 7: Monitor Serial Output**
- Open **Tools** → **Serial Monitor**
- Set baud rate to **115200**
- You should see CSV data: `distance_top,distance_bottom,wall_tilt,encoders,robot_angle`

## **Troubleshooting**

### Compilation Error: "micro_ros_arduino.h not found"
- Ensure library is installed (Manage Libraries)
- Restart Arduino IDE after installation

### Board Not Detected
- Check USB cable connection
- Install CH340/FTDI drivers if needed
- Try different USB port

### Micro-ROS Connection Fails
- Verify agent is running on Raspberry Pi
- Check baud rate matches (115200)
- Use `ros2 topic list` to verify topics are created

### MPU6050 Not Initializing
- Check I2C connections (SDA/SCL)
- Verify address: 0x68 (default)
- Add pull-up resistors if needed

## **Testing Commands (on Raspberry Pi)**

```bash
# List available topics
ros2 topic list

# View sensor data
ros2 topic echo /sensor/distance_top
ros2 topic echo /sensor/imu

# Send motor command
ros2 topic pub /cmd_vel geometry_msgs/msg/Twist "{linear: {x: 0.2}, angular: {z: 0.0}}"
```

## **Hardware Pinout Reference**
See pin definitions at top of `arduino_microros_robot.ino`:
- Motor A: EN=10, IN1=32, IN2=34
- Motor B: EN=12, IN1=36, IN2=45
- Encoders: FR(28,22), BR(26,24), FL(50,46), BL(51,40)
- Ultrasonic: TOP(8,9), BOTTOM(6,7)
- I2C: SDA/SCL (standard pins)

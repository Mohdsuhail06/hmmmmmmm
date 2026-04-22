/*
 * TRACKED ROBOT - MICRO-ROS INTEGRATION (Arduino AI)
 * Arduino AI communicates with Raspberry Pi 5 via ROS 2
 * 
 * Hardware:
 * - 2x DC Motors (L298N driver) for 4-wheel drive
 * - 4x Wheel encoders (quadrature encoders) - FR, BR, FL, BL
 * - 2x Ultrasonic sensors (distance measurement)
 * - 1x MPU6050 IMU (accelerometer/gyroscope)
 * - RPLidar A1 (connected to Pi, not Arduino)
 * - Camera (connected to Pi via USB)
 * 
 * Communication: Arduino AI → USB Serial → Raspberry Pi 5 (micro-ROS agent)
 */

#include <micro_ros_arduino.h>
#include <stdio.h>
#include <unistd.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

// ROS 2 message types
#include <std_msgs/msg/string.h>
#include <std_msgs/msg/int32.h>
#include <sensor_msgs/msg/range.h>
#include <sensor_msgs/msg/imu.h>
#include <geometry_msgs/msg/twist.h>

// MPU6050 sensor
Adafruit_MPU6050 mpu;

// ============ HARDWARE PIN DEFINITIONS (ARDUINO AI) ============

// Motor Control Pins (L298N Driver)
#define MOTOR_A_EN 10      // PWM enable for Motor A
#define MOTOR_A_IN1 32     // Motor A direction 1
#define MOTOR_A_IN2 34     // Motor A direction 2
#define MOTOR_B_IN1 36     // Motor B direction 1
#define MOTOR_B_IN2 45     // Motor B direction 2
#define MOTOR_B_EN 12      // PWM enable for Motor B

// Ultrasonic Sensor Pins (Distance Measurement)
#define TRIG_TOP 8         // Top sensor trigger
#define ECHO_TOP 9         // Top sensor echo
#define TRIG_BOTTOM 6      // Bottom sensor trigger
#define ECHO_BOTTOM 7      // Bottom sensor echo

// Wheel Encoder Pins (4 Encoders - FR, BR, FL, BL)
#define ENC_FR_A 28        // Front-Right encoder A
#define ENC_FR_B 22        // Front-Right encoder B
#define ENC_BR_A 26        // Back-Right encoder A
#define ENC_BR_B 24        // Back-Right encoder B
#define ENC_FL_A 50        // Front-Left encoder A
#define ENC_FL_B 46        // Front-Left encoder B
#define ENC_BL_A 51        // Back-Left encoder A
#define ENC_BL_B 40        // Back-Left encoder B

// ============ ROS 2 OBJECTS ============

rcl_publisher_t distance_top_pub;
rcl_publisher_t distance_bottom_pub;
rcl_publisher_t encoder_fr_pub;
rcl_publisher_t encoder_br_pub;
rcl_publisher_t encoder_fl_pub;
rcl_publisher_t encoder_bl_pub;
rcl_publisher_t imu_pub;
rcl_subscription_t cmd_vel_sub;

sensor_msgs__msg__Range distance_top_msg;
sensor_msgs__msg__Range distance_bottom_msg;
std_msgs__msg__Int32 encoder_fr_msg;
std_msgs__msg__Int32 encoder_br_msg;
std_msgs__msg__Int32 encoder_fl_msg;
std_msgs__msg__Int32 encoder_bl_msg;
sensor_msgs__msg__Imu imu_msg;
geometry_msgs__msg__Twist cmd_vel_msg;

rclc_executor_t executor;
rclc_support_t support;
rcl_node_t node;
rcl_allocator_t allocator;

// ============ ENCODER VARIABLES (VOLATILE FOR INTERRUPTS) ============

volatile long countFR = 0;   // Front-Right
volatile long countBR = 0;   // Back-Right
volatile long countFL = 0;   // Front-Left
volatile long countBL = 0;   // Back-Left

volatile int lastFR_A = 0;
volatile int lastBR_A = 0;
volatile int lastFL_A = 0;
volatile int lastBL_A = 0;

// ============ MOVEMENT VARIABLES ============

float target_linear_velocity = 0.0;   // m/s
float target_angular_velocity = 0.0;  // rad/s
unsigned long last_cmd_time = 0;
#define CMD_TIMEOUT 1000  // 1 second timeout
unsigned long lastPrintTime = 0;

// ============ SETUP ============

void setup() {
  Serial.begin(115200);
  set_microros_transports();

  delay(2000);

  // Initialize GPIO pins
  pinMode(MOTOR_A_EN, OUTPUT);
  pinMode(MOTOR_A_IN1, OUTPUT);
  pinMode(MOTOR_A_IN2, OUTPUT);
  pinMode(MOTOR_B_EN, OUTPUT);
  pinMode(MOTOR_B_IN1, OUTPUT);
  pinMode(MOTOR_B_IN2, OUTPUT);

  // Ultrasonic sensor pins
  pinMode(TRIG_TOP, OUTPUT);
  pinMode(ECHO_TOP, INPUT);
  pinMode(TRIG_BOTTOM, OUTPUT);
  pinMode(ECHO_BOTTOM, INPUT);

  // Encoder pins with pull-up
  pinMode(ENC_FR_A, INPUT_PULLUP);
  pinMode(ENC_FR_B, INPUT_PULLUP);
  pinMode(ENC_BR_A, INPUT_PULLUP);
  pinMode(ENC_BR_B, INPUT_PULLUP);
  pinMode(ENC_FL_A, INPUT_PULLUP);
  pinMode(ENC_FL_B, INPUT_PULLUP);
  pinMode(ENC_BL_A, INPUT_PULLUP);
  pinMode(ENC_BL_B, INPUT_PULLUP);

  // Initialize encoder last states
  lastFR_A = digitalRead(ENC_FR_A);
  lastBR_A = digitalRead(ENC_BR_A);
  lastFL_A = digitalRead(ENC_FL_A);
  lastBL_A = digitalRead(ENC_BL_A);

  // Initialize MPU6050 IMU
  if (!mpu.begin()) {
    Serial.println("ERROR: MPU6050 not found!");
  } else {
    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
    Serial.println("MPU6050 initialized!");
  }

  // Initialize ROS 2
  allocator = rcl_get_default_allocator();

  // Create support
  rclc_support_init(&support, 0, NULL, &allocator);

  // Create node
  rclc_node_init_default(&node, "robot_node", "", &support);

  // Create publishers (sensor data → ROS 2)
  rclc_publisher_init_default(
    &distance_top_pub,
    &node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(sensor_msgs, msg, Range),
    "sensor/distance_top");

  rclc_publisher_init_default(
    &distance_bottom_pub,
    &node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(sensor_msgs, msg, Range),
    "sensor/distance_bottom");

  rclc_publisher_init_default(
    &encoder_fr_pub,
    &node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
    "encoder/front_right");

  rclc_publisher_init_default(
    &encoder_br_pub,
    &node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
    "encoder/back_right");

  rclc_publisher_init_default(
    &encoder_fl_pub,
    &node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
    "encoder/front_left");

  rclc_publisher_init_default(
    &encoder_bl_pub,
    &node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
    "encoder/back_left");

  rclc_publisher_init_default(
    &imu_pub,
    &node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(sensor_msgs, msg, Imu),
    "sensor/imu");

  // Create subscription (ROS 2 → motor commands)
  rclc_subscription_init_default(
    &cmd_vel_sub,
    &node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(geometry_msgs, msg, Twist),
    "cmd_vel");

  // Create executor
  rclc_executor_init(&executor, &support.context, 1, &allocator);
  rclc_executor_add_subscription(&executor, &cmd_vel_sub, &cmd_vel_msg, &cmd_vel_callback, ON_NEW_DATA);

  // Initialize message types
  initialize_sensor_messages();

  Serial.println("Arduino AI ROS 2 Node Initialized!");
  delay(1000);
}

// ============ MAIN LOOP ============

void loop() {
  // Spin executor to receive ROS 2 messages (with timeout)
  rclc_executor_spin_some(&executor, RCL_MS_TO_NS(50));

  // Check command timeout (safety stop)
  if (millis() - last_cmd_time > CMD_TIMEOUT) {
    target_linear_velocity = 0.0;
    target_angular_velocity = 0.0;
  }

  // Read encoders (polling method)
  read_encoders();

  // Execute motor commands
  execute_motion();

  // Read and publish sensor data
  publish_sensor_data();

  delay(50);  // 20 Hz main loop
}

// ============ ENCODER READING (POLLING METHOD) ============

void read_encoders() {
  // Front-Right encoder
  int currFR_A = digitalRead(ENC_FR_A);
  if (currFR_A != lastFR_A) {
    if (digitalRead(ENC_FR_B) != currFR_A) {
      countFR++;
    } else {
      countFR--;
    }
    lastFR_A = currFR_A;
  }

  // Back-Right encoder
  int currBR_A = digitalRead(ENC_BR_A);
  if (currBR_A != lastBR_A) {
    if (digitalRead(ENC_BR_B) != currBR_A) {
      countBR++;
    } else {
      countBR--;
    }
    lastBR_A = currBR_A;
  }

  // Front-Left encoder
  int currFL_A = digitalRead(ENC_FL_A);
  if (currFL_A != lastFL_A) {
    if (digitalRead(ENC_FL_B) != currFL_A) {
      countFL++;
    } else {
      countFL--;
    }
    lastFL_A = currFL_A;
  }

  // Back-Left encoder
  int currBL_A = digitalRead(ENC_BL_A);
  if (currBL_A != lastBL_A) {
    if (digitalRead(ENC_BL_B) != currBL_A) {
      countBL++;
    } else {
      countBL--;
    }
    lastBL_A = currBL_A;
  }
}

// ============ ROS 2 CALLBACKS ============

void cmd_vel_callback(const void *msgin) {
  const geometry_msgs__msg__Twist *msg = (const geometry_msgs__msg__Twist *)msgin;

  target_linear_velocity = msg->linear.x;      // m/s
  target_angular_velocity = msg->angular.z;    // rad/s

  last_cmd_time = millis();

  Serial.print("Received cmd_vel - linear: ");
  Serial.print(target_linear_velocity);
  Serial.print(", angular: ");
  Serial.println(target_angular_velocity);
}

// ============ MOTOR CONTROL FUNCTIONS ============

void execute_motion() {
  /*
   * Convert ROS 2 cmd_vel to motor speeds
   * 
   * For 4-wheel differential drive:
   * All wheels can move independently
   * Linear velocity affects all wheels equally
   * Angular velocity affects right/left wheels differentially
   */

  const float WHEEL_BASE = 0.20;  // meters (distance between left and right wheels)
  const float MAX_SPEED = 0.5;    // m/s

  // Calculate individual wheel speeds
  float left_speed = target_linear_velocity - (target_angular_velocity * WHEEL_BASE / 2.0);
  float right_speed = target_linear_velocity + (target_angular_velocity * WHEEL_BASE / 2.0);

  // Clamp to max speed
  left_speed = constrain(left_speed, -MAX_SPEED, MAX_SPEED);
  right_speed = constrain(right_speed, -MAX_SPEED, MAX_SPEED);

  // Convert m/s to PWM (0-255)
  int left_pwm = speed_to_pwm(left_speed);
  int right_pwm = speed_to_pwm(right_speed);

  // Set motor speeds using the Arduino AI pins
  set_motor(MOTOR_A_IN1, MOTOR_A_IN2, MOTOR_A_EN, left_pwm);   // Motor A (Left side)
  set_motor(MOTOR_B_IN1, MOTOR_B_IN2, MOTOR_B_EN, right_pwm);  // Motor B (Right side)
}

int speed_to_pwm(float speed) {
  const float MAX_SPEED = 0.5;  // m/s
  
  if (speed == 0) return 0;
  
  int pwm = (int)((abs(speed) / MAX_SPEED) * 255);
  return constrain(pwm, 0, 255);
}

void set_motor(int pin_in1, int pin_in2, int pin_en, int pwm_value) {
  if (pwm_value > 0) {
    // Forward
    digitalWrite(pin_in1, HIGH);
    digitalWrite(pin_in2, LOW);
    analogWrite(pin_en, abs(pwm_value));
  } else if (pwm_value < 0) {
    // Backward
    digitalWrite(pin_in1, LOW);
    digitalWrite(pin_in2, HIGH);
    analogWrite(pin_en, abs(pwm_value));
  } else {
    // Stop
    digitalWrite(pin_in1, LOW);
    digitalWrite(pin_in2, LOW);
    analogWrite(pin_en, 0);
  }
}

// ============ SENSOR READING FUNCTIONS ============

float read_ultrasonic(int trigger_pin, int echo_pin) {
  /*
   * Send ultrasonic pulse and measure distance
   * Returns distance in meters
   */

  digitalWrite(trigger_pin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigger_pin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigger_pin, LOW);

  // Measure echo duration (max 30ms = 5m range)
  long duration = pulseIn(echo_pin, HIGH, 30000);

  // Handle timeout
  if (duration == 0) {
    return 999.0;  // Invalid reading
  }

  // Convert to distance in meters
  // Speed of sound = 343 m/s
  // Distance = (duration * speed) / 2
  float distance = (duration / 1000000.0) * 343.0 / 2.0;

  return distance;
}

void publish_sensor_data() {
  static unsigned long last_publish = 0;

  // Publish at 5 Hz (every 200ms)
  if (millis() - last_publish < 200) {
    return;
  }
  last_publish = millis();

  // Read ultrasonic sensors
  float dist_top = read_ultrasonic(TRIG_TOP, ECHO_TOP);
  delayMicroseconds(20);
  float dist_bottom = read_ultrasonic(TRIG_BOTTOM, ECHO_BOTTOM);

  // Update distance messages
  distance_top_msg.range = dist_top;
  distance_bottom_msg.range = dist_bottom;

  // Update encoder messages
  encoder_fr_msg.data = countFR;
  encoder_br_msg.data = countBR;
  encoder_fl_msg.data = countFL;
  encoder_bl_msg.data = countBL;

  // Read IMU data
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  // Update IMU message
  imu_msg.linear_acceleration.x = a.acceleration.x;
  imu_msg.linear_acceleration.y = a.acceleration.y;
  imu_msg.linear_acceleration.z = a.acceleration.z;
  imu_msg.angular_velocity.x = g.gyro.x;
  imu_msg.angular_velocity.y = g.gyro.y;
  imu_msg.angular_velocity.z = g.gyro.z;

  // Publish to ROS 2
  rcl_publish(&distance_top_pub, &distance_top_msg, NULL);
  rcl_publish(&distance_bottom_pub, &distance_bottom_msg, NULL);
  rcl_publish(&encoder_fr_pub, &encoder_fr_msg, NULL);
  rcl_publish(&encoder_br_pub, &encoder_br_msg, NULL);
  rcl_publish(&encoder_fl_pub, &encoder_fl_msg, NULL);
  rcl_publish(&encoder_bl_pub, &encoder_bl_msg, NULL);
  rcl_publish(&imu_pub, &imu_msg, NULL);

  // Debug output (CSV format for data logging)
  float wall_tilt = dist_top - dist_bottom;
  float robot_angle = atan2(a.acceleration.x, sqrt(a.acceleration.y * a.acceleration.y + a.acceleration.z * a.acceleration.z)) * 180.0 / PI;

  Serial.print(dist_top);
  Serial.print(",");
  Serial.print(dist_bottom);
  Serial.print(",");
  Serial.print(wall_tilt);
  Serial.print(",");
  Serial.print(countFR);
  Serial.print(",");
  Serial.print(countFL);
  Serial.print(",");
  Serial.println(robot_angle);
}

// ============ MESSAGE INITIALIZATION ============

void initialize_sensor_messages() {
  // Initialize distance_top message
  distance_top_msg.radiation_type = 0;  // ULTRASOUND
  distance_top_msg.field_of_view = 0.26;  // radians (~15 degrees)
  distance_top_msg.min_range = 0.02;  // 2cm
  distance_top_msg.max_range = 4.0;   // 4m
  strcpy(distance_top_msg.header.frame_id, "sensor_top");

  // Initialize distance_bottom message
  distance_bottom_msg.radiation_type = 0;  // ULTRASOUND
  distance_bottom_msg.field_of_view = 0.26;
  distance_bottom_msg.min_range = 0.02;
  distance_bottom_msg.max_range = 4.0;
  strcpy(distance_bottom_msg.header.frame_id, "sensor_bottom");

  // Initialize IMU message
  strcpy(imu_msg.header.frame_id, "imu_link");
}

// ============ HELPER FUNCTIONS ============

void stop_all_motors() {
  digitalWrite(MOTOR_A_IN1, LOW);
  digitalWrite(MOTOR_A_IN2, LOW);
  digitalWrite(MOTOR_B_IN1, LOW);
  digitalWrite(MOTOR_B_IN2, LOW);
  analogWrite(MOTOR_A_EN, 0);
  analogWrite(MOTOR_B_EN, 0);
}

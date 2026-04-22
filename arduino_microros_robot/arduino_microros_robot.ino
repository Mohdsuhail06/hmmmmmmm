/*
 * TRACKED ROBOT - HARDWARE TEST (Arduino AI)
 * Tests all hardware components: motors, encoders, sensors, IMU
 * 
 * Hardware:
 * - 2x DC Motors (L298N driver) for 4-wheel drive
 * - 4x Wheel encoders (quadrature encoders) - FR, BR, FL, BL
 * - 2x Ultrasonic sensors (distance measurement)
 * - 1x MPU6050 IMU (accelerometer/gyroscope)
 * 
 * Serial Output: CSV format for monitoring
 * Baud Rate: 115200
 */

#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

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

// ============ ROS 2 OBJECTS (REPLACED WITH SIMPLE VARIABLES) ============

// Movement control
float target_linear_velocity = 0.0;   // m/s
float target_angular_velocity = 0.0;  // rad/s
unsigned long last_cmd_time = 0;
#define CMD_TIMEOUT 1000  // 1 second timeout
unsigned long lastPrintTime = 0;

// Motor speed
int left_pwm = 0;
int right_pwm = 0;

// ============ MOTOR SPEED CALIBRATION ============
// Adjust these values to compensate for motor speed differences
// Increase RIGHT value if right wheel is slow and robot tilts left
// Increase LEFT value if left wheel is slow and robot tilts right
float motor_left_speed_factor = 1.0;   // Speed factor for left motor (increase to speed up left motor)
float motor_right_speed_factor = 0.5;  // Speed factor for right motor (increase to speed up right motor)

// ============ ENCODER CALIBRATION ============
// Calibrate these values based on your specific robot
#define ENCODER_COUNTS_PER_REV 20      // Counts per wheel revolution (adjust for your encoder)
#define WHEEL_DIAMETER_MM 80           // Wheel diameter in mm (adjust for your wheels)
#define WHEEL_CIRCUMFERENCE_MM (WHEEL_DIAMETER_MM * 3.14159)
#define COUNTS_PER_MM (ENCODER_COUNTS_PER_REV / WHEEL_CIRCUMFERENCE_MM)
#define COUNTS_PER_METER (COUNTS_PER_MM * 1000)
#define WHEEL_BASE_MM 150              // Distance between left and right wheels (adjust for your robot)
#define COUNTS_PER_DEGREE (WHEEL_BASE_MM * 3.14159 / (360.0 * WHEEL_CIRCUMFERENCE_MM) * ENCODER_COUNTS_PER_REV)

// ============ ENCODER VARIABLES (VOLATILE FOR INTERRUPTS) ============

volatile long countFR = 0;   // Front-Right
volatile long countBR = 0;   // Back-Right
volatile long countFL = 0;   // Front-Left
volatile long countBL = 0;   // Back-Left

volatile int lastFR_A = 0;
volatile int lastBR_A = 0;
volatile int lastFL_A = 0;
volatile int lastBL_A = 0;

// ============ FORWARD DECLARATIONS ============

void move_forward();
void move_forward(int pwm_val);
void move_backward(int pwm_val);
void turn_left(int pwm_val);
void turn_right(int pwm_val);
void stop_all_motors();
void read_encoders();
void execute_motion();
void publish_sensor_data();
void test_motors();
void move_distance_meters(float meters, int pwm_val);
void turn_degrees(float degrees, int pwm_val);

void setup() {
  Serial.begin(115200);
  delay(2000);  // Wait for Serial to initialize

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
    while (1) delay(10);  // Halt if IMU not found
  } else {
    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
    Serial.println("MPU6050 initialized!");
  }

  Serial.println("\n=== Arduino AI Robot Hardware Test Started ===");
  Serial.println("Format: dist_top, dist_bottom, wall_tilt, countFR, countBR, countFL, countBL, robot_angle");
  delay(1000);
  
  // Test motors
  Serial.println("\n--- Motor Test ---");
  test_motors();
}

// ============ MAIN LOOP ============

void loop() {
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

// ============ MOTOR TEST FUNCTION ============

void test_motors() {
  Serial.println("Starting SHORT movement sequence (for video test)...");
  
  Serial.println("Step 1: Moving STRAIGHT for 0.5 meters...");
  move_distance_meters(0.5, 250);
  delay(500);
  
  Serial.println("Step 2: Turning LEFT 90 degrees (Right forward, Left backward)...");
  turn_degrees(90, 200);
  delay(500);
  
  Serial.println("Step 3: Moving STRAIGHT for 0.2 meters (20cm) after turn...");
  move_distance_meters(0.2, 250);
  delay(500);
  
  Serial.println("Test complete!");
  stop_all_motors();
  delay(1000);
  
  Serial.println("Movement sequence complete. Starting sensor monitoring...\n");
}

// ============ MOVEMENT FUNCTIONS ============

void move_forward() {
  digitalWrite(MOTOR_A_IN1, LOW);
  digitalWrite(MOTOR_A_IN2, HIGH);
  analogWrite(MOTOR_A_EN, 250);
  
  digitalWrite(MOTOR_B_IN1, HIGH);
  digitalWrite(MOTOR_B_IN2, LOW);
  analogWrite(MOTOR_B_EN, 250);
}

void move_backward(int pwm_val = 250) {
  digitalWrite(MOTOR_A_IN1, HIGH);
  digitalWrite(MOTOR_A_IN2, LOW);
  analogWrite(MOTOR_A_EN, pwm_val);  // Motor A = LEFT (backward)
  
  digitalWrite(MOTOR_B_IN1, HIGH);
  digitalWrite(MOTOR_B_IN2, LOW);
  analogWrite(MOTOR_B_EN, pwm_val);  // Motor B = RIGHT (backward)
}

void move_forward(int pwm_val) {
  digitalWrite(MOTOR_A_IN1, LOW);
  digitalWrite(MOTOR_A_IN2, HIGH);
  analogWrite(MOTOR_A_EN, pwm_val);  // Motor A = LEFT (forward)
  
  digitalWrite(MOTOR_B_IN1, LOW);
  digitalWrite(MOTOR_B_IN2, HIGH);
  analogWrite(MOTOR_B_EN, pwm_val);  // Motor B = RIGHT (forward)
}

void turn_left(int pwm_val = 200) {
  digitalWrite(MOTOR_A_IN1, HIGH);
  digitalWrite(MOTOR_A_IN2, LOW);
  analogWrite(MOTOR_A_EN, pwm_val);  // Motor A = LEFT (backward)
  
  digitalWrite(MOTOR_B_IN1, LOW);
  digitalWrite(MOTOR_B_IN2, HIGH);
  analogWrite(MOTOR_B_EN, pwm_val);  // Motor B = RIGHT (forward)
}

void turn_right(int pwm_val = 200) {
  digitalWrite(MOTOR_A_IN1, LOW);
  digitalWrite(MOTOR_A_IN2, HIGH);
  analogWrite(MOTOR_A_EN, pwm_val);  // Motor A = LEFT (forward)
  
  digitalWrite(MOTOR_B_IN1, HIGH);
  digitalWrite(MOTOR_B_IN2, LOW);
  analogWrite(MOTOR_B_EN, pwm_val);  // Motor B = RIGHT (backward)
}

// ============ SIMPLE MOTION EXECUTION ============

void execute_motion() {
  // Currently just maintains idle state
  // In ROS mode, this would receive cmd_vel messages
  // For now, motors controlled by test_motors() function
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

  // Read IMU data
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  // Calculate metrics
  float wall_tilt = dist_top - dist_bottom;
  float robot_angle = atan2(a.acceleration.x, sqrt(a.acceleration.y * a.acceleration.y + a.acceleration.z * a.acceleration.z)) * 180.0 / PI;

  // Serial output in CSV format (for data logging)
  Serial.print(dist_top);
  Serial.print(",");
  Serial.print(dist_bottom);
  Serial.print(",");
  Serial.print(wall_tilt);
  Serial.print(",");
  Serial.print(countFR);
  Serial.print(",");
  Serial.print(countBR);
  Serial.print(",");
  Serial.print(countFL);
  Serial.print(",");
  Serial.print(countBL);
  Serial.print(",");
  Serial.println(robot_angle);
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

// ============ DISTANCE AND ANGLE MOVEMENT FUNCTIONS ============

void move_distance_meters(float meters, int pwm_val) {
  /*
   * Move forward/backward a specific distance using encoder feedback
   * Synchronizes left and right wheel speeds to keep robot moving straight
   * Parameters:
   *   meters: Distance in meters (positive = forward, negative = backward)
   *   pwm_val: PWM value for motor speed (0-255)
   */
  
  long target_counts = (long)(meters * COUNTS_PER_METER);
  
  // Reset encoder counts
  countFR = 0;
  countBR = 0;
  countFL = 0;
  countBL = 0;
  
  Serial.print("Target distance: ");
  Serial.print(meters);
  Serial.print(" meters (");
  Serial.print(target_counts);
  Serial.print(" encoder counts) | Speed factors: L=");
  Serial.print(motor_left_speed_factor);
  Serial.print(" R=");
  Serial.println(motor_right_speed_factor);
  
  // Move until average encoder count reaches target
  while (true) {
    read_encoders();
    
    // Calculate average of left and right wheels
    long avg_countL = (abs(countFL) + abs(countBL)) / 2;
    long avg_countR = (abs(countFR) + abs(countBR)) / 2;
    long avg_count = (avg_countL + avg_countR) / 2;
    
    // Calculate base speeds with speed factors applied
    int left_speed = (int)(pwm_val * motor_left_speed_factor);
    int right_speed = (int)(pwm_val * motor_right_speed_factor);
    
    // Constrain to valid PWM range
    left_speed = constrain(left_speed, 0, 255);
    right_speed = constrain(right_speed, 0, 255);
    
    // Real-time synchronization: if one wheel is ahead, slow it down
    long speed_diff = avg_countR - avg_countL;
    
    // Adjust speeds based on encoder difference
    if (speed_diff > 3) {  // Right is faster, slow it down
      right_speed = constrain(right_speed - 5, 0, 255);
    } else if (speed_diff < -3) {  // Left is faster, slow it down
      left_speed = constrain(left_speed - 5, 0, 255);
    }
    
    if (meters > 0) {
      // Move forward with synchronized speeds
      digitalWrite(MOTOR_A_IN1, LOW);
      digitalWrite(MOTOR_A_IN2, HIGH);
      analogWrite(MOTOR_A_EN, left_speed);   // Motor A = LEFT
      
      digitalWrite(MOTOR_B_IN1, LOW);
      digitalWrite(MOTOR_B_IN2, HIGH);
      analogWrite(MOTOR_B_EN, right_speed);  // Motor B = RIGHT
    } else {
      // Move backward with synchronized speeds
      digitalWrite(MOTOR_A_IN1, HIGH);
      digitalWrite(MOTOR_A_IN2, LOW);
      analogWrite(MOTOR_A_EN, left_speed);   // Motor A = LEFT
      
      digitalWrite(MOTOR_B_IN1, HIGH);
      digitalWrite(MOTOR_B_IN2, LOW);
      analogWrite(MOTOR_B_EN, right_speed);  // Motor B = RIGHT
    }
    
    // Check if target reached
    if (avg_count >= abs(target_counts)) {
      stop_all_motors();
      Serial.print("Distance reached: L=");
      Serial.print(avg_countL);
      Serial.print(" R=");
      Serial.print(avg_countR);
      Serial.print(" Avg=");
      Serial.println(avg_count);
      break;
    }
    
    delay(50);  // Small delay for next sensor read
  }
}

void turn_degrees(float degrees, int pwm_val) {
  /*
   * Turn left (positive degrees) or right (negative degrees) using tank-style rotation
   * One wheel goes forward, the other goes backward for in-place rotation
   * Uses encoder feedback to ensure accurate rotation
   * Parameters:
   *   degrees: Angle to turn in degrees (positive = left, negative = right)
   *   pwm_val: PWM value for motor speed (0-255)
   */
  
  // For 90 degrees, we need to calculate how far each wheel travels
  // Using a multiplier to account for actual encoder resolution
  long target_counts = (long)(abs(degrees) * 2);  // Simple approach: ~2 counts per degree as baseline
  
  // Reset encoder counts
  countFR = 0;
  countBR = 0;
  countFL = 0;
  countBL = 0;
  
  Serial.print("Target rotation: ");
  Serial.print(degrees);
  Serial.print(" degrees (");
  Serial.print(target_counts);
  Serial.println(" encoder counts) - Tank style rotation");
  
  unsigned long turn_start = millis();
  unsigned long turn_timeout = abs(degrees) * 50;  // ~50ms per degree timeout
  
  // Turn until encoder count reaches target or timeout
  while (millis() - turn_start < turn_timeout) {
    read_encoders();
    
    // Calculate average encoder count for right wheel only (it's driving the turn)
    long avg_countR = (abs(countFR) + abs(countBR)) / 2;
    long avg_countL = (abs(countFL) + abs(countBL)) / 2;
    
    Serial.print("Turn encoder: L=");
    Serial.print(avg_countL);
    Serial.print(" R=");
    Serial.print(avg_countR);
    Serial.print(" Target=");
    Serial.println(target_counts);
    
    // Calculate speeds with speed factors
    int left_speed = (int)(pwm_val * motor_left_speed_factor);
    int right_speed = (int)(pwm_val * motor_right_speed_factor);
    
    left_speed = constrain(left_speed, 0, 255);
    right_speed = constrain(right_speed, 0, 255);
    
    if (degrees > 0) {
      // Turn LEFT: Right wheel FORWARD, Left wheel BACKWARD
      digitalWrite(MOTOR_A_IN1, HIGH);
      digitalWrite(MOTOR_A_IN2, LOW);
      analogWrite(MOTOR_A_EN, left_speed);   // Motor A = LEFT (BACKWARD)
      
      digitalWrite(MOTOR_B_IN1, LOW);
      digitalWrite(MOTOR_B_IN2, HIGH);
      analogWrite(MOTOR_B_EN, right_speed);  // Motor B = RIGHT (FORWARD)
    } else {
      // Turn RIGHT: Right wheel BACKWARD, Left wheel FORWARD
      digitalWrite(MOTOR_A_IN1, LOW);
      digitalWrite(MOTOR_A_IN2, HIGH);
      analogWrite(MOTOR_A_EN, left_speed);   // Motor A = LEFT (FORWARD)
      
      digitalWrite(MOTOR_B_IN1, HIGH);
      digitalWrite(MOTOR_B_IN2, LOW);
      analogWrite(MOTOR_B_EN, right_speed);  // Motor B = RIGHT (BACKWARD)
    }
    
    // Check if target reached (use the wheel moving faster)
    long max_count = (avg_countR > avg_countL) ? avg_countR : avg_countL;
    
    if (max_count >= target_counts) {
      Serial.println("Target reached!");
      break;
    }
    
    delay(50);  // Small delay for next sensor read
  }
  
  stop_all_motors();
  Serial.println("Turn complete.");
  delay(500);  // Brief pause after turn
}

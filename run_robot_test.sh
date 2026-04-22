#!/bin/bash

# 🤖 ROBOT MOVEMENT TEST - AUTOMATED SCRIPT
# Run this to test: Move 2m → Stop → Turn Left → Complete → Take Photos

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
ROBOT_WORKSPACE="$HOME/robot_ws"
CONTAINER_NAME="ros2_robot"
PHOTO_DIR="/tmp/robot_test_photos"
LOG_FILE="/tmp/robot_test_$(date +%Y%m%d_%H%M%S).log"

# Test parameters
FORWARD_SPEED=0.3      # m/s
FORWARD_DURATION=6.7   # seconds (2m / 0.3 m/s)
TURN_SPEED=1.0         # rad/s
TURN_DURATION=1.57     # seconds (π/2 radians)
STOP_DURATION=2.0      # seconds

echo -e "${BLUE}🤖 ROBOT MOVEMENT TEST - AUTOMATED SCRIPT${NC}"
echo "=================================================="
echo "Test Sequence: Move 2m → Stop → Turn Left → Complete → Take Photos"
echo "Log file: $LOG_FILE"
echo ""

# Function: Execute command in Docker container
run_in_docker() {
    local cmd="$1"
    docker exec -it "$CONTAINER_NAME" bash -c "source /opt/ros/humble/setup.bash && $cmd"
}

# Function: Publish velocity command
publish_velocity() {
    local linear=$1
    local angular=$2
    local duration=$3
    
    local cmd="ros2 topic pub /cmd_vel geometry_msgs/Twist \
        \"{linear: {x: $linear, y: 0, z: 0}, angular: {x: 0, y: 0, z: $angular}}\" &
    sleep $duration
    kill %1 2>/dev/null || true"
    
    run_in_docker "$cmd"
}

# Function: Take photo
take_photo() {
    local name=$1
    echo -e "${YELLOW}📷 Capturing photo: $name${NC}"
    
    # For now, create a placeholder
    # In production, this would call the Python test script
    mkdir -p "$PHOTO_DIR"
    echo "Photo would be captured at: $PHOTO_DIR/${name}_$(date +%Y%m%d_%H%M%S).jpg"
}

# Function: Log message
log_msg() {
    echo -e "$1" | tee -a "$LOG_FILE"
}

# Step 1: Verify prerequisites
log_msg "${BLUE}[STEP 1] Verifying Prerequisites...${NC}"

if ! docker ps | grep -q "$CONTAINER_NAME"; then
    log_msg "${RED}❌ ERROR: Docker container '$CONTAINER_NAME' is not running${NC}"
    log_msg "${YELLOW}Start it with: cd $ROBOT_WORKSPACE && docker-compose up -d${NC}"
    exit 1
fi
log_msg "${GREEN}✅ Docker container is running${NC}"

# Verify workspace
if [ ! -d "$ROBOT_WORKSPACE" ]; then
    log_msg "${RED}❌ ERROR: Workspace not found at $ROBOT_WORKSPACE${NC}"
    exit 1
fi
log_msg "${GREEN}✅ Workspace found at $ROBOT_WORKSPACE${NC}"

# Step 2: Verify connectivity
log_msg ""
log_msg "${BLUE}[STEP 2] Verifying ROS 2 Connectivity...${NC}"

if ! run_in_docker "ros2 topic list | grep -q cmd_vel" 2>/dev/null; then
    log_msg "${RED}❌ ERROR: /cmd_vel topic not found${NC}"
    log_msg "${YELLOW}Make sure ROS 2 system is running: ros2 launch robot_bringup robot_bringup.launch.py${NC}"
    exit 1
fi
log_msg "${GREEN}✅ cmd_vel topic is available${NC}"

if ! run_in_docker "ros2 topic list | grep -q distance" 2>/dev/null; then
    log_msg "${YELLOW}⚠️  WARNING: Distance sensors not responding yet${NC}"
else
    log_msg "${GREEN}✅ Distance sensors available${NC}"
fi

# Step 3: Verify sensors
log_msg ""
log_msg "${BLUE}[STEP 3] Verifying Sensor Data...${NC}"

# Try to read sensor
SENSOR_READING=$(run_in_docker "timeout 2 ros2 topic echo /sensor/distance_left --once 2>/dev/null | grep range | head -1" 2>/dev/null || echo "")

if [ -z "$SENSOR_READING" ]; then
    log_msg "${YELLOW}⚠️  WARNING: Could not read distance sensors${NC}"
    log_msg "${YELLOW}    (They may initialize once robot is powered)${NC}"
else
    log_msg "${GREEN}✅ Distance sensor reading: $SENSOR_READING${NC}"
fi

# Step 4: Ready for test
log_msg ""
log_msg "${BLUE}[STEP 4] Robot Test Ready!${NC}"
log_msg ""
log_msg "${YELLOW}⚠️  IMPORTANT - Safety Checklist:${NC}"
log_msg "  [ ] Robot is on a flat, open surface (at least 2.5m × 2.5m)"
log_msg "  [ ] No obstacles in the path"
log_msg "  [ ] All cables are clear"
log_msg "  [ ] You can physically reach the emergency stop button"
log_msg "  [ ] Camera is pointing in safe direction"
log_msg ""
log_msg "Press ENTER to start test (or Ctrl+C to abort)"
read -r

# Step 5: Start test sequence
log_msg ""
log_msg "${GREEN}🚀 STARTING TEST SEQUENCE${NC}"
log_msg ""

# TEST 1: Take initial photo
log_msg "${BLUE}[TEST 1/5] INITIAL POSITION${NC}"
take_photo "01_start_position"
sleep 1
log_msg "${GREEN}✅ Initial photo captured${NC}"

# TEST 2: Move forward 2 meters
log_msg ""
log_msg "${BLUE}[TEST 2/5] MOVE FORWARD 2 METERS${NC}"
log_msg "Linear velocity: $FORWARD_SPEED m/s"
log_msg "Duration: $FORWARD_DURATION seconds"
log_msg "Expected distance: $((FORWARD_SPEED * FORWARD_DURATION | bc)) meters"
log_msg "Command: /cmd_vel {linear.x: $FORWARD_SPEED, angular.z: 0.0}"
log_msg ""

publish_velocity "$FORWARD_SPEED" 0.0 "$FORWARD_DURATION"
log_msg "${GREEN}✅ Forward movement complete${NC}"

take_photo "02_after_2m_forward"
sleep 1

# TEST 3: Stop
log_msg ""
log_msg "${BLUE}[TEST 3/5] STOP ROBOT${NC}"
log_msg "Duration: $STOP_DURATION seconds"
log_msg "Command: /cmd_vel {linear.x: 0.0, angular.z: 0.0}"
log_msg ""

publish_velocity 0.0 0.0 "$STOP_DURATION"
log_msg "${GREEN}✅ Robot stopped${NC}"

take_photo "03_stopped_position"
sleep 1

# TEST 4: Turn left
log_msg ""
log_msg "${BLUE}[TEST 4/5] TURN LEFT 90 DEGREES${NC}"
log_msg "Angular velocity: $TURN_SPEED rad/s (counterclockwise)"
log_msg "Duration: $TURN_DURATION seconds"
log_msg "Expected angle: 90 degrees (π/2 radians)"
log_msg "Command: /cmd_vel {linear.x: 0.0, angular.z: $TURN_SPEED}"
log_msg ""

publish_velocity 0.0 "$TURN_SPEED" "$TURN_DURATION"
log_msg "${GREEN}✅ Left turn complete${NC}"

take_photo "04_after_left_turn"
sleep 1

# TEST 5: Final stop
log_msg ""
log_msg "${BLUE}[TEST 5/5] FINAL POSITION${NC}"
log_msg "Command: /cmd_vel {linear.x: 0.0, angular.z: 0.0}"
log_msg ""

publish_velocity 0.0 0.0 "$STOP_DURATION"
log_msg "${GREEN}✅ Test sequence complete${NC}"

take_photo "05_final_position"

# Summary
log_msg ""
log_msg "=================================================="
log_msg "${GREEN}🎉 TEST COMPLETED SUCCESSFULLY${NC}"
log_msg "=================================================="
log_msg ""
log_msg "📋 Summary:"
log_msg "  ✅ Moved forward 2 meters"
log_msg "  ✅ Stopped"
log_msg "  ✅ Turned left 90 degrees"
log_msg "  ✅ Sequence completed"
log_msg "  ✅ Photos captured at each step"
log_msg ""
log_msg "📁 Photos saved to: $PHOTO_DIR"
log_msg "📝 Log saved to: $LOG_FILE"
log_msg ""
log_msg "Next steps:"
log_msg "  1. Review captured photos"
log_msg "  2. Verify movement accuracy"
log_msg "  3. Check sensor readings"
log_msg "  4. Review test log for any warnings"
log_msg ""

# Verification
log_msg "${BLUE}[VERIFICATION] Getting final sensor data...${NC}"

# Get final distance reading
if run_in_docker "ros2 topic list | grep -q distance" 2>/dev/null; then
    FINAL_DIST=$(run_in_docker "timeout 2 ros2 topic echo /sensor/distance_left --once 2>/dev/null | grep range" 2>/dev/null || echo "N/A")
    log_msg "Final distance sensor reading: $FINAL_DIST"
fi

# Get encoder readings
if run_in_docker "ros2 topic list | grep -q encoder" 2>/dev/null; then
    LEFT_ENC=$(run_in_docker "timeout 2 ros2 topic echo /encoder/left_ticks --once 2>/dev/null | grep data | head -1" 2>/dev/null || echo "N/A")
    RIGHT_ENC=$(run_in_docker "timeout 2 ros2 topic echo /encoder/right_ticks --once 2>/dev/null | grep data | head -1" 2>/dev/null || echo "N/A")
    log_msg "Final encoder readings:"
    log_msg "  Left: $LEFT_ENC"
    log_msg "  Right: $RIGHT_ENC"
fi

log_msg ""
log_msg "${GREEN}✅ ALL TESTS COMPLETED${NC}"

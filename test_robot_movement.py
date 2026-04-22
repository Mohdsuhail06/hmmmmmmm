#!/usr/bin/env python3
"""
ROBOT MOVEMENT TEST SCRIPT
Tests: Move 2m → Stop → Turn Left → Complete → Take Photos
"""

import rclpy
from rclpy.node import Node
from geometry_msgs.msg import Twist
from sensor_msgs.msg import Image
from std_msgs.msg import String
import time
import cv2
from cv_bridge import CvBridge
import os
from datetime import datetime

class RobotMovementTester(Node):
    def __init__(self):
        super().__init__('robot_movement_tester')
        
        self.bridge = CvBridge()
        
        # Publishers
        self.cmd_vel_pub = self.create_publisher(Twist, 'cmd_vel', 10)
        
        # Subscribers
        self.camera_sub = self.create_subscription(
            Image, '/camera/image_raw', self.camera_callback, 5)
        self.left_dist_sub = self.create_subscription(
            String, 'sensor/distance_left', self.distance_callback, 10)
        self.right_dist_sub = self.create_subscription(
            String, 'sensor/distance_right', self.distance_callback, 10)
        self.left_enc_sub = self.create_subscription(
            String, 'encoder/left_ticks', self.encoder_callback, 10)
        self.right_enc_sub = self.create_subscription(
            String, 'encoder/right_ticks', self.encoder_callback, 10)
        
        # Storage
        self.latest_frame = None
        self.latest_distances = {'left': 0.0, 'right': 0.0}
        self.latest_encoders = {'left': 0, 'right': 0}
        
        # Test photos directory
        self.photo_dir = '/tmp/robot_test_photos'
        os.makedirs(self.photo_dir, exist_ok=True)
        
        self.test_results = []
        
        self.get_logger().info('🤖 Robot Movement Tester Initialized')
        self.get_logger().info(f'📁 Photos will be saved to: {self.photo_dir}')
    
    def camera_callback(self, msg):
        """Receive and store camera frames"""
        try:
            self.latest_frame = self.bridge.imgmsg_to_cv2(msg, "bgr8")
        except Exception as e:
            self.get_logger().error(f"Camera error: {e}")
    
    def distance_callback(self, msg):
        """Store distance sensor data"""
        try:
            # Parse sensor name from topic
            if 'left' in msg.data or 'distance_left' in str(self):
                self.latest_distances['left'] = float(msg.data)
            else:
                self.latest_distances['right'] = float(msg.data)
        except:
            pass
    
    def encoder_callback(self, msg):
        """Store encoder data"""
        try:
            if 'left' in msg.data:
                self.latest_encoders['left'] = int(msg.data)
            else:
                self.latest_encoders['right'] = int(msg.data)
        except:
            pass
    
    def publish_cmd_vel(self, linear_x, angular_z, duration=1.0):
        """Send velocity command for specified duration"""
        msg = Twist()
        msg.linear.x = linear_x
        msg.angular.z = angular_z
        
        start_time = time.time()
        while time.time() - start_time < duration:
            self.cmd_vel_pub.publish(msg)
            time.sleep(0.1)
    
    def take_photo(self, name):
        """Capture and save a photo"""
        if self.latest_frame is None:
            self.get_logger().warning(f"⚠️  No camera frame available for {name}")
            return False
        
        timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
        filename = f'{self.photo_dir}/{name}_{timestamp}.jpg'
        
        cv2.imwrite(filename, self.latest_frame)
        self.get_logger().info(f'📷 Photo saved: {filename}')
        self.test_results.append({
            'step': name,
            'timestamp': timestamp,
            'file': filename,
            'status': '✅ PASS'
        })
        return True
    
    def verify_sensors(self):
        """Check if sensors are providing valid data"""
        self.get_logger().info('🔍 Verifying sensors...')
        
        left_dist = self.latest_distances['left']
        right_dist = self.latest_distances['right']
        left_enc = self.latest_encoders['left']
        right_enc = self.latest_encoders['right']
        
        self.get_logger().info(f'📊 Distance Sensors:')
        self.get_logger().info(f'   Left: {left_dist} m')
        self.get_logger().info(f'   Right: {right_dist} m')
        self.get_logger().info(f'🔄 Encoders:')
        self.get_logger().info(f'   Left: {left_enc} ticks')
        self.get_logger().info(f'   Right: {right_enc} ticks')
        
        if left_dist > 0 and right_dist > 0:
            self.get_logger().info('✅ Distance sensors: WORKING')
            return True
        else:
            self.get_logger().error('❌ Distance sensors: NOT WORKING')
            return False
    
    async def run_test(self):
        """Execute complete test sequence"""
        self.get_logger().info('=' * 60)
        self.get_logger().info('🚀 STARTING ROBOT MOVEMENT TEST SEQUENCE')
        self.get_logger().info('=' * 60)
        
        try:
            # Step 1: Verify sensors
            self.get_logger().info('\n[STEP 1] Verifying Sensors')
            self.get_logger().info('-' * 40)
            time.sleep(1)  # Wait for sensors to initialize
            self.verify_sensors()
            time.sleep(1)
            
            # Step 2: Move forward 2 meters
            self.get_logger().info('\n[STEP 2] Moving Forward 2 Meters')
            self.get_logger().info('-' * 40)
            self.get_logger().info('⬆️  Publishing cmd_vel: linear.x=0.3 m/s')
            self.get_logger().info('📏 Expected time: ~6.7 seconds (2m ÷ 0.3m/s)')
            self.take_photo('01_start_position')
            
            self.publish_cmd_vel(linear_x=0.3, angular_z=0.0, duration=6.7)
            
            self.get_logger().info('✅ Moved 2 meters forward')
            self.take_photo('02_after_2m_forward')
            time.sleep(1)
            
            # Step 3: Stop
            self.get_logger().info('\n[STEP 3] Stop')
            self.get_logger().info('-' * 40)
            self.get_logger().info('⏹️  Publishing cmd_vel: linear.x=0.0, angular.z=0.0')
            self.publish_cmd_vel(linear_x=0.0, angular_z=0.0, duration=2.0)
            
            self.get_logger().info('✅ Robot stopped')
            self.take_photo('03_stopped_position')
            time.sleep(1)
            
            # Step 4: Turn Left
            self.get_logger().info('\n[STEP 4] Turn Left (90 degrees)')
            self.get_logger().info('-' * 40)
            self.get_logger().info('⬅️  Publishing cmd_vel: angular.z=1.0 rad/s')
            self.get_logger().info('📐 Expected time: ~1.57 seconds (π/2 ÷ 1.0)')
            
            self.publish_cmd_vel(linear_x=0.0, angular_z=1.0, duration=1.57)
            
            self.get_logger().info('✅ Turned left 90 degrees')
            self.take_photo('04_after_left_turn')
            time.sleep(1)
            
            # Step 5: Complete - Return to start
            self.get_logger().info('\n[STEP 5] Complete - Return Movement')
            self.get_logger().info('-' * 40)
            self.get_logger().info('🔄 Sequence completion movement')
            self.publish_cmd_vel(linear_x=0.0, angular_z=0.0, duration=2.0)
            
            self.get_logger().info('✅ Sequence completed')
            self.take_photo('05_final_position')
            time.sleep(1)
            
            # Step 6: Final verification
            self.get_logger().info('\n[STEP 6] Final Sensor Verification')
            self.get_logger().info('-' * 40)
            self.verify_sensors()
            
            # Print summary
            self.print_summary()
            
        except Exception as e:
            self.get_logger().error(f'❌ Test failed with error: {e}')
    
    def print_summary(self):
        """Print test summary"""
        self.get_logger().info('\n' + '=' * 60)
        self.get_logger().info('📋 TEST SUMMARY')
        self.get_logger().info('=' * 60)
        
        for i, result in enumerate(self.test_results, 1):
            self.get_logger().info(f"{i}. {result['step']}: {result['status']}")
            self.get_logger().info(f"   📷 {result['file']}")
        
        self.get_logger().info('=' * 60)
        self.get_logger().info(f'📁 All photos saved to: {self.photo_dir}')
        self.get_logger().info('🎉 TEST COMPLETE')
        self.get_logger().info('=' * 60)


def main(args=None):
    rclpy.init(args=args)
    
    tester = RobotMovementTester()
    
    try:
        # Run test in background
        import asyncio
        loop = asyncio.get_event_loop()
        loop.run_until_complete(tester.run_test())
    except KeyboardInterrupt:
        tester.get_logger().info('⏹️  Test interrupted')
    finally:
        tester.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()

"""
ROS 2 LAUNCH FILES FOR TRACKED ROBOT
robot_bringup.launch.py - Main launch file that starts all nodes
"""

import os
from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription, ExecuteProcess
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
import xacro

# ============================================================
# MAIN BRING-UP LAUNCH FILE
# ============================================================

def generate_launch_description():
    
    # Get package paths
    robot_pkg_dir = FindPackageShare(package='robot_bringup').find('robot_bringup')
    slam_pkg_dir = FindPackageShare(package='slam_toolbox').find('slam_toolbox')
    
    # Configuration
    use_sim_time = LaunchConfiguration('use_sim_time', default='false')
    slam_params = PathJoinSubstitution([robot_pkg_dir, 'config', 'slam.yaml'])
    nav_params = PathJoinSubstitution([robot_pkg_dir, 'config', 'navigation.yaml'])
    lidar_params = PathJoinSubstitution([robot_pkg_dir, 'config', 'lidar.yaml'])
    
    return LaunchDescription([
        # ========== MICRO-ROS AGENT (Arduino Bridge) ==========
        Node(
            package='micro_ros_agent',
            executable='micro_ros_agent',
            name='micro_ros_agent',
            arguments=['serial', '--dev', '/dev/ttyUSB0', '-b', '115200'],
            output='screen',
            emulate_tty=True,
        ),
        
        # ========== RPLidar Node ==========
        Node(
            package='sllidar_ros2',
            executable='sllidar_node',
            name='lidar_node',
            parameters=[lidar_params],
            output='screen',
            emulate_tty=True,
        ),
        
        # ========== SLAM (Simultaneous Localization and Mapping) ==========
        Node(
            package='slam_toolbox',
            executable='async_slam_toolbox_node',
            name='slam_toolbox',
            output='screen',
            emulate_tty=True,
            parameters=[
                slam_params,
                {'use_sim_time': use_sim_time},
            ],
            remappings=[
                ('/scan', '/scan'),  # From RPLidar
                ('/tf', '/tf'),
                ('/tf_static', '/tf_static'),
            ],
        ),
        
        # ========== ROBOT ODOMETRY NODE (from encoders) ==========
        Node(
            package='robot_odometry',
            executable='odometry_node',
            name='odometry',
            output='screen',
            emulate_tty=True,
            parameters=[{
                'wheel_diameter': 0.1,
                'wheel_base': 0.15,
                'ticks_per_revolution': 100,
            }],
        ),
        
        # ========== MOTOR CONTROLLER NODE ==========
        Node(
            package='robot_controller',
            executable='motor_controller',
            name='motor_controller',
            output='screen',
            emulate_tty=True,
            parameters=[{
                'max_linear_speed': 0.5,
                'max_angular_speed': 2.0,
                'cmd_vel_timeout': 1.0,
            }],
        ),
        
        # ========== CAMERA NODE (Object Detection) ==========
        Node(
            package='robot_vision',
            executable='camera_node',
            name='camera',
            output='screen',
            emulate_tty=True,
            parameters=[{
                'camera_id': 0,
                'frame_width': 640,
                'frame_height': 480,
                'fps': 30,
            }],
        ),
        
        # ========== YOLOV8 DETECTION NODE ==========
        Node(
            package='robot_vision',
            executable='yolo_detection',
            name='yolo_detector',
            output='screen',
            emulate_tty=True,
            parameters=[{
                'model_path': '/root/ros2_ws/models/best.onnx',
                'confidence_threshold': 0.5,
                'classes_to_detect': ['electrical_fixture', 'outlet', 'switch'],
            }],
        ),
        
        # ========== ROSBRIDGE (Web Dashboard) ==========
        Node(
            package='rosbridge_server',
            executable='rosbridge_websocket',
            name='rosbridge_websocket',
            parameters=[{
                'address': '0.0.0.0',
                'port': 9090,
            }],
            output='screen',
            emulate_tty=True,
        ),
        
        # ========== TELEGRAM BOT NODE (Command & Control) ==========
        Node(
            package='robot_teleop',
            executable='telegram_bot',
            name='telegram_bot',
            output='screen',
            emulate_tty=True,
            parameters=[{
                'bot_token': '<YOUR_BOT_TOKEN>',
                'allowed_chat_id': '<YOUR_CHAT_ID>',
            }],
        ),
        
        # ========== TF2 STATIC BROADCASTER ==========
        Node(
            package='tf2_ros',
            executable='static_transform_publisher',
            arguments=['0', '0', '0.1', '0', '0', '0', 'base_link', 'lidar_link'],
            output='screen',
        ),
        
        # ========== RViz2 (3D Visualization) ==========
        Node(
            package='rviz2',
            executable='rviz2',
            name='rviz2',
            arguments=['-d', PathJoinSubstitution([robot_pkg_dir, 'rviz', 'robot.rviz'])],
            output='screen',
        ),
    ])

---

# FILE 2: lidar.launch.py
---

from launch import LaunchDescription
from launch_ros.actions import Node
from launch.substitutions import PathJoinSubstitution
from launch_ros.substitutions import FindPackageShare

def generate_launch_description():
    
    config_dir = FindPackageShare(package='robot_bringup').find('robot_bringup')
    lidar_params = PathJoinSubstitution([config_dir, 'config', 'lidar.yaml'])
    
    return LaunchDescription([
        Node(
            package='sllidar_ros2',
            executable='sllidar_node',
            name='lidar',
            parameters=[lidar_params],
            output='screen',
            emulate_tty=True,
        ),
    ])

---

# FILE 3: slam.launch.py
---

from launch import LaunchDescription
from launch_ros.actions import Node
from launch.substitutions import PathJoinSubstitution, LaunchConfiguration
from launch_ros.substitutions import FindPackageShare

def generate_launch_description():
    
    config_dir = FindPackageShare(package='robot_bringup').find('robot_bringup')
    slam_params = PathJoinSubstitution([config_dir, 'config', 'slam.yaml'])
    use_sim_time = LaunchConfiguration('use_sim_time', default='false')
    
    return LaunchDescription([
        Node(
            package='slam_toolbox',
            executable='async_slam_toolbox_node',
            name='slam_toolbox',
            output='screen',
            emulate_tty=True,
            parameters=[
                slam_params,
                {'use_sim_time': use_sim_time},
            ],
        ),
    ])

---

# FILE 4: nav2.launch.py (Autonomous Navigation)
---

from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.substitutions import FindPackageShare
from launch.substitutions import PathJoinSubstitution
import os

def generate_launch_description():
    
    nav2_dir = FindPackageShare(package='nav2_bringup').find('nav2_bringup')
    config_dir = FindPackageShare(package='robot_bringup').find('robot_bringup')
    nav_params = PathJoinSubstitution([config_dir, 'config', 'navigation.yaml'])
    
    return LaunchDescription([
        # Include Nav2 launch file
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(
                PathJoinSubstitution([nav2_dir, 'launch', 'bringup_launch.py'])
            ),
            launch_arguments={
                'params_file': nav_params,
                'use_sim_time': 'false',
            }.items(),
        ),
    ])

---

# FILE 5: vision.launch.py (Camera + YOLOv8)
---

from launch import LaunchDescription
from launch_ros.actions import Node
from launch.substitutions import PathJoinSubstitution
from launch_ros.substitutions import FindPackageShare

def generate_launch_description():
    
    config_dir = FindPackageShare(package='robot_bringup').find('robot_bringup')
    
    return LaunchDescription([
        # Camera node
        Node(
            package='robot_vision',
            executable='camera_node',
            name='camera',
            output='screen',
            emulate_tty=True,
            parameters=[{
                'camera_id': 0,
                'frame_width': 640,
                'frame_height': 480,
                'fps': 30,
                'camera_info_url': 'file://' + str(PathJoinSubstitution([config_dir, 'camera_info.yaml'])),
            }],
        ),
        
        # YOLOv8 Detection
        Node(
            package='robot_vision',
            executable='yolo_detection',
            name='yolo_detector',
            output='screen',
            emulate_tty=True,
            parameters=[{
                'model_path': '/root/ros2_ws/models/best.onnx',
                'confidence_threshold': 0.5,
                'input_topic': '/camera/image_raw',
                'output_topic': '/detections',
                'classes': ['electrical_fixture', 'outlet', 'switch'],
            }],
        ),
    ])

---

# CONFIG FILES (Referenced in launch files)

# config/slam.yaml
---

slam_toolbox:
  ros__parameters:
    # SLAM parameters
    use_scan_matching: true
    use_scan_bagging: true
    minimum_travel_distance: 0.1
    minimum_travel_heading: 0.1
    scan_buffer_size: 10
    score_threshold: 0.1
    
    # Loop closure
    use_loop_closure: true
    loop_search_maximum_distance: 3.0
    do_loop_closure: true
    
    # Output
    map_update_interval: 1.0
    resolution: 0.05
    max_laser_range: 5.0
    minimum_time_interval: 0.5

---

# config/lidar.yaml
---

sllidar_ros2:
  ros__parameters:
    channel_type: serial
    serial_port: /dev/ttyUSB1
    serial_baudrate: 115200
    frame_id: lidar_link
    inverted: false
    angle_compensate: true
    scan_mode: Standard
    
    # Lidar parameters
    max_distance: 12.0
    min_distance: 0.15
    angle_min: -3.14159
    angle_max: 3.14159

---

# config/navigation.yaml
---

nav2_bringup:
  ros__parameters:
    use_sim_time: false
    
    # Costmap configuration
    global_costmap:
      global_frame: map
      robot_base_frame: base_link
      update_frequency: 5.0
      publish_frequency: 2.0
      width: 50
      height: 50
      resolution: 0.05
      
    local_costmap:
      global_frame: odom
      robot_base_frame: base_link
      update_frequency: 10.0
      publish_frequency: 10.0
      width: 10
      height: 10
      resolution: 0.05
      
    planner_server:
      ros__parameters:
        expected_planner_frequency: 10.0
        planner_plugins: ['GridBased']
        GridBased:
          plugin: nav2_navfn_planner/NavfnPlanner
          
    controller_server:
      ros__parameters:
        controller_frequency: 10.0
        controller_plugins: ['FollowPath']
        FollowPath:
          plugin: nav2_rotation_shim_controller/RotationShimController

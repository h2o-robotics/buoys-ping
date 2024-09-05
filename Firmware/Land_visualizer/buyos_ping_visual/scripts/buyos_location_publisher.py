#!/usr/bin/env python3

import rospy
import serial
from foxglove_msgs.msg import GeoJSON, LocationFix

import math_support

class BuyosPublisher:
    def __init__(self):
        self.frequency = 0.5 # [Hz]

        self.serial_port_ID = "/dev/ttyUSB0"
        self.serial_port_BAUD = 9600

        self.buyos_overlay_publisher = rospy.Publisher('/buyosOverlay', GeoJSON, queue_size=10)
        
        self.diver_publisher = rospy.Publisher('/diver_loc', LocationFix, queue_size=10)
        
        self.buoy1_pub = rospy.Publisher('/buoy1_loc', LocationFix, queue_size=10)
        self.buoy2_pub = rospy.Publisher('/buoy2_loc', LocationFix, queue_size=10)
        self.buoy3_pub = rospy.Publisher('/buoy3_loc', LocationFix, queue_size=10)

        # Start serial interface
        self.serial_int = serial.Serial(self.serial_port_ID, self.serial_port_BAUD, timeout=1)
        self.start_collecting_buoy_data = False

        rospy.Timer(rospy.Duration(1.0 / 2.0), self.process_coast_unit)

    def process_coast_unit(self, _):
        buoy_data = []

        while len(buoy_data) < 3:
            line = self.serial_int.readline().decode('utf-8').strip()
            rospy.loginfo(f'COST_UNIT -> {line}')
            
            if '[RNG] B1' in line:
                self.start_collecting_buoy_data = True
                rospy.loginfo("New data cycle has started, collecting the data...")

            parsed_data = self.parse_buyo_message(line)

            if parsed_data:
                buoy_data.append(parsed_data)
                rospy.loginfo(str(parsed_data))
            else:
                buoy_data = []
                rospy.loginfo("Data not complete! Droping previous metric...")
        
        rospy.loginfo(f"NEW METRIC -> {buoy_data}")

        calculated_data = math_support.calculate_diver(buoy_data)

        json_dump = GeoJSON()
        json_dump.geojson = calculated_data[0]

        self.buyos_overlay_publisher.publish(json_dump)

        diver_msg = LocationFix()
        diver_msg.latitude = calculated_data[1]
        diver_msg.longitude = calculated_data[2]
        diver_msg.altitude = calculated_data[3]

        self.diver_publisher.publish(diver_msg)

        buoy1_loc = LocationFix()
        buoy1_loc.latitude = buoy_data[0][1]
        buoy1_loc.longitude = buoy_data[0][2]

        self.buoy1_pub.publish(buoy1_loc)

        buoy2_loc = LocationFix()
        buoy2_loc.latitude = buoy_data[1][1]
        buoy2_loc.longitude = buoy_data[1][2]

        self.buoy1_pub.publish(buoy2_loc)

        buoy3_loc = LocationFix()
        buoy3_loc.latitude = buoy_data[2][1]
        buoy3_loc.longitude = buoy_data[2][2]

        self.buoy3_pub.publish(buoy3_loc)
    
    def parse_buyo_message(self,message):
        if message.startswith('[RNG] B') and ',RNG,' in message and self.start_collecting_buoy_data:
            parts = message.split(',')
            
            if len(parts) == 6:
                distance = float(parts[2])
                latitude = float(parts[3])
                longitude = float(parts[4])
                
                return (distance, latitude, longitude)
        
        self.start_collecting_buoy_data = False
        return None


if __name__ == "__main__":
    rospy.init_node("buyos_location_publisher")
    node = BuyosPublisher()
    rospy.spin()


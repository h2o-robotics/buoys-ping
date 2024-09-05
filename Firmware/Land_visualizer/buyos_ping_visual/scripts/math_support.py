import numpy as np
from scipy.optimize import minimize

import geojson
from geojson import Feature, Polygon, FeatureCollection

# Constants
EARTH_RADIUS = 6371e3  # Earth's radius in meters

# Function to generate points for a circle
def geoJson_circle_creator(lat, lon, radius, num_points=360):
    points = []
    for i in range(num_points):
        angle = np.radians(i)
        dlat = (radius / EARTH_RADIUS) * np.cos(angle)
        dlon = (radius / (EARTH_RADIUS * np.cos(np.radians(lat)))) * np.sin(angle)
        points.append((lon + np.degrees(dlon), lat + np.degrees(dlat)))
    points.append(points[0])  # Close the polygon
    return points

# Function to convert lat/long to Cartesian coordinates
def latlon_to_cartesian(lat, lon, origin_lat, origin_lon):
    lat = np.radians(lat)
    lon = np.radians(lon)
    origin_lat = np.radians(origin_lat)
    origin_lon = np.radians(origin_lon)

    x = EARTH_RADIUS * (lon - origin_lon) * np.cos((lat + origin_lat) / 2)
    y = EARTH_RADIUS * (lat - origin_lat)
    return x, y

# Function to convert Cartesian coordinates to lat/long
def cartesian_to_latlon(x, y, origin_lat, origin_lon):
    origin_lat = np.radians(origin_lat)
    origin_lon = np.radians(origin_lon)

    lat = y / EARTH_RADIUS + origin_lat
    lon = x / (EARTH_RADIUS * np.cos((lat + origin_lat) / 2)) + origin_lon

    lat = np.degrees(lat)
    lon = np.degrees(lon)
    return lat, lon

def calculate_diver(buoy_data):

    d1, buoy1_lat, buoy1_lon = buoy_data[0]
    d2, buoy2_lat, buoy2_lon = buoy_data[1]
    d3, buoy3_lat, buoy3_lon = buoy_data[2]

    origin_lat, origin_lon = buoy1_lat, buoy1_lon

    buoy1_x, buoy1_y, buoy1_z = latlon_to_cartesian(buoy1_lat, buoy1_lon, origin_lat, origin_lon), 0
    buoy2_x, buoy2_y, buoy2_z = latlon_to_cartesian(buoy2_lat, buoy2_lon, origin_lat, origin_lon), 0
    buoy3_x, buoy3_y, buoy3_z = latlon_to_cartesian(buoy3_lat, buoy3_lon, origin_lat, origin_lon), 0

    buoy1 = np.array([buoy1_x, buoy1_y, buoy1_z])
    buoy2 = np.array([buoy2_x, buoy2_y, buoy2_z])
    buoy3 = np.array([buoy3_x, buoy3_y, buoy3_z])

    initial_guess = np.array([0, 0, 0])

    def objective_function(estimated_position):
        x, y, z = estimated_position
        estimated_distances = [
            np.sqrt((x - buoy1[0])**2 + (y - buoy1[1])**2 + (z - buoy1[2])**2),
            np.sqrt((x - buoy2[0])**2 + (y - buoy2[1])**2 + (z - buoy2[2])**2),
            np.sqrt((x - buoy3[0])**2 + (y - buoy3[1])**2 + (z - buoy3[2])**2)
        ]
        return np.sum((np.array(estimated_distances) - np.array([d1, d2, d3]))**2)

    max_depth = min(buoy1_z, buoy2_z, buoy3_z)
    bounds = [(-np.inf, np.inf), (-np.inf, np.inf), (-np.inf, max_depth)]

    result = minimize(objective_function, initial_guess, bounds=bounds)
    diver_position = result.x

    diver_lat, diver_lon = cartesian_to_latlon(diver_position[0], diver_position[1], origin_lat, origin_lon)
    diver_depth = diver_position[2]

    buoy1_latlon = cartesian_to_latlon(buoy1[0], buoy1[1], origin_lat, origin_lon)
    buoy2_latlon = cartesian_to_latlon(buoy2[0], buoy2[1], origin_lat, origin_lon)
    buoy3_latlon = cartesian_to_latlon(buoy3[0], buoy3[1], origin_lat, origin_lon)

    buoy1_circle = geoJson_circle_creator(buoy1_latlon[0], buoy1_latlon[1], d1)
    buoy2_circle = geoJson_circle_creator(buoy2_latlon[0], buoy2_latlon[1], d2)
    buoy3_circle = geoJson_circle_creator(buoy3_latlon[0], buoy3_latlon[1], d3)

    # GeoJSON features
    buoy1_feature = Feature(geometry=Polygon([buoy1_circle]), properties={
        "name": "Buoy 1",
        "style": {
            "color": "#FCFF00",
            "weight": 2,
            "fill": False
        }
    })
    buoy2_feature = Feature(geometry=Polygon([buoy2_circle]), properties={
        "name": "Buoy 2",
        "style": {
            "color": "#FF0000",
            "weight": 2,
            "fill": False
        }
    })
    buoy3_feature = Feature(geometry=Polygon([buoy3_circle]), properties={
        "name": "Buoy 3",
        "style": {
            "color": "#0000FF",
            "weight": 2,
            "fill": False
        }
    })
    feature_collection = FeatureCollection([buoy1_feature, buoy2_feature, buoy3_feature])
    
    return (geojson.dumps(feature_collection), diver_lat, diver_lon, diver_depth)
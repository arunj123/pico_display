# File: weather.py
"""
Handles fetching and parsing weather data from the free Open-Meteo API.
"""
import requests
from typing import Optional, Dict, Any
from datetime import datetime

def get_weather(lat: float, lon: float) -> Optional[Dict[str, Any]]:
    # Added humidity, windspeed, and daily sunrise/sunset to the API call
    url = (f"https://api.open-meteo.com/v1/forecast?latitude={lat}&longitude={lon}"
           "&current=temperature_2m,relative_humidity_2m,is_day,weather_code,wind_speed_10m"
           "&daily=sunrise,sunset&timezone=auto")
    try:
        response = requests.get(url, timeout=10)
        response.raise_for_status()
        data = response.json()
        
        current = data['current']
        daily = data['daily']
        weather_info = {
            'temperature': int(round(current['temperature_2m'])),
            'humidity': int(current['relative_humidity_2m']),
            'windspeed': int(round(current['wind_speed_10m'])),
            'description': _map_weather_code_to_description(current['weather_code']),
            'icon': _map_weather_code_to_icon(current['weather_code'], current.get('is_day', 1)),
            'sunrise': datetime.fromisoformat(daily['sunrise'][0]).strftime('%H:%M'),
            'sunset': datetime.fromisoformat(daily['sunset'][0]).strftime('%H:%M'),
        }
        
        print(f"Weather Updated: {weather_info['temperature']}°C, {weather_info['description']}, Wind: {weather_info['windspeed']}km/h")
        return weather_info
        
    except requests.exceptions.RequestException as e:
        print(f"Error fetching weather: {e}")
        return None

def _map_weather_code_to_description(code: int) -> str:
    # Simplified descriptions for display
    if code in [0, 1]: return "Clear Sky"
    if code in [2]: return "Partly Cloudy"
    if code in [3]: return "Overcast"
    if code in [45, 48]: return "Fog"
    if code in [51, 53, 55]: return "Drizzle"
    if code in [56, 57]: return "Freezing Drizzle"
    if code in [61, 80]: return "Light Rain"
    if code in [63, 81]: return "Rain"
    if code in [65, 82]: return "Heavy Rain"
    if code in [66, 67]: return "Freezing Rain"
    if code in [71, 85]: return "Light Snow"
    if code in [73]: return "Snow"
    if code in [75, 86]: return "Heavy Snow"
    if code in [77]: return "Snow Grains"
    if code in [95]: return "Thunderstorm"
    if code in [96, 99]: return "Thunderstorm Hail"
    return "Cloudy"

def _map_weather_code_to_icon(code: int, is_day: int) -> str:
    if code in [0, 1]: return "sun" if is_day else "moon"
    if code in [2]: return "sun_cloud" if is_day else "moon_cloud"
    if code in [3]: return "cloud"
    if code in [45, 48]: return "fog"
    if code in [51, 53, 55]: return "drizzle"
    if code in [56, 57, 66, 67]: return "freezing_rain"
    if code in [61, 80]: return "light_rain"
    if code in [63, 81]: return "rain"
    if code in [65, 82]: return "heavy_rain"
    if code in [71, 77, 85]: return "light_snow"
    if code in [73]: return "snow"
    if code in [75, 86]: return "heavy_snow"
    if code in [95]: return "storm"
    if code in [96, 99]: return "storm_hail"
    return "cloud"

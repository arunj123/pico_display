import os

DATA_PATH = "/mnt/data/weather_log.txt"

def save_data(data):
    try:
        # Remount RW
        os.system("mount -o remount,rw /mnt/data")
        
        with open(DATA_PATH, "a") as f:
            f.write(data + "\n")
            
        # Immediate sync to ensure data is on disk
        os.sync()
    finally:
        # Remount RO immediately to protect SD card
        os.system("mount -o remount,ro /mnt/data")
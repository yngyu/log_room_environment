from datetime import datetime
import time
import struct
import binascii
import json

from bluepy import btle
import influxdb
from schedule import every, repeat, run_pending

SENSOR_DATA_NUM = 4

def insert_into_db(sensor_data):
    point = [{
        "measurement": "sensor_data",
        "time": datetime.utcnow(),
        "fields": sensor_data
    }]
    db.write_points(point)


def get_ble_data(sensor_data):
    for i in range(SENSOR_DATA_NUM):
        sub_str = sensor_data[8*i:8*i + 8]
        if i == 0:
            temperature = struct.unpack("<f", binascii.unhexlify(sub_str))[0]
        elif i == 1:
            pressure = struct.unpack("<f", binascii.unhexlify(sub_str))[0]
        elif i == 2:
            humidity = struct.unpack("<f", binascii.unhexlify(sub_str))[0]
        else:
            co2_ppm = struct.unpack("<f", binascii.unhexlify(sub_str))[0]
    insert_into_db({
        "temperature": temperature,
        "pressure": pressure,
        "humidity": humidity,
        "co2_ppm": co2_ppm
    })


@repeat(every(1).seconds)
def scan_ble_advertise():
    scanner = btle.Scanner(0)
    devices = scanner.scan(0.1)
    for device in devices:
        if device.addr == BLE_ADDRESS:
            for advertise_data in device.getScanData():
                if advertise_data[0] == 255:
                    get_ble_data(advertise_data[2])

def main():
    with open("./local-value.json") as f:
        df = json.load(f)
        global BLE_ADDRESS, DB_HOST, DB_PORT, DB_NAME, db
        BLE_ADDRESS = df["BLE_ADDRESS"]
        DB_HOST     = df["DB_HOST"]
        DB_PORT     = df["DB_PORT"]
        DB_NAME     = df["DB_NAME"]

        db = influxdb.InfluxDBClient(
            host=DB_HOST,
            port=DB_PORT,
            database=DB_NAME
        )

    while True:
        run_pending()
        time.sleep(0.001)

if __name__ == "__main__":
    main()

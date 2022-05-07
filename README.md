# 部屋環境監視用のコード

### HW
* ESP32-WROVER-E
  * センサーの読み取り, ラズパイと BLE でセンサー値通信. ラズパイには ADC が無いので.
* Raspberry Pi Zero W
  * サーバー用. Grafana と InfluxDB が立っていて ESP32 と BLE で通信している.
  * OS: Raspbion 32 bit
* BME680
  * 気圧、温度、湿度センサー (ガスも測れるが使っていない).
* MG812
  * CO2 センサー

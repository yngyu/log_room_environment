# 部屋環境監視用のコード
## HW
* Seeed Studio XIAO ESP32C3
  * センサーの読み取り, Wifi につないで InfluxDB の API を叩く. 可視化は Grafana で
* BME680
  * 気圧、温度、湿度センサー (ガスも測れるが使っていない).
  * https://github.com/BoschSensortec/BME68x-Sensor-API のライブラリを使用
* MH-Z19C
  * CO2 センサー

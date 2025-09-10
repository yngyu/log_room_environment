mod mh_z19c;
mod sensors;

use linux_embedded_hal::{I2cdev, Serial};
use log::{error, info};
use reqwest::{
    Client,
    header::{ACCEPT, AUTHORIZATION, CONTENT_TYPE, HeaderMap},
};
use sensors::Sensors;
use std::{env::var, time::Duration};
use tokio::time::Instant;

#[tokio::main]
async fn main() {
    let mut sensors = init().await.expect("Failed to init sensors");
    let client = reqwest::Client::new();

    loop {
        let now = Instant::now();
        let result = run(&mut sensors, &client).await;
        match result {
            Ok(_) => info!("Successfully sent measurements"),
            Err(e) => {
                error!("Failed to get measurements: {}", e);
                panic!("Panicked with an error, {}", e);
            }
        }

        tokio::time::sleep_until(now + Duration::from_secs(1)).await;
    }
}

async fn init() -> Result<Sensors, Box<dyn std::error::Error>> {
    env_logger::init();

    let serial = Serial::open("/dev/ttyS0".to_string(), 9600)?;
    let i2c_bus = I2cdev::new("/dev/i2c-1")?;

    let mut sensors = Sensors::new(serial, i2c_bus);
    sensors.init()?;

    Ok(sensors)
}

async fn run(sensors: &mut Sensors, client: &Client) -> Result<(), Box<dyn std::error::Error>> {
    let token = var("INFLUXDB_TOKEN")?;
    let uri = var("INFLUXDB_URI")?;

    let measurements = sensors.measure().await;
    let mut headers = HeaderMap::new();
    headers.insert(AUTHORIZATION, token.parse()?);
    headers.insert(CONTENT_TYPE, "text/plain; charset=utf-8".parse()?);
    headers.insert(ACCEPT, "application/json".parse()?);

    let mut measurement_strs = Vec::new();
    if measurements.temperature_celsius.is_some() {
        measurement_strs.push(format!(
            "temperature={:.2}",
            measurements.temperature_celsius.unwrap()
        ));
    }
    if measurements.pressure_pa.is_some() {
        measurement_strs.push(format!("pressure={:.2}", measurements.pressure_pa.unwrap()));
    }
    if measurements.humidity_percent.is_some() {
        measurement_strs.push(format!(
            "humidity={:.2}",
            measurements.humidity_percent.unwrap()
        ));
    }
    if measurements.co2_ppm.is_some() {
        measurement_strs.push(format!("co2_ppm={:.2}", measurements.co2_ppm.unwrap()));
    }

    let body = format!(
        "{},sensor_id={} {}",
        "Desk",
        "RaspberryPiZeroW",
        measurement_strs.join(","),
    );

    // Does not panic on error, just logs it
    if let Err(e) = client.post(uri).headers(headers).body(body).send().await {
        error!("Failed to send measurements: {}", e);
    }

    Ok(())
}

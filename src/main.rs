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

    let measurements = sensors.measure().await?;
    let mut headers = HeaderMap::new();
    headers.insert(AUTHORIZATION, token.parse()?);
    headers.insert(CONTENT_TYPE, "text/plain; charset=utf-8".parse()?);
    headers.insert(ACCEPT, "application/json".parse()?);

    let body = format!(
        "{},sensor_id={} temperature={:.2},pressure={:.2},humidity={:.2},co2_ppm={:.2}",
        "Desk",
        "RaspberryPiZeroW",
        measurements.temperature_celsius,
        measurements.pressure_pa,
        measurements.humidity_percent,
        measurements.co2_ppm
    );

    client.post(uri).headers(headers).body(body).send().await?;

    Ok(())
}

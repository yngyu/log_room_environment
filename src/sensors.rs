use bme680::{
    Bme680, FieldData, IIRFilterSize, OversamplingSetting, PowerMode::ForcedMode, SettingsBuilder,
};
use linux_embedded_hal::{Delay, I2CError, I2cdev, Serial};

use crate::mh_z19c::MhZ19c;

pub struct Sensors {
    mh_z19c: MhZ19c,
    bme680: Bme680<I2cdev, Delay>,
}

pub struct Measurements {
    pub temperature_celsius: f32,
    pub pressure_pa: f32,
    pub humidity_percent: f32,
    pub co2_ppm: u16,
}

impl Sensors {
    pub fn new(serial: Serial, i2c: I2cdev) -> Self {
        let mut delay = Delay {};
        let bme680 = Bme680::init(i2c, &mut delay, bme680::I2CAddress::Secondary).unwrap();

        Sensors {
            mh_z19c: MhZ19c::new(serial),
            bme680,
        }
    }

    pub fn init(&mut self) -> Result<(), Box<dyn std::error::Error>> {
        let mut delay = Delay {};
        let settings = SettingsBuilder::new()
            .with_humidity_oversampling(OversamplingSetting::OS16x)
            .with_pressure_oversampling(OversamplingSetting::OS16x)
            .with_temperature_oversampling(OversamplingSetting::OS16x)
            .with_temperature_filter(IIRFilterSize::Size7)
            .with_run_gas(false)
            .build();

        let result = self.bme680.set_sensor_settings(&mut delay, settings);

        match result {
            Ok(_) => Ok(()),
            Err(_) => Err("Error has happend during initializing".into()),
        }
    }

    pub async fn measure(&mut self) -> Result<Measurements, Box<dyn std::error::Error>> {
        let bme680_data = self
            .measure_bme680()
            .expect("Error has happened during getting data");

        let co2_ppm = self.mh_z19c.read_co2().await?;

        let measurements = Measurements {
            temperature_celsius: bme680_data.temperature_celsius(),
            pressure_pa: bme680_data.pressure_hpa() * 100.0,
            humidity_percent: bme680_data.humidity_percent(),
            co2_ppm,
        };

        self.verify_data(&measurements)?;

        Ok(measurements)
    }

    fn measure_bme680(&mut self) -> Result<FieldData, bme680::Error<I2CError>> {
        let mut delay = Delay {};
        self.bme680.set_sensor_mode(&mut delay, ForcedMode)?;

        let (data, _) = self.bme680.get_sensor_data(&mut delay)?;

        Ok(data)
    }

    fn verify_data(&self, measurements: &Measurements) -> Result<(), Box<dyn std::error::Error>> {
        if measurements.temperature_celsius < -40.0 || 85.0 < measurements.temperature_celsius {
            return Err("The temperature data is invalid.".into());
        }
        if measurements.pressure_pa < 30000.0 || 110000.0 < measurements.pressure_pa {
            return Err("The air pressure data is invalid.".into());
        }
        if measurements.humidity_percent < 0.0 || 100.0 < measurements.humidity_percent {
            return Err("The humidity data is invalid.".into());
        }
        if measurements.co2_ppm < 300 || 10000 < measurements.co2_ppm {
            return Err("The co2 data is invalid.".into());
        }

        Ok(())
    }
}

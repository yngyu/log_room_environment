use bme680::{
    Bme680, FieldData, IIRFilterSize, OversamplingSetting, PowerMode::ForcedMode, SettingsBuilder,
};
use linux_embedded_hal::{Delay, I2CError, I2cdev, Serial};
use log::error;

use crate::mh_z19c::MhZ19c;

pub struct Sensors {
    mh_z19c: MhZ19c,
    bme680: Bme680<I2cdev, Delay>,
}

pub struct Measurements {
    pub temperature_celsius: Option<f32>,
    pub pressure_pa: Option<f32>,
    pub humidity_percent: Option<f32>,
    pub co2_ppm: Option<u16>,
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

    pub async fn measure(&mut self) -> Measurements {
        let bme680_data = self.measure_bme680();
        if bme680_data.is_err() {
            error!("{:?}", bme680_data.as_ref().unwrap_err());
        }
        let co2_ppm = self.mh_z19c.read_co2().await;
        if co2_ppm.is_err() {
            error!("{:?}", co2_ppm.as_ref().unwrap_err());
        }

        self.verify_data(&bme680_data, &co2_ppm)
    }

    fn measure_bme680(&mut self) -> Result<FieldData, bme680::Error<I2CError>> {
        let mut delay = Delay {};
        self.bme680.set_sensor_mode(&mut delay, ForcedMode)?;

        let (data, _) = self.bme680.get_sensor_data(&mut delay)?;

        Ok(data)
    }

    fn verify_data(
        &self,
        bme680_data: &Result<FieldData, bme680::Error<I2CError>>,
        co2_ppm: &Result<u16, Box<dyn std::error::Error>>,
    ) -> Measurements {
        let temperature_celsius = if bme680_data.is_ok() {
            let temp = bme680_data.as_ref().unwrap().temperature_celsius();
            if (-40.0..=85.0).contains(&temp) {
                Some(temp)
            } else {
                None
            }
        } else {
            None
        };

        let pressure_pa = if bme680_data.is_ok() {
            let pressure = bme680_data.as_ref().unwrap().pressure_hpa() * 100.0;
            if (30000.0..=110000.0).contains(&pressure) {
                Some(pressure)
            } else {
                None
            }
        } else {
            None
        };

        let humidity_percent = if bme680_data.is_ok() {
            let humidity = bme680_data.as_ref().unwrap().humidity_percent();
            if (0.0..=100.0).contains(&humidity) {
                Some(bme680_data.as_ref().unwrap().humidity_percent())
            } else {
                None
            }
        } else {
            None
        };

        let co2_ppm = if co2_ppm.is_ok() {
            let co2_ppm = *co2_ppm.as_ref().unwrap();
            if (300..=10000).contains(&co2_ppm) {
                Some(co2_ppm)
            } else {
                None
            }
        } else {
            None
        };

        Measurements {
            temperature_celsius,
            pressure_pa,
            humidity_percent,
            co2_ppm,
        }
    }
}

use linux_embedded_hal::Serial;
use std::{
    io::{Read, Write},
    time::Duration,
};
use tokio::time::sleep;

const READ_COMMAND: [u8; 9] = [0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79];

pub struct MhZ19c {
    serial: Serial,
}

impl MhZ19c {
    pub fn new(serial: Serial) -> Self {
        MhZ19c { serial }
    }

    pub async fn read_co2(&mut self) -> Result<u16, Box<dyn std::error::Error>> {
        self.serial.0.write_all(&READ_COMMAND)?;

        // Need to wait for a while
        sleep(Duration::from_millis(200)).await;
        let mut rec = Vec::new();
        // it returns error as timed out, so do not care
        let _ = self.serial.0.read_to_end(&mut rec);

        self.parse_co2_ppm(&rec)
    }

    fn parse_co2_ppm(&self, frame: &[u8]) -> Result<u16, Box<dyn std::error::Error>> {
        let sum = frame.iter().fold(0, |acc, x| acc + *x as u16) & 0xFF;

        if sum != 0xFF {
            return Err("Checksum is invalid".into());
        }

        let ppm = frame[2] as u16 * 256 + frame[3] as u16;

        Ok(ppm)
    }
}

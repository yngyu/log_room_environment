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
        sleep(Duration::from_millis(50)).await;
        let mut buffer = Vec::new();
        // it returns error as timed out, so do not care
        let _ = self.serial.0.read_to_end(&mut buffer);
        let ppm = buffer[2] as u16 * 256 + buffer[3] as u16;

        Ok(ppm)
    }
}

use linux_embedded_hal::Serial;
use std::{
    io::{Read, Write},
    time::Duration,
};
use tokio::time::sleep;

const READ_COMMAND: [u8; 9] = [0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79];
const BUFFER_SIZE: usize = 64;

pub struct MhZ19c {
    serial: Serial,
    ring_buffer: Vec<u8>,
    head: usize,
    tail: usize,
}

impl MhZ19c {
    pub fn new(serial: Serial) -> Self {
        MhZ19c {
            serial,
            ring_buffer: vec![0; BUFFER_SIZE],
            head: 0,
            tail: 0,
        }
    }

    pub async fn read_co2(&mut self) -> Result<u16, Box<dyn std::error::Error>> {
        self.serial.0.write_all(&READ_COMMAND)?;

        // Need to wait for a while
        sleep(Duration::from_millis(100)).await;
        let mut rec = Vec::new();
        // it returns error as timed out, so do not care
        let _ = self.serial.0.read_to_end(&mut rec);
        self.put_into_buffer(&rec)?;

        let mut latest_ppm = 0;
        loop {
            let frame_result = self.reas_from_buffer();
            if frame_result.is_err() {
                break;
            } else {
                let frame = frame_result?;
                latest_ppm = self.parse_co2_ppm(&frame)?;
            }
        }

        if latest_ppm == 0 {
            Err("Failed to read CO2".into())
        } else {
            Ok(latest_ppm)
        }
    }

    fn put_into_buffer(&mut self, rec: &[u8]) -> Result<(), Box<dyn std::error::Error>> {
        if self.tail + rec.len() > BUFFER_SIZE {
            let len = BUFFER_SIZE - self.tail;
            self.ring_buffer[self.tail..].copy_from_slice(&rec[..len]);
            self.ring_buffer[..(rec.len() - len)].copy_from_slice(&rec[len..]);
            self.tail = rec.len() - len;
        } else {
            self.ring_buffer[self.tail..self.tail + rec.len()].copy_from_slice(rec);
            self.tail += rec.len();
        }

        self.tail %= BUFFER_SIZE;

        Ok(())
    }

    fn reas_from_buffer(&mut self) -> Result<Vec<u8>, Box<dyn std::error::Error>> {
        while self.head != self.tail {
            if self.ring_buffer[self.head] == 0xFF
                && self.ring_buffer[(self.head + 1) % BUFFER_SIZE] == 0x86
            {
                if (self.tail + BUFFER_SIZE - self.head) % BUFFER_SIZE < 9 {
                    return Err("Data is not ready yet".into());
                } else if self.head + 9 <= BUFFER_SIZE {
                    let frame = self.ring_buffer[self.head..(self.head + 9)].to_vec();
                    self.head = (self.head + 9) % BUFFER_SIZE;

                    return Ok(frame);
                } else {
                    let mut frame = self.ring_buffer[self.head..].to_vec();
                    frame.extend_from_slice(&self.ring_buffer[..(self.head + 9) % BUFFER_SIZE]);
                    self.head = (self.head + 9) % BUFFER_SIZE;

                    return Ok(frame);
                }
            }
            self.head = (self.head + 1) % BUFFER_SIZE;
        }

        Err("No data in buffer".into())
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



use esp_idf_sys::EspError;
use esp_idf_sys::ESP_ERR_TIMEOUT;
use esp_idf_hal::uart;

#[repr(u8)]
pub enum Command {
    MoveSegment = 0x01,
    ClearSegments = 0x02,
    SpawnCookie = 0x03,
    CollisionCookie = 0x04,
    CollisionWall = 0x05,
    KeyPressed = 0x06,
}

pub fn read_char(uart: &mut uart::UartRxDriver) -> Result<char, EspError> {
    let mut buf = [0u8; 1];

    let read = uart.read(&mut buf, 10000)?;
    if read != buf.len() {
        return Err(EspError::from(ESP_ERR_TIMEOUT).unwrap());
    }
    Ok(buf[0] as char)
}

pub fn write_i16(data: &mut Vec<u8>, val: i16) {
    data.push((val & 0xFF) as u8);
    data.push((val >> 8) as u8);
}

pub fn send_packet(uart: &mut uart::UartTxDriver, cmd: Command, data: &Vec<u8>) -> Result<(), EspError> {
    let header: [u8; 4] = [
        0xFF, 0x80, data.len() as u8, cmd as u8
    ];

    uart.write(&header)?;
    uart.write(data.as_slice())?;
    Ok(())
}

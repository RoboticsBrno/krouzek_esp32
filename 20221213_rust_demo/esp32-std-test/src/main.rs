mod uart_utils;
mod timer;

use esp_idf_hal::gpio;
use esp_idf_sys as _; // If using the `binstart` feature of `esp-idf-sys`, always keep this module imported
use esp_idf_hal::prelude::*;
use esp_idf_hal::uart;
use esp_idf_sys::EspError;

use getrandom;

use timer::Timer;
use uart_utils::Command;
use uart_utils::send_packet;
use uart_utils::write_i16;

use std::thread::sleep;
use std::time::Duration;
use std::mem;


enum Direction {
    Left,
    Right,
    Up,
    Down,
}

#[derive(Copy, Clone)]
struct Pos{
    x: i16, y:i16
}

impl Pos {
    fn nudge(&mut self, direction: &Direction, delta: i16) {
        match direction {
            Direction::Left => self.x -= delta,
            Direction::Right => self.x += delta,
            Direction::Down => self.y += delta,
            Direction::Up => self.y -= delta,
        }
    }
}

#[derive(Copy, Clone)]
struct Rgb {
    r: u8, g: u8, b: u8
}

impl Rgb {
    fn random() -> Rgb {
        let mut buf = [0u8; 3];
        getrandom::getrandom(&mut buf).unwrap();
        Rgb {
            r: buf[0],
            g: buf[1],
            b: buf[2],
        }
    }

    fn black() -> Rgb {
        Rgb {
            r: 0,
            g: 0,
            b: 0,
        }
    }

    fn write(&self, data: &mut Vec<u8>) {
        data.push(self.r);
        data.push(self.g);
        data.push(self.b);
    }
}
#[derive(Copy, Clone)]
struct TailSegment {
    id: u8,
    color: Rgb,
    pos: Pos,
}

struct SnakeState<'a> {
    paused: bool,
    game_over: bool,
    direction: Direction,
    head: Pos,
    tail: Vec<TailSegment>,
    tail_id_counter: u8,

    cookie_color: Option<Rgb>,

    move_timer: Timer,
    uart: uart::UartTxDriver<'a>,
}

impl<'a> SnakeState<'a> {
    fn new(uart: uart::UartTxDriver) -> SnakeState {
        SnakeState {
            paused: true,
            game_over: false,
            head: Pos{ x:0, y:0 },
            tail: Vec::new(),
            tail_id_counter: 0,
            direction: Direction::Right,
            cookie_color: None,
            move_timer: Timer::new(Duration::from_millis(600)),
            uart: uart,
        }
    }

    fn handle_pkt(&mut self, uart: &mut uart::UartRxDriver, cmd_raw: u8) -> Result<(), EspError> {
        let cmd: Command = unsafe { ::std::mem::transmute(cmd_raw) };
        match Command::from(cmd) {
            Command::KeyPressed => {
                let c = uart_utils::read_char(uart)?;
                self.handle_key(c);
                Ok(())
            },
            Command::CollisionCookie => {
                self.add_tail_segment();
                self.send_cookie_spawn();
                Ok(())
            },
            Command::CollisionWall => {
                self.game_over = true;
                Ok(())
            },
            _ => {
                println!("Unknown command: {}", cmd_raw);
                Ok(())
            }
        }
    }

    fn handle_key(&mut self, key: char) {
        match key {
            'w' => self.direction = Direction::Up,
            'a' => self.direction = Direction::Left,
            's' => self.direction = Direction::Down,
            'd' => self.direction = Direction::Right,
            'p' => {
                self.paused = !self.paused;
                self.send_head_pos();
                if !self.paused && self.cookie_color.is_none() {
                    self.send_cookie_spawn();
                }
            },
            'r' => {
                match send_packet(&mut self.uart, Command::ClearSegments, &mut Vec::new()) {
                    Ok(_) => {},
                    Err(err) => println!("Failed to send ClearSegments packet: {}", err),
                }
                self.game_over = false;
                self.paused = false;
                self.send_head_pos();
                self.send_cookie_spawn();
            },
            c => {
                println!("Unhandled key pressed: {}", c)
            },
        }
    }

    fn update(&mut self, elapsed: Duration) {
        if self.paused || self.game_over || !self.move_timer.update(elapsed) {
            return
        }
        self.move_timer.reset();
        self.update_segment_positions();
    }

    fn send_head_pos(&mut self) {
        match self.send_segment_pos(0, self.head.clone(), Rgb::black()) {
            Ok(_) => {},
            Err(err) => println!("Failed to send packet: {}", err),
        }
    }

    fn update_segment_positions(&mut self) {
        const STEP_SIZE: i16 = 110;

        let mut prev_pos = self.head;

        self.head.nudge(&self.direction, STEP_SIZE);
        self.send_head_pos();

        prev_pos.x += 10;
        prev_pos.y += 10;

        let mut tail: Vec<TailSegment> = vec![];
        mem::swap(&mut tail, &mut self.tail);
        for seg in &mut tail {
            let segment_pos = seg.pos;
            seg.pos = prev_pos;
            prev_pos = segment_pos;

            match self.send_segment_pos(seg.id, seg.pos.clone(), seg.color.clone()) {
                Ok(_) => {},
                Err(err) => println!("Failed to send packet: {}", err),
            }
        }
        mem::swap(&mut tail, &mut self.tail);
    }

    fn send_segment_pos(&mut self, id: u8, pos: Pos, color: Rgb) -> Result<(), EspError> {
        let mut data: Vec<u8> = Vec::new();
        data.push(id);
        color.write(&mut data);
        write_i16(&mut data, pos.x);
        write_i16(&mut data, pos.y);
        send_packet(&mut self.uart, Command::MoveSegment, &data)?;
        Ok(())
    }

    fn send_cookie_spawn(&mut self) {
        let color = Rgb::random();
        let mut data: Vec<u8> = Vec::new();
        color.write(&mut data);

        match send_packet(&mut self.uart, Command::SpawnCookie, &data) {
            Ok(_) => {},
            Err(err) => println!("Failed to send cookie pos: {}", err),
        }

        self.cookie_color = Some(color);
    }

    fn add_tail_segment(&mut self) {
        let segmentPos = if self.tail.is_empty() {
            Pos { x: self.head.x + 10, y: self.head.y + 10, }
        } else {
            self.tail.last().unwrap().pos
        };

        self.tail_id_counter = self.tail_id_counter.wrapping_add(1);
        if self.tail_id_counter == 0 {
            self.tail_id_counter = self.tail_id_counter.wrapping_add(1);
        }

        let seg = TailSegment {
            id: self.tail_id_counter,
            color: self.cookie_color.unwrap(),
            pos: segmentPos,
        };
        self.tail.push(seg)
    }
}

fn main() {
    // It is necessary to call this function once. Otherwise some patches to the runtime
    // implemented by esp-idf-sys might not link properly. See https://github.com/esp-rs/esp-idf-template/issues/71
    esp_idf_sys::link_patches();

    println!("Hello world");

    let peripherals = Peripherals::take().unwrap();
    let pins = peripherals.pins;

    let config = uart::config::Config::default().baudrate(Hertz(115_200));

    let uart: uart::UartDriver = uart::UartDriver::new(
        peripherals.uart0,
        pins.gpio1,
        pins.gpio3,
        Option::<gpio::Gpio0>::None,
        Option::<gpio::Gpio1>::None,
        &config
    ).unwrap();

    let (uart_tx, mut uart_rx) = uart.split();

    let mut snake = SnakeState::new(uart_tx);

    let mut pkt_read = 0;
    let mut buf: [u8; 1] = [ 0 ];
    loop {
        while uart_rx.read(&mut buf, 0).unwrap() > 0 {
            match pkt_read {
                0 if buf[0] == 0xFF => pkt_read += 1,
                1 if buf[0] == 0x80 => pkt_read += 1,
                2 => pkt_read += 1,
                3 => {
                    match snake.handle_pkt(&mut uart_rx, buf[0]) {
                        Ok(_) => {},
                        Err(e) => println!("Failed to handle cmd {}: {}", buf[0], e),
                    }
                    pkt_read = 0;
                },
                _ => pkt_read = 0
            }
        }

        snake.update(Duration::from_millis(10));
        sleep(Duration::from_millis(10));
    }
}

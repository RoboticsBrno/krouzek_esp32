use esp_idf_sys::TickType_t;
use std::time::Duration;
use esp_idf_hal::delay::TickType;

pub struct Timer {
    original_ticks: TickType_t,
    remaining_ticks: TickType_t,
}

impl Timer {
    pub fn new(duration: Duration) -> Timer {
        let ticks = TickType::from(duration).0;
        Timer {
            original_ticks: ticks,
            remaining_ticks: ticks,
        }
    }

    pub fn update(&mut self, elapsed: Duration) -> bool {
        let elapsed_ticks = TickType::from(elapsed).0;
        if self.remaining_ticks == 0 {
            return false
        }

        if elapsed_ticks >= self.remaining_ticks {
            self.remaining_ticks = 0;
            return true
        }
        self.remaining_ticks -= elapsed_ticks;
        return false
    }

    pub fn reset(&mut self) {
        self.remaining_ticks = self.original_ticks
    }
}























fn test() {
    /*let r: &i32;

    {
        let x = 2;
        r = &x;
    }

    println!("number {}", r);*/


    let string1 = String::from("abcde");
    let longstr: &str;

    {
        let string2 = String::from("efg");

        longstr = longest(string1.as_str(), string2.as_str());
        println!("longest is {}", longstr)
    }
}

fn longest<'a>(a: &'a str, b: &'a str) -> &'a str {
    if a.len() > b.len() {
        a
    } else {
        b
    }
}

struct Foo<'a> {
    superText: &'a str
}

impl<'a> Foo<'a> {
    fn bar(&self) {
        
    }
}























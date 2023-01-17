use std::io;

/*
fn random_int() -> i32 {
    4
}

fn pow(x: i32) -> i32 {
    x * x
}

fn fib(n: i32) -> i32 {
    if n <= 1 {
        return n
    }
    
    fib(n - 1) + fib(n - 2)
}
 */

/*
fn get_string() -> (String, usize) {
    let x = String::from("aaa");
    let s = x.len();
    (x, s)
}
 
fn main() {


    let (my_str, my_str_len) = get_string();

    let tup = get_string();

    println!("{my_str} {} {}", tup.0, tup.1);




    let numbers = [ 4, 8, 15, 16, 23, 42 ];

    println!("Please enter an array index.");

    let mut index = String::new();

    io::stdin()
        .read_line(&mut index)
        .expect("Failed to read line");

    let index: usize = index
        .trim()
        .parse()
        .expect("Index entered was not a number");

    let element = numbers[index];

    println!("The value of the element at index {index} is: {element}");
} */

/*
fn pow(x: i32) -> i32 {
    x * x
}

fn print_string(val: &mut String) {
    val.push_str("aaa");
    println!("{}", val);
}


fn main() {

    {
        let x = 4;

        let y = pow(x);

        println!("{} {}", x, y);

        let mut my_str = String::from("Hello");

        print_string(&mut my_str);

        println!("{} {} {}", a, b, c);
    }

    

    let mut x = String::from("Hello");
    x.push_str(" world!");






    println!("{x}");
} */


/*
#[derive(Debug)]
struct Rectangle {
    width: u32,
    height: u32,
}

impl Rectangle {
    fn area(&self) -> u32 {
        self.width * self.height
    }

    fn scale(&mut self, ratio: f64) {
        self.width = (self.width as f64 * ratio) as u32;
        self.height = (self.height as f64 * ratio) as u32;
    }
}

fn main() {
    let mut rect1 = Rectangle {
        width: 30,
        height: 50,
    };

    rect1.scale(4.0);

    println!(
        "The area of the rectangle {:?} is {} square pixels.",
        rect1,
        rect1.area()
    );
}
*/

enum IpAddr {
    V4(u8, u8, u8, u8),
    V6(String),
}

fn test_ip(addr: &IpAddr) {
    match addr {
        IpAddr::V4(a, b, c, d) if *a == 0 =>
            println!("Address v4 {a}.{b}.{c}.{d}"),
        IpAddr::V4(a, b, c, d)  =>
            println!("Address v4 {a}.{b}.{c}.{d}"),
        IpAddr::V6(val) =>
            println!("Address v6 {val}"),
    }


    let x = 4
    match x {
        5 | 6 | 7 => ...
        _ => 
    }

    if let IpAddr::V4(a, b, c, d) = addr {
        println!("Address v4 {a}.{b}.{c}.{d}");
    }
}

fn main() {
    let home = IpAddr::V4(127, 0, 0, 1);
    let loopback = IpAddr::V6(String::from("::1"));

    test_ip(&home);
    test_ip(&loopback);
}

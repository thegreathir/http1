use reqwest::blocking::Client;
use std::{thread};
use std::time::{Instant};

fn main() {
    let n: i32 = std::env::args().nth(1).unwrap().parse().unwrap();
    thread::scope(|s| {
        let mut join_handles = Vec::new();
        const REQ_COUNT:i32 = 500000;
        for _t in 0..n {
            join_handles.push(s.spawn(|| {
                let client = Client::new();
                let start = Instant::now();
                for _i in 0..(REQ_COUNT / n) {
                    let resp = client.get("http://127.0.0.1:8000").send().unwrap();
                    if resp.status() != reqwest::StatusCode::OK {
                        println!("Bad status code");
                        break;
                    }
                }
                let duration = start.elapsed();
                duration
            }));
        }

        let max_duration: f64 = join_handles
            .into_iter()
            .map(|x| x.join().unwrap())
            .fold(0., |acc, x| acc.max(x.as_secs_f64()));

        println!("{}", f64::from(REQ_COUNT) / max_duration);
    });
}

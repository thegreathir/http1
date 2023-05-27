use reqwest::blocking::Client;
use std::thread;
use std::time::Instant;

fn main() {
    let number_of_threads: i32 = std::env::args().nth(1).unwrap().parse().unwrap();
    let req_count: i32 = std::env::args().nth(2).unwrap().parse().unwrap();
    let host: String = std::env::args().nth(3).unwrap().parse().unwrap();
    thread::scope(|s| {
        let mut join_handles = Vec::new();
        for _t in 0..number_of_threads {
            let host = host.clone();
            join_handles.push(s.spawn(move || {
                let client = Client::new();
                let start = Instant::now();
                for _i in 0..(req_count / number_of_threads) {
                    let resp = client.get(host.as_str()).send().unwrap();
                    if resp.status() != reqwest::StatusCode::OK {
                        println!("Bad status code");
                        break;
                    }
                }
                start.elapsed()
            }));
        }

        let max_duration: f64 = join_handles
            .into_iter()
            .map(|x| x.join().unwrap())
            .fold(0., |acc, x| acc.max(x.as_secs_f64()));

        println!("{}", f64::from(req_count) / max_duration);
    });
}

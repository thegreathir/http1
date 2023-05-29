# http1

Basic HTTP/1.1 implemetation in C++

## Task overview

- Run on Linux
- Do not use any third party library, i.e. use only Linux system API.
- Handle at least 10,000 concurrent connections.
- Serve at least 100,000 requests per second on a modern personal computer.
- Serve a simple one or two pages website for demonstration.
- Clarify what HTTP features are supported.

## Implementation

### TCP server architecture

The TCP server is implemented using [epoll](https://man7.org/linux/man-pages/man7/epoll.7.html) in edge-triggered mode. This mechanism is capable of handling a large number of concurrent connections (up to the kernel's file descriptor limit) within a single-threaded application.

### Write API
In a non-blocking environment, it's not possible to wait for tasks to complete. Therefore, the write API of the implemented TCP server includes a callback argument, which it will invoke once the write task is finished. This mechanism enables the writing of large data chunks.

### HTTP stream parser
TCP is a stream-based protocol, and thus there are no assumptions about the size of received data chunks. To parse HTTP/1.1 requests under these conditions, a Deterministic Finite Automaton (DFA) has been designed to parse both the request header and body.

#### Limitations
* The request body will be parsed only if the header contains a `content-length` field, implying that `Transfer-Encoding: chunked` is not supported.

#### Parser DFA
![dfa](docs/images/http_parser_dfa.jpg)

* `-` means anything else.
* Cr stands for [carriage return](https://en.wikipedia.org/wiki/Carriage_return) and Lf stands for [line feed](https://en.wikipedia.org/wiki/Line_feed).

## Tests

Tests for the HTTP parser and serializer can be found in the `test` folder. The HTTP stream parser has been tested against various edge cases.

## Demo
An example server will be run by executing following commands.
```bash
mkdir build
cd build
cmake ..
make
./example-server
```
Then "http://127.0.0.1:8000" is available in browsers.

## Benchmark
Benchmarks have been measured in two separated experiments.

### Total requests per second
For measuring server's maximum requests per second rate, A [Rust](https://www.rust-lang.org/)y tester program is implemented using [reqwest](https://github.com/seanmonstar/reqwest) library. The Rust language was chosen due to the availability of reqwest library and the blazing fast runtime performance.

#### Multi-threaded
The Rust client is multi-threaded and the number of threads is configurable.

#### Execution
After starting example server:
```bash
cd benchmark/rps
cargo build --release
cargo run --release 16 500000 http://127.0.0.1:8000
```
* 16 is the number of threads.
* 500000 is the number of total sending requests.
* Total requests per second will be printed in standard output.

#### Results

Following experiments are executed on a PC with [Intel® Core™ i5-10210U Processor](https://ark.intel.com/content/www/us/en/ark/products/195436/intel-core-i510210u-processor-6m-cache-up-to-4-20-ghz.html). Requests have been sent through **loopback interface**.

![rps](docs/images/rps_chart.png)
* `n` is number of client's threads

### Concurrent connections
Project [locust](https://locust.io/) is used for load-testing the HTTP server. The locust file is available in `benchmark/load`

#### Executions
```bash
cd benchmark/load
python3 -m venv venv
source venv/bin/activate
pip3 install -r requirements.txt
python3 -m locust -f locustfile.py
```
Then the address "http://0.0.0.0:8089" is available. For more information see [locust documentations](https://docs.locust.io/en/stable/). 

#### Results

All experiments are executed on two directly connected laptops with a LAN cable.

* Server: [Intel® Core™ i7-6500U Processor](https://www.intel.com/content/www/us/en/products/sku/88194/intel-core-i76500u-processor-4m-cache-up-to-3-10-ghz/specifications.html) with 16GB RAM
* Client: [Apple M1 chip](https://support.apple.com/kb/SP824?locale=en_US) with 8GB RAM

![users](docs/images/users10000.png)
![rps](docs/images/rps10000.png)
![response time](docs/images/rt10000.png)

Tests were executed using 10,000 connections, requiring an increase in the kernel's open file descriptors limit.

```bash
ulimit -n 16384
```

#### Nginx comparison
Nginx with `worker_processes = 1`:
![nginx1](docs/images/nginx1_rps.png)

Nginx with `worker_processes = 4`:
![nginx4](docs/images/nginx4_rps.png)

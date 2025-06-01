# Lightweight HTTP Server (C++)

A simple, multithreaded HTTP server implemented in C++20 using a custom thread pool. Includes a Python load tester for benchmarking.

## Features

- Handles HTTP GET requests on multiple endpoints
- Returns HTML or JSON responses depending on the endpoint
- Uses a thread pool for concurrent request handling
- Graceful error handling
- Simple and portable (no external dependencies except standard C++ libraries)
- Includes a Python load tester (`load_tester.py`) for benchmarking

## Endpoints

- `/` - Root page (HTML)
- `/info` - Info page (JSON, includes server time)
- `/res1`, `/res2`, `/res3` - Example HTML resources
- Any other path returns a 404 Not Found

## Building

Requires a C++20 compiler (e.g., g++ or clang++) and pthreads.


## Benchmarking
```bash
python load_tester.py --url http://localhost:8080/ --concurrency 10 --requests 1000
```

This scripts requires `requests` module.
Command-line Arguments:

- `--url <URL>`: The base URL of your server (e.g., http://localhost:8080/, http://localhost:8080/info). (Required)

- `--concurrency <NUMBER>`: The number of concurrent threads (simulated users). Default is 8.

- `--requests <NUMBER>`: The total number of requests to send. Default is 100.

- `--timeout <SECONDS>:` The timeout for each individual HTTP request in seconds. Default is 5.0.

## License
MIT license (check the `LICENSE` file)

## Author
Maksym Nosal 
- [LinkedIn](https://www.linkedin.com/in/max-nosal-89017b285/)
- [Github](https://github.com/MaxUnlimited101/)

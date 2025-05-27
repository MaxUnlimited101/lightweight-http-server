#!/usr/bin/env python3
import requests
import time
import argparse
from concurrent.futures import ThreadPoolExecutor, as_completed
from collections import defaultdict

def send_request(url, timeout):
    """Sends a single GET request and records its outcome and duration."""
    start_time = time.perf_counter()
    status_code = None
    error_type = None
    try:
        response = requests.get(url, timeout=timeout)
        status_code = response.status_code
        if not (200 <= status_code < 300):
            error_type = f"HTTP_{status_code}"
    except requests.exceptions.Timeout:
        error_type = "Timeout"
    except requests.exceptions.ConnectionError:
        error_type = "ConnectionError"
    except requests.exceptions.RequestException as e:
        error_type = f"RequestException_{type(e).__name__}"
    except Exception as e:
        error_type = f"UnknownError_{type(e).__name__}"
    finally:
        end_time = time.perf_counter()
        duration = (end_time - start_time) * 1000 # Convert to milliseconds

    return {
        "duration": duration,
        "status_code": status_code,
        "error_type": error_type,
        "success": error_type is None and status_code is not None and (200 <= status_code < 300)
    }


def main():
    parser = argparse.ArgumentParser(description="Simple HTTP Load Tester")
    parser.add_argument("--url", type=str, required=True,
                        help="The URL to load test (e.g., http://localhost:8080/)")
    parser.add_argument("--concurrency", type=int, default=8,
                        help="Number of concurrent users (threads)")
    parser.add_argument("--requests", type=int, default=100,
                        help="Total number of requests to send")
    parser.add_argument("--timeout", type=float, default=5.0,
                        help="Timeout for each request in seconds")

    args = parser.parse_args()

    print(f"Starting load test on {args.url}")
    print(f"Concurrency: {args.concurrency} threads")
    print(f"Total Requests: {args.requests}")
    print(f"Request Timeout: {args.timeout}s")
    print("-" * 30)

    start_test_time = time.perf_counter()

    durations = []
    successful_requests = 0
    failed_requests = 0
    error_counts = defaultdict(int)

    with ThreadPoolExecutor(max_workers=args.concurrency) as executor:
        futures = [executor.submit(send_request, args.url, args.timeout) for _ in range(args.requests)]

        for i, future in enumerate(as_completed(futures)):
            result = future.result()
            durations.append(result["duration"])
            if result["success"]:
                successful_requests += 1
            else:
                failed_requests += 1
                if result["error_type"]:
                    error_counts[result["error_type"]] += 1
                else:
                    error_counts["UnknownFailure"] += 1

            # Optional: Print progress
            if (i + 1) % (args.requests // 10 or 1) == 0 or (i + 1) == args.requests:
                print(f"Progress: {i + 1}/{args.requests} requests completed.")

    end_test_time = time.perf_counter()
    total_test_duration = end_test_time - start_test_time

    print("-" * 30)
    print("Load Test Results:")
    print(f"Total Requests Sent: {args.requests}")
    print(f"Total Requests Completed: {successful_requests + failed_requests}")
    print(f"Successful Requests: {successful_requests}")
    print(f"Failed Requests: {failed_requests}")
    print(f"Success Rate: {successful_requests / (successful_requests + failed_requests) * 100:.2f}%" if (successful_requests + failed_requests) > 0 else "N/A")
    print(f"Total Test Duration: {total_test_duration:.2f} seconds")

    if total_test_duration > 0:
        rps = (successful_requests + failed_requests) / total_test_duration
        print(f"Requests Per Second (RPS): {rps:.2f}")
    else:
        print("Requests Per Second (RPS): N/A (duration too short)")

    if durations:
        print(f"Average Response Time: {sum(durations) / len(durations):.2f} ms")
        print(f"Min Response Time: {min(durations):.2f} ms")
        print(f"Max Response Time: {max(durations):.2f} ms")
    else:
        print("No response time data available.")

    if error_counts:
        print("\nError Breakdown:")
        for error_type, count in error_counts.items():
            print(f"- {error_type}: {count}")
    else:
        print("\nNo errors recorded.")

if __name__ == "__main__":
    main()

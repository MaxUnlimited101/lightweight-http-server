#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <thread>
#include <chrono>
#include <ctime>

#include "threadpool.hpp"

const int PORT = 8080;
const int BUFFER_SIZE = 4096;
const int MAX_QUEUE_SIZE = 1000;

// Emulate some important data that the server will serve
static const std::unordered_map<std::string, std::string> some_important_data = {
    {"/", "<h1>Hello from C++!</h1><p>This is the root page.</p>"},
    {"/info", "{\"message\": \"This is some info!\", \"version\": \"1.0\"}"},
    {"/res1", "<h1>Resource 1</h1><p>This is the first resource.</p>"},
    {"/res2", "<h1>Resource 2</h1><p>This is the second resource.</p>"},
    {"/res3", "<h1>Resource 3</h1><p>This is the third resource.</p>"},
    {"/notfound", "<h1>404 Not Found</h1><p>The requested resource was not found.</p>"}
};

static const std::unordered_map<std::string, std::string> some_important_data_content_type = {
    {"/", "text/html"},
    {"/info", "application/json"},
    {"/res1", "text/html"},
    {"/res2", "text/html"},
    {"/res3", "text/html"},
    {"/notfound", "text/html"}
};

// Function to get current time for responses
std::string get_current_time_gmt()
{
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm *tm_gmt = std::gmtime(&now_c); // Use gmtime for GMT/UTC
    char buf[100];
    // Format according to HTTP date format (e.g., "Mon, 27 Jul 2009 12:28:53 GMT")
    std::strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S GMT", tm_gmt);
    return std::string(buf);
}

// Function to handle the URI and generate a response
void handle_uri(const std::string& uri, std::string &response_body, std::string &content_type, int &status_code, std::string &status_message)
{
    if (uri == "/")
    {
        response_body = "<h1>Hello from C++ Thread Pool Server!</h1><p>This is the root page.</p>";
        content_type = "text/html";
        status_code = 200;
        status_message = "OK";
    }
    else if (uri == "/info")
    {
        response_body = "{\"message\": \"This is info from C++ thread pool server!\", \"version\": \"1.0\", \"time\": \"" + get_current_time_gmt() + "\"}";
        content_type = "application/json";
        status_code = 200;
        status_message = "OK";
    }
    else
    {
        response_body = "<h1>404 Not Found</h1><p>The requested resource was not found.</p>";
        content_type = "text/html";
        status_code = 404;
        status_message = "Not Found";
    }
    auto new_uri = some_important_data.contains(uri) ? uri : "/notfound"; 
    response_body = some_important_data.at(new_uri);
    content_type = some_important_data_content_type.at(new_uri);
    status_code = uri != "/notfound" ? 200 : 404;
    status_message = uri != "/notfound" ? "OK" : "Not Found; ";
}

// Function to handle a single client request (this will be a task for the thread pool)
void handle_client_request(int client_socket)
{
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);

    // Read the client's request
    ssize_t bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
    if (bytes_received < 0)
    {
        std::cerr << "Error reading from socket" << std::endl;
        close(client_socket);
        return;
    }
    buffer[bytes_received] = '\0'; // Null-terminate the received data
    // std::cout << "Received request:\n" << buffer << std::endl; // Be careful with logging volume

    std::string request_str(buffer);
    std::string response_body;
    std::string content_type;
    int status_code;
    std::string status_message;

    // Very basic GET request parsing
    if (request_str.rfind("GET /", 0) == 0)
    {
        std::string uri;
        size_t uri_start = request_str.find(" ") + 1;
        size_t uri_end = request_str.find(" ", uri_start);
        if (uri_start != std::string::npos && uri_end != std::string::npos)
        {
            uri = request_str.substr(uri_start, uri_end - uri_start);
        }

        handle_uri(uri, response_body, content_type, status_code, status_message);

        std::string http_response =
            "HTTP/1.1 " + std::to_string(status_code) + " " + status_message + "\r\n"
                                                                               "Content-Type: " +
            content_type + "\r\n"
                           "Content-Length: " +
            std::to_string(response_body.length()) + "\r\n"
                                                     "Connection: close\r\n"
                                                     "Date: " +
            get_current_time_gmt() + "\r\n"
                                     "\r\n" +
            response_body;

        send(client_socket, http_response.c_str(), http_response.length(), 0);
    }
    else
    {
        std::string bad_request_response =
            "HTTP/1.1 400 Bad Request\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 17\r\n"
            "Connection: close\r\n"
            "Date: " +
            get_current_time_gmt() + "\r\n"
                                     "\r\n"
                                     "400 Bad Request";
        send(client_socket, bad_request_response.c_str(), bad_request_response.length(), 0);
    }

    close(client_socket); // Close the client connection
}

int main()
{
    int server_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // Create socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Optional: Set socket options to reuse address (avoids "Address already in use" after restart)
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(opt)))
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // Set up address structure
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind the socket to the specified IP and port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_fd, 10) < 0)
    { // Max 10 pending connections in the OS queue
        perror("listen");
        exit(EXIT_FAILURE);
    }

    std::cout << "Server listening on port " << PORT << std::endl;

    // Initialize the thread pool
    ThreadPool pool(0);

    while (true)
    {
        int client_socket;
        if ((client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
        {
            perror("accept");
            // If accept fails, it might be due to a transient error, continue listening
            // For a robust server, you might add a sleep here or error specific handling
            continue;
        }

        // Enqueue the client handling task to the thread pool
        // Check if queue is not full before enqueuing to prevent unbounded growth
        // This basic ThreadPool doesn't directly expose queue size, you'd need to add that.
        // For now, we'll just enqueue. A full queue would indicate backpressure.
        try
        {
            pool.enqueue(handle_client_request, client_socket);
        }
        catch (const std::runtime_error &e)
        {
            std::cerr << "Failed to enqueue task: " << e.what() << ". Closing client socket." << std::endl;
            close(client_socket); // If enqueue fails, close the socket
        }
    }

    // This part is generally not reached in a continuous server loop
    // unless you add signal handling for graceful shutdown.
    // When main exits, the pool's destructor will be called and join threads.
    close(server_fd);
    return 0;
}
#ifndef MESSAGE_QUEUE_H
#define MESSAGE_QUEUE_H

#include <string>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <errno.h>
#include "Update.h"

#define MAX_MSG_SIZE 8192
#define MAX_MSGS 10

class MessageQueue {
private:
    std::string socket_path;
    int sockfd;
    bool is_owner;
    struct sockaddr_un server_addr;

public:
    MessageQueue() : sockfd(-1), is_owner(false) {
        memset(&server_addr, 0, sizeof(server_addr));
    }

    bool create(const std::string& name) {
        socket_path = "/tmp" + name + ".sock";
        is_owner = true;

        // Close if already open
        if (sockfd != -1) {
            ::close(sockfd);
        }

        // Remove existing socket file
        ::unlink(socket_path.c_str());

        // Create socket
        sockfd = socket(AF_UNIX, SOCK_DGRAM, 0);
        if (sockfd == -1) {
            std::cerr << "Failed to create socket: " << strerror(errno) << "\n";
            return false;
        }

        // Set non-blocking
        int flags = fcntl(sockfd, F_GETFL, 0);
        fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

        // Bind socket
        server_addr.sun_family = AF_UNIX;
        strncpy(server_addr.sun_path, socket_path.c_str(), sizeof(server_addr.sun_path) - 1);

        if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
            std::cerr << "Failed to bind socket: " << socket_path 
                      << " Error: " << strerror(errno) << "\n";
            ::close(sockfd);
            return false;
        }

        std::cout << "Message queue created: " << socket_path << "\n";
        return true;
    }

    bool open(const std::string& name) {
        socket_path = "/tmp" + name + ".sock";
        is_owner = false;

        // Create socket for sending
        if (sockfd == -1) {
            sockfd = socket(AF_UNIX, SOCK_DGRAM, 0);
            if (sockfd == -1) {
                return false;
            }

            // Set non-blocking
            int flags = fcntl(sockfd, F_GETFL, 0);
            fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
        }

        // Check if target socket exists
        struct stat st;
        if (stat(socket_path.c_str(), &st) != 0) {
            return false;
        }

        return true;
    }

    bool send(const Update& update) {
        std::string data = update.serialize();
        
        if (data.length() >= MAX_MSG_SIZE) {
            std::cerr << "Message too large\n";
            return false;
        }

        struct sockaddr_un dest_addr;
        memset(&dest_addr, 0, sizeof(dest_addr));
        dest_addr.sun_family = AF_UNIX;
        strncpy(dest_addr.sun_path, socket_path.c_str(), sizeof(dest_addr.sun_path) - 1);

        ssize_t sent = sendto(sockfd, data.c_str(), data.length(), 0,
                              (struct sockaddr*)&dest_addr, sizeof(dest_addr));

        if (sent == -1) {
            if (errno != EAGAIN && errno != EWOULDBLOCK && errno != ENOENT) {
                // std::cerr << "Failed to send message: " << strerror(errno) << "\n";
            }
            return false;
        }

        return true;
    }

    bool receive(Update& update) {
        char buffer[MAX_MSG_SIZE];
        ssize_t bytes_read = recvfrom(sockfd, buffer, MAX_MSG_SIZE, 0, nullptr, nullptr);

        if (bytes_read == -1) {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                // std::cerr << "Failed to receive message: " << strerror(errno) << "\n";
            }
            return false;
        }

        buffer[bytes_read] = '\0';
        update = Update::deserialize(std::string(buffer));
        return true;
    }

    void close() {
        if (sockfd != -1) {
            ::close(sockfd);
            sockfd = -1;
        }
    }

    void unlink() {
        if (is_owner && !socket_path.empty()) {
            ::unlink(socket_path.c_str());
        }
    }

    ~MessageQueue() {
        close();
    }
};

#endif // MESSAGE_QUEUE_H

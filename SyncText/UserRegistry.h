#ifndef USER_REGISTRY_H
#define USER_REGISTRY_H

#include <string>
#include <vector>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <atomic>

#define MAX_USERS 5
#define USER_ID_LEN 32
#define QUEUE_NAME_LEN 64
#define SHM_NAME "/synctext_registry"

struct UserInfo {
    char user_id[USER_ID_LEN];
    char queue_name[QUEUE_NAME_LEN];
    std::atomic<int> active;
    
    UserInfo() {
        memset(user_id, 0, USER_ID_LEN);
        memset(queue_name, 0, QUEUE_NAME_LEN);
        active.store(0);
    }
};

struct SharedRegistry {
    UserInfo users[MAX_USERS];
    std::atomic<int> user_count;
    std::atomic<int> global_update_count;  // Global counter for all updates
    
    SharedRegistry() {
        user_count.store(0);
        global_update_count.store(0);
    }
};

class UserRegistry {
private:
    int shm_fd;
    SharedRegistry* registry;
    std::string my_user_id;
    int my_index;

public:
    UserRegistry() : shm_fd(-1), registry(nullptr), my_index(-1) {}

    bool initialize() {
        // Create or open shared memory
        shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
        if (shm_fd == -1) {
            return false;
        }

        // Set size
        ftruncate(shm_fd, sizeof(SharedRegistry));

        // Map to memory
        registry = static_cast<SharedRegistry*>(
            mmap(nullptr, sizeof(SharedRegistry), PROT_READ | PROT_WRITE, 
                 MAP_SHARED, shm_fd, 0));

        if (registry == MAP_FAILED) {
            return false;
        }

        return true;
    }

    bool registerUser(const std::string& user_id, const std::string& queue_name) {
        if (!registry) return false;

        my_user_id = user_id;

        // Find empty slot
        for (int i = 0; i < MAX_USERS; i++) {
            int expected = 0;
            if (registry->users[i].active.compare_exchange_strong(expected, 1)) {
                strncpy(registry->users[i].user_id, user_id.c_str(), USER_ID_LEN - 1);
                strncpy(registry->users[i].queue_name, queue_name.c_str(), QUEUE_NAME_LEN - 1);
                my_index = i;
                registry->user_count.fetch_add(1);
                return true;
            }
        }

        return false; // No empty slot
    }

    void unregisterUser() {
        if (my_index >= 0 && registry) {
            registry->users[my_index].active.store(0);
            registry->user_count.fetch_sub(1);
        }
    }

    std::vector<std::pair<std::string, std::string>> getActiveUsers() {
        std::vector<std::pair<std::string, std::string>> users;
        
        if (!registry) return users;

        for (int i = 0; i < MAX_USERS; i++) {
            if (registry->users[i].active.load() == 1) {
                std::string uid(registry->users[i].user_id);
                std::string qname(registry->users[i].queue_name);
                if (!uid.empty() && uid != my_user_id) {
                    users.push_back({uid, qname});
                }
            }
        }

        return users;
    }

    int getUserCount() {
        return registry ? registry->user_count.load() : 0;
    }

    int getGlobalUpdateCount() {
        return registry ? registry->global_update_count.load() : 0;
    }

    int incrementGlobalUpdateCount(int amount = 1) {
        return registry ? registry->global_update_count.fetch_add(amount) : 0;
    }

    void resetGlobalUpdateCount() {
        if (registry) {
            registry->global_update_count.store(0);
        }
    }

    ~UserRegistry() {
        if (registry) {
            munmap(registry, sizeof(SharedRegistry));
        }
        if (shm_fd != -1) {
            close(shm_fd);
        }
    }

    static void cleanup() {
        shm_unlink(SHM_NAME);
    }
};

#endif // USER_REGISTRY_H

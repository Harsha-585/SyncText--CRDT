#!/bin/bash
# Cleanup script for SyncText editor

echo "🧹 Cleaning up SyncText resources..."

# Remove socket files
echo "Removing socket files..."
rm -f /tmp/queue_*.sock

# Remove user documents
echo "Removing user documents..."
rm -f user_*_doc.txt

# Clear shared memory (macOS uses /dev/shm or similar)
echo "Clearing shared memory..."
# On macOS, shared memory objects are in a special namespace
# We'll compile and run a small cleanup program

cat > cleanup_shm.cpp << 'EOF'
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>

int main() {
    // Unlink the shared memory
    if (shm_unlink("/synctext_registry") == 0) {
        std::cout << "✅ Shared memory cleared: /synctext_registry\n";
    } else {
        std::cout << "ℹ️  No shared memory to clear (or already cleaned)\n";
    }
    return 0;
}
EOF

g++ -std=c++17 cleanup_shm.cpp -o cleanup_shm
./cleanup_shm
rm -f cleanup_shm.cpp cleanup_shm

echo ""
echo "✅ Cleanup complete! Ready for fresh start."
echo "   - Socket files removed"
echo "   - User documents removed"
echo "   - Shared memory cleared"

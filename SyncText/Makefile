CXX = g++
CXXFLAGS = -std=c++17 -pthread -Wall -Wextra -O2
LDFLAGS = -pthread

TARGET = editor
SOURCES = editor.cpp
HEADERS = Update.h UserRegistry.h Document.h CRDTMerger.h MessageQueue.h

all: $(TARGET)

$(TARGET): $(SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(TARGET) $(LDFLAGS)

clean:
	rm -f $(TARGET)
	rm -f *_doc.txt
	rm -f /tmp/queue_*.sock
	rm -f /dev/shm/synctext_registry

test: $(TARGET)
	@echo "Building SyncText Editor..."
	@echo "Run with: ./editor <user_id>"
	@echo "Example: ./editor user_1"

.PHONY: all clean test

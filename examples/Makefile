# Makefile for HTTP Server Examples
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -fsanitize=address -g
# INCLUDES = -I../includes -I../libs/socket-lib/includes
LIBDIR = -L../build -L../build/libs/socket-lib
LIBS = -lhttp_server -lsocket_lib -pthread

# Source files
CALLBACK_SRC = callback_based_server.cpp
INHERITANCE_SRC = inheritance_based_server.cpp

# Executables
CALLBACK_BIN = callback_server
INHERITANCE_BIN = inheritance_server

.PHONY: all clean callback inheritance run-callback run-inheritance help

# Default target
all: callback inheritance

# Build callback-based server
callback: $(CALLBACK_BIN)

$(CALLBACK_BIN): $(CALLBACK_SRC)
	@echo "🔨 Building callback-based server..."
	$(CXX) $(CXXFLAGS)  $(LIBDIR) -o $@ $< $(LIBS)
	@echo "✅ Callback-based server built successfully!"

# Build inheritance-based server
inheritance: $(INHERITANCE_BIN)

$(INHERITANCE_BIN): $(INHERITANCE_SRC)
	@echo "🔨 Building inheritance-based server..."
	$(CXX) $(CXXFLAGS)  $(LIBDIR) -o $@ $< $(LIBS)
	@echo "✅ Inheritance-based server built successfully!"

# Run callback-based server
run-callback: $(CALLBACK_BIN)
	@echo "🚀 Starting callback-based server on http://localhost:8080"
	@echo "   Press Ctrl+C to stop"
	./$(CALLBACK_BIN)

# Run inheritance-based server
run-inheritance: $(INHERITANCE_BIN)
	@echo "🚀 Starting inheritance-based server on http://localhost:8080"
	@echo "   Press Ctrl+C to stop"
	./$(INHERITANCE_BIN)

# Clean build artifacts
clean:
	@echo "🧹 Cleaning build artifacts..."
	rm -f $(CALLBACK_BIN) $(INHERITANCE_BIN)
	@echo "✅ Clean complete!"

# Help target
help:
	@echo "🛠️  HTTP Server Examples Makefile"
	@echo ""
	@echo "Available targets:"
	@echo "  all              - Build both servers"
	@echo "  callback         - Build callback-based server"
	@echo "  inheritance      - Build inheritance-based server"
	@echo "  run-callback     - Build and run callback-based server"
	@echo "  run-inheritance  - Build and run inheritance-based server"
	@echo "  clean            - Remove built executables"
	@echo "  help             - Show this help message"
	@echo ""
	@echo "🚀 Quick start:"
	@echo "  make run-callback     # Run callback-based example"
	@echo "  make run-inheritance  # Run inheritance-based example"
	@echo ""
	@echo "📋 Prerequisites:"
	@echo "  - Main library must be built (run '../run.sh' from project root)"
	@echo "  - GCC with C++17 support"
	@echo "  - pthread library"

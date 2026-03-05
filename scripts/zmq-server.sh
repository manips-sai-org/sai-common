#!/bin/bash

# Configuration
# Change this path if you move the binary somewhere else (e.g., /usr/local/bin/mock_zmq_server)
SERVER_BIN="../build/examples/06-zmq_communication/zmq_client"
LOG_FILE="zmq_server.log"
PID_FILE="zmq_server.pid"

# Command routing
case "$1" in
    start)
        if [ ! -f "$SERVER_BIN" ]; then
            echo "Error: Executable not found at $SERVER_BIN"
            echo "Compile it first using: g++ -std=c++14 MockZmqServer.cpp -o mock_zmq_server -lzmq"
            exit 1
        fi

        if [ -f "$PID_FILE" ]; then
            echo "ZMQ Server is already running (PID: $(cat $PID_FILE))."
            exit 1
        fi

        echo "Starting ZMQ Server..."
        # Run in the background and redirect output
        $SERVER_BIN > "$LOG_FILE" 2>&1 &
        
        # Save the process ID
        PID=$!
        echo $PID > "$PID_FILE"
        
        echo "Server started in the background (PID: $PID)."
        echo "Logs are being written to $LOG_FILE."
        ;;
        
    stop)
        if [ ! -f "$PID_FILE" ]; then
            echo "No PID file found. Is the server running?"
            exit 1
        fi
        
        PID=$(cat "$PID_FILE")
        echo "Stopping ZMQ Server (PID: $PID)..."
        kill $PID
        
        # Clean up the PID file
        rm "$PID_FILE"
        echo "Server stopped."
        ;;
        
    status)
        if [ -f "$PID_FILE" ]; then
            PID=$(cat "$PID_FILE")
            # Check if the process is actually running
            if ps -p $PID > /dev/null; then
                echo "ZMQ Server is running (PID: $PID)."
            else
                echo "ZMQ Server is NOT running, but a stale PID file was found. Cleaning up..."
                rm "$PID_FILE"
            fi
        else
            echo "ZMQ Server is NOT running."
        fi
        ;;
        
    *)
        echo "Usage: $0 {start|stop|status}"
        exit 1
        ;;
esac
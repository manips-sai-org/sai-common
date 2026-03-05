#include <zmq.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>

int main() {
    // 1. Initialize the ZeroMQ context
    zmq::context_t context(1);
    
    // 2. Create a ROUTER socket for robust multi-client handling
    zmq::socket_t socket(context, zmq::socket_type::router);
    
    const std::string address = "tcp://*:5555";
    socket.bind(address);
    std::cout << "ZMQ ROUTER Server listening on " << address << std::endl;

    // 3. Our in-memory Key-Value store
    std::unordered_map<std::string, std::string> kv_store;

    while (true) {
        std::vector<std::string> request;
        
        // --- RECEIVE PHASE ---
        
        // A. In a ROUTER socket communicating with a REQ client, 
        // the first frame is ALWAYS the client's routing Identity.
        zmq::message_t identity_msg;
        auto res = socket.recv(identity_msg, zmq::recv_flags::none);
        if (!res) continue;
        std::string client_id(static_cast<char*>(identity_msg.data()), identity_msg.size());

        // B. The second frame is an empty delimiter frame (inserted automatically by the REQ client)
        zmq::message_t empty_msg;
        socket.recv(empty_msg, zmq::recv_flags::none);

        // C. Now receive the actual multi-part command sent by the user
        while (true) {
            zmq::message_t msg;
            socket.recv(msg, zmq::recv_flags::none);
            request.push_back(std::string(static_cast<char*>(msg.data()), msg.size()));
            if (!msg.more()) {
                break; // No more parts in this message
            }
        }

        if (request.empty()) continue;

        // --- PROCESSING PHASE ---
        
        std::vector<std::string> reply;
        std::string command = request[0];

        if (command == "PING") {
            reply.push_back("PONG");
        } 
        else if (command == "GET" && request.size() == 2) {
            auto it = kv_store.find(request[1]);
            if (it != kv_store.end()) {
                reply.push_back(it->second);
            } else {
                reply.push_back("NIL");
            }
        } 
        else if (command == "SET" && request.size() == 3) {
            kv_store[request[1]] = request[2];
            reply.push_back("OK");
        } 
        else if (command == "DEL" && request.size() == 2) {
            reply.push_back(kv_store.erase(request[1]) ? "1" : "0");
        } 
        else if (command == "EXISTS" && request.size() == 2) {
            reply.push_back(kv_store.count(request[1]) ? "1" : "0");
        } 
        else if (command == "MGET") {
            // MGET returns a multi-part message where each part is a value
            for (size_t i = 1; i < request.size(); ++i) {
                auto it = kv_store.find(request[i]);
                if (it != kv_store.end()) {
                    reply.push_back(it->second);
                } else {
                    reply.push_back("NIL");
                }
            }
        } 
        else if (command == "MSET") {
            // MSET expects pairs of keys and values: [MSET, key1, val1, key2, val2...]
            for (size_t i = 1; i + 1 < request.size(); i += 2) {
                kv_store[request[i]] = request[i + 1];
            }
            reply.push_back("OK");
        } 
        else {
            // If the command is unknown or malformed
            std::cerr << "Unknown or malformed command: " << command << std::endl;
            reply.push_back("ERROR");
        }

        // --- REPLY PHASE ---

        // A. Send the client identity back first so ZMQ knows who to route to
        zmq::message_t rep_id(client_id.data(), client_id.size());
        socket.send(rep_id, zmq::send_flags::sndmore);

        // B. Send the empty delimiter frame back
        zmq::message_t rep_empty(0);
        socket.send(rep_empty, zmq::send_flags::sndmore);

        // C. Send the actual multi-part reply payload
        for (size_t i = 0; i < reply.size(); ++i) {
            zmq::message_t msg(reply[i].data(), reply[i].size());
            
            // If it's the last element, don't set the SNDMORE flag
            zmq::send_flags flags = (i == reply.size() - 1) ? zmq::send_flags::none : zmq::send_flags::sndmore;
            
            socket.send(msg, flags);
        }
    }

    return 0;
}
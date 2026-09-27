/**
    Asynchronous Server
    
    One thread per client for reading/writing (no blocking between clients)
    One broadcast thread for publishing state at fixed ~60 Hz
    
    This allows clients to run at different speeds without affecting others.
*/

#include "evilNetworking.hpp"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <map>
#include <mutex>
#include <sstream>
#include <thread>
#include <condition_variable>

std::map< int, PlayerState > players;
std::mutex playersLock;
int nextId = 1;

const int CLIENT_PORT_BASE = 6000;  // Client threads listen on 6000, 6001, 6002, etc.

/**
    Per-client thread - handles JOIN/MOVE/LEAVE for one client
    Each client gets its own thread, so no blocking between clients
*/
void clientThread(zmq::context_t& context, int clientId) {
    try {
        zmq::socket_t socket(context, zmq::socket_type::rep);
        int port = CLIENT_PORT_BASE + clientId;
        socket.bind("tcp://*:" + std::to_string(port));
        std::cout << "[server] client " << clientId << " listening on port " << port << std::endl;

        while (true) {
            zmq::message_t request;
            if (!socket.recv(request, zmq::recv_flags::none)) {
                continue;
            }

            std::string requestText(static_cast<char*>(request.data()), request.size());
            std::istringstream in(requestText);
            std::string command;
            in >> command;

            std::string reply = "ERR";

            if (command == "JOIN") {
                std::lock_guard<std::mutex> lock(playersLock);

                int id = nextId++;

                // Spread players out
                PlayerState player;
                player.id = id;
                player.x = 200.0 + (id - 1) * 150.0;
                player.y = 300.0;
                players[id] = player;

                std::ostringstream out;
                out << std::setprecision(17);
                out << "ID " << id << " " << player.x << " " << player.y;
                reply = out.str();

                std::cout << "[server] Client " << id << " joined from client thread " << clientId << std::endl;
            }
            else if (command == "MOVE") {
                int id = 0;
                double x = 0.0;
                double y = 0.0;
                double clientTime = 0.0;  // Client's elapsed time (for async support)
                in >> id >> x >> y >> clientTime;

                if (!in.fail()) {
                    std::lock_guard<std::mutex> lock(playersLock);
                    if (players.count(id) > 0) {
                        players[id].x = x;
                        players[id].y = y;
                    }
                    reply = "OK";
                }
            }
            else if (command == "LEAVE") {
                int id = 0;
                in >> id;

                std::lock_guard<std::mutex> lock(playersLock);
                if (players.count(id) > 0) {
                    std::cout << "[server] Client " << id << " left" << std::endl;
                    players.erase(id);
                }
                reply = "OK";
            }

            socket.send(zmq::message_t(reply.begin(), reply.end()), zmq::send_flags::none);
        }
    }
    catch (const std::exception& e) {
        std::cerr << "[server] Client thread error: " << e.what() << std::endl;
    }
}

/**
    Broadcast thread - publishes world state at fixed rate (~60 Hz)
    Doesn't wait for clients, just sends state periodically
*/
void publishThread(zmq::context_t& context) {
    zmq::socket_t socket(context, zmq::socket_type::pub);
    socket.bind("tcp://*:" + std::to_string(PUB_PORT));
    std::cout << "[server] broadcasting world state on " << PUB_PORT << std::endl;

    while (true) {
        std::ostringstream out;
        {
            std::lock_guard<std::mutex> lock(playersLock);

            out << "STATE " << players.size();
            for (const auto& entry : players) {
                out << " " << entry.second.id
                    << " " << entry.second.x
                    << " " << entry.second.y;
            }
        }

        std::string message = out.str();
        socket.send(zmq::message_t(message.begin(), message.end()), zmq::send_flags::none);

        // Broadcast at ~60 Hz (16 ms per frame)
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
}

int main() {
    std::cout << "Async Server" << std::endl;
    std::cout << "Per-client threads on ports " << CLIENT_PORT_BASE << "+ (one per client)" << std::endl;
    std::cout << "Broadcast on port " << PUB_PORT << std::endl;

    zmq::context_t context(1);

    // Start broadcast thread (always running)
    std::thread publish(publishThread, std::ref(context));

    // For demo: start 3 pre-allocated client threads
    // In a real server, you'd spawn these dynamically as clients connect
    std::vector<std::thread> clientThreads;
    for (int i = 0; i < 3; ++i) {
        clientThreads.emplace_back(clientThread, std::ref(context), i);
    }

    // Wait for threads (they run forever)
    publish.join();
    for (auto& t : clientThreads) {
        t.join();
    }

    return 0;
}
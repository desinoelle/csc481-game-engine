/**
    Runs two threads, each owning one socket. The player list is the only shared state and the
    mutex guards it

    reply thread      answers JOIN, MOVE and LEAVE on port 5555
    publish thread    broadcasts every player on port 5556
*/

#include "evilNetworking.hpp"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <map>
#include <mutex>
#include <sstream>
#include <thread>

std::map< int, PlayerState > players;
std::mutex playersLock;
int nextId = 1;

/**
    Answers requests from clients
    Every request gets exactly one reply
*/
void replyThread( zmq::context_t& context ) {
    zmq::socket_t socket( context, zmq::socket_type::rep );
    socket.bind( "tcp://*:" + std::to_string( REQ_PORT ) );
    std::cout << "[server] listening for requests on " << REQ_PORT << std::endl;

    while ( true ) {
        zmq::message_t request;
        if ( !socket.recv( request, zmq::recv_flags::none ) ) {
            continue;
        }

        std::string requestText( static_cast< char* >( request.data() ), request.size() );
        std::istringstream in( requestText );
        std::string command;
        in >> command;

        std::string reply = "ERR";

        if ( command == "JOIN" ) {
            std::lock_guard< std::mutex > lock( playersLock );

            int id = nextId++;

            // Spread players out

            PlayerState player;
            player.id = id;
            player.x = 200.0 + ( id - 1 ) * 150.0;
            player.y = 300.0;
            players[ id ] = player;

            std::ostringstream out;
        out << std::setprecision( 17 );
            out << std::setprecision( 17 );
            out << "ID " << id << " " << player.x << " " << player.y;
            reply = out.str();

        }
        else if ( command == "MOVE" ) {
            int id = 0;
            double x = 0.0;
            double y = 0.0;
            in >> id >> x >> y;

            if ( !in.fail() ) {
                std::lock_guard< std::mutex > lock( playersLock );
                if ( players.count( id ) > 0 ) {
                    players[ id ].x = x;
                    players[ id ].y = y;
                }
                reply = "OK";
            }
        }
        else if ( command == "LEAVE" ) {
            int id = 0;
            in >> id;

            std::lock_guard< std::mutex > lock( playersLock );
            players.erase( id );
            reply = "OK";

        }

        socket.send( zmq::message_t( reply.begin(), reply.end() ), zmq::send_flags::none );
    }
}

/**
    Sends the whole world publically
*/
void publishThread( zmq::context_t& context ) {
    zmq::socket_t socket( context, zmq::socket_type::pub );
    socket.bind( "tcp://*:" + std::to_string( PUB_PORT ) );
    std::cout << "[server] broadcasting world state on " << PUB_PORT << std::endl;

    while ( true ) {
        std::ostringstream out;
        {
            std::lock_guard< std::mutex > lock( playersLock );

            out << "STATE " << players.size();
            for ( const auto& entry : players ) {
                out << " " << entry.second.id
                    << " " << entry.second.x
                    << " " << entry.second.y;
            }
        }

        std::string message = out.str();
        socket.send( zmq::message_t( message.begin(), message.end() ), zmq::send_flags::none );

        std::this_thread::sleep_for( std::chrono::milliseconds( 16 ) );
    }
}

int main() {
    std::cout << "starting" << std::endl;

    zmq::context_t context( 1 );

    std::thread reply( replyThread, std::ref( context ) );
    std::thread publish( publishThread, std::ref( context ) );

    reply.join();
    publish.join();
    return 0;
}

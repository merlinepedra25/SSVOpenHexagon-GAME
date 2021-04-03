// Copyright (c) 2013-2020 Vittorio Romeo
// License: Academic Free License ("AFL") v. 3.0
// AFL License page: https://opensource.org/licenses/AFL-3.0

#include "SSVOpenHexagon/Core/HexagonServer.hpp"

#include "SSVOpenHexagon/Utils/Match.hpp"
#include "SSVOpenHexagon/Global/Assets.hpp"
#include "SSVOpenHexagon/Global/Assert.hpp"
#include "SSVOpenHexagon/Core/HexagonGame.hpp"
#include "SSVOpenHexagon/Utils/Concat.hpp"
#include "SSVOpenHexagon/Online/Shared.hpp"

#include <SFML/Network.hpp>

#include <thread>
#include <chrono>

static auto& slog(const char* funcName)
{
    return ::ssvu::lo(::hg::Utils::concat("hg::HexagonServer::", funcName));
}

#define SSVOH_SLOG ::slog(__func__)

#define SSVOH_SLOG_VERBOSE \
    if(_verbose) ::slog(__func__)

#define SSVOH_SLOG_ERROR ::slog(__func__) << "[ERROR] "

#define SSVOH_SLOG_VAR(x) '\'' << #x << "': '" << x << '\''

namespace hg
{

[[nodiscard]] bool HexagonServer::initializeTcpListener()
{
    SSVOH_SLOG << "Initializing TCP listener...\n";

    _listener.setBlocking(true);
    if(_listener.listen(_serverPort) == sf::TcpListener::Status::Error)
    {
        SSVOH_SLOG_ERROR << "Failure initializing TCP listener\n";
        return false;
    }

    return true;
}

[[nodiscard]] bool HexagonServer::initializeSocketSelector()
{
    _socketSelector.add(_listener);
    return true;
}

void HexagonServer::runSocketSelector()
{
    while(_running)
    {
        runSocketSelector_Iteration();
    }
}

void HexagonServer::runSocketSelector_Iteration()
{
    SSVOH_SLOG_VERBOSE << "Waiting for clients...\n";

    if(_socketSelector.wait(sf::seconds(30)))
    {
        // A timeout is specified so that we can purge clients even if we didn't
        // receive anything.

        runSocketSelector_Iteration_TryAcceptingNewClient();
        runSocketSelector_Iteration_LoopOverSockets();
    }

    runSocketSelector_Iteration_PurgeClients();
} // namespace hg

bool HexagonServer::runSocketSelector_Iteration_TryAcceptingNewClient()
{
    if(!_socketSelector.isReady(_listener))
    {
        return false;
    }

    SSVOH_SLOG_VERBOSE << "Listener is ready\n";

    ConnectedClient& potentialClient =
        _connectedClients.emplace_back(Clock::now());

    sf::TcpSocket& potentialSocket = potentialClient._socket;
    potentialSocket.setBlocking(true);

    const void* potentialClientAddress = static_cast<void*>(&potentialClient);

    // The listener is ready: there is a pending connection
    if(_listener.accept(potentialSocket) != sf::Socket::Done)
    {
        SSVOH_SLOG << "Listener failed to accept new client '"
                   << potentialClientAddress << "'\n";

        // Error, we won't get a new connection, delete the socket
        _connectedClients.pop_back();
        return false;
    }

    SSVOH_SLOG << "Listener accepted new client '" << potentialClientAddress
               << "'\n";

    // Add the new client to the selector so that we will  be notified when he
    // sends something
    _socketSelector.add(potentialSocket);
    return true;
}

void HexagonServer::runSocketSelector_Iteration_LoopOverSockets()
{
    for(auto it = _connectedClients.begin(); it != _connectedClients.end();
        ++it)
    {
        ConnectedClient& connectedClient = *it;
        const void* clientAddress = static_cast<void*>(&connectedClient);
        sf::TcpSocket& clientSocket = connectedClient._socket;

        if(!_socketSelector.isReady(clientSocket))
        {
            continue;
        }

        SSVOH_SLOG_VERBOSE << "Client '" << clientAddress
                           << "' has sent data\n ";

        // The client has sent some data, we can receive it
        _packetBuffer.clear();
        if(clientSocket.receive(_packetBuffer) == sf::Socket::Done)
        {
            SSVOH_SLOG_VERBOSE << "Successfully received data from client '"
                               << clientAddress << "'\n";

            if(processPacket(connectedClient, _packetBuffer))
            {
                connectedClient._lastActivity = Clock::now();
                connectedClient._consecutiveFailures = 0;

                continue;
            }
            else
            {
                ++connectedClient._consecutiveFailures;
            }
        }

        // Failed to receive data
        SSVOH_SLOG_VERBOSE << "Failed to receive data from client '"
                           << clientAddress << "' (consecutive failures: "
                           << connectedClient._consecutiveFailures << ")\n";

        ++connectedClient._consecutiveFailures;

        constexpr int maxConsecutiveFailures = 5;
        if(connectedClient._consecutiveFailures == maxConsecutiveFailures)
        {
            SSVOH_SLOG << "Too many consecutive failures for client '"
                       << clientAddress << "', removing from list\n";

            _socketSelector.remove(connectedClient._socket);
            it = _connectedClients.erase(it);
        }
    }
}

void HexagonServer::runSocketSelector_Iteration_PurgeClients()
{
    constexpr std::chrono::duration maxInactivity = std::chrono::seconds(60);

    const TimePoint now = Clock::now();

    for(auto it = _connectedClients.begin(); it != _connectedClients.end();
        ++it)
    {
        ConnectedClient& connectedClient = *it;
        const void* clientAddress = static_cast<void*>(&connectedClient);

        const auto kickClient = [&] {
            makeServerToClientPacket(_packetBuffer, STCPKick{});
            connectedClient._socket.send(_packetBuffer);

            _socketSelector.remove(connectedClient._socket);
            it = _connectedClients.erase(it);
        };

        if(connectedClient._mustDisconnect)
        {
            SSVOH_SLOG << "Client '" << clientAddress
                       << "' disconnected, removing from list\n";

            kickClient();
            continue;
        }

        if(now - connectedClient._lastActivity > maxInactivity)
        {
            SSVOH_SLOG << "Client '" << clientAddress
                       << "' timed out, removing from list\n";

            kickClient();
            continue;
        }
    }
}

[[nodiscard]] bool HexagonServer::processPacket(
    ConnectedClient& c, sf::Packet& p)
{
    const void* clientAddress = static_cast<void*>(&c);

    _errorOss.str("");
    const PVClientToServer pv = decodeClientToServerPacket(_errorOss, p);

    return Utils::match(
        pv,

        [&](const PInvalid&) {
            SSVOH_SLOG_ERROR << "Error processing packet from client '"
                             << clientAddress
                             << "', details: " << _errorOss.str() << '\n';

            return false;
        },

        [&](const CTSPHeartbeat&) { return true; },

        [&](const CTSPDisconnect&) {
            c._mustDisconnect = true;
            return true;
        }

        //
    );
}

HexagonServer::HexagonServer(HGAssets& assets, HexagonGame& hexagonGame)
    : _assets{assets}, _hexagonGame{hexagonGame},
      _serverIp{Config::getServerIp()}, _serverPort{Config::getServerPort()},
      _listener{}, _socketSelector{}, _running{true}, _verbose{true}
{
    SSVOH_SLOG << "Initializing server...\n";

    if(_serverIp == sf::IpAddress::None)
    {
        SSVOH_SLOG_ERROR << "Failure initializing server, invalid ip address '"
                         << Config::getServerIp() << "'\n";

        return;
    }

    SSVOH_SLOG << "Server data:\n"
               << SSVOH_SLOG_VAR(_serverIp) << '\n'
               << SSVOH_SLOG_VAR(_serverPort) << '\n';

    if(!initializeTcpListener())
    {
        SSVOH_SLOG_ERROR << "Failure initializing server, TCP listener could "
                            "not be initialized\n";

        return;
    }

    if(!initializeSocketSelector())
    {
        SSVOH_SLOG_ERROR
            << "Failure initializing server, socket selector could "
               "not be initialized\n";

        return;
    }

    runSocketSelector();
}

HexagonServer::~HexagonServer()
{
    SSVOH_SLOG << "Uninitializing server...\n";

    for(ConnectedClient& connectedClient : _connectedClients)
    {
        connectedClient._socket.setBlocking(true);

        makeServerToClientPacket(_packetBuffer, STCPKick{});
        connectedClient._socket.send(_packetBuffer);

        connectedClient._socket.disconnect();
    }

    _socketSelector.clear();
    _listener.close();
}

} // namespace hg
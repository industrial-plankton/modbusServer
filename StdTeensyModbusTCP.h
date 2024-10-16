#ifndef H_StdTeenyModbus_IP
#define H_StdTeenyModbus_IP

#include <QNEthernet.h>
#include <utility>
#include <algorithm>
#include <stdint.h>
#include <registers.h>
#include <array>

using namespace qindesign::network;

struct PotentialClient
{
    bool exists;
    EthernetClient client;
};

class EthernetClientStream : public Stream
{
private:
    EthernetClient &Client;

public:
    explicit EthernetClientStream(EthernetClient &client) : Client(client) {};
    ~EthernetClientStream() {};

    int available() override { return Client.available(); };
    int read() override { return Client.read(); };
    int peek() override { return Client.peek(); };
    size_t write(uint8_t b) override { return Client.write(b); };
};

struct TCPServerInit
{
    uint16_t ServerPort;
    uint32_t LinkTimeout;
    uint32_t ClientTimeout;
    uint32_t ShutdownTimeout;
    IPAddress staticIP{192, 168, 0, 10};
    IPAddress subnetMask{255, 255, 0, 0};
    IPAddress gateway{192, 168, 0, 1};
};

struct ClientState
{
    explicit ClientState(EthernetClient client)
        : client(std::move(client)) {}

    EthernetClient client;
    bool closed = false;

    // For timeouts.
    uint32_t lastRead = millis(); // Mark creation time

    // For half closed connections, after "Connection: close" was sent
    // and closeOutput() was called
    uint32_t closedTime = 0;   // When the output was shut down
    bool outputClosed = false; // Whether the output was shut down

    // Parsing state
    bool emptyLine = false;
};

class StdTeenyModbusTCPServer
{
private:
    uint32_t ClientTimeout;
    uint32_t ShutdownTimeout;

    std::vector<ClientState> clients;
    EthernetServer server;

    Registers &registers;

public:
    StdTeenyModbusTCPServer(TCPServerInit ServerSettings, Registers &registers)
        : ClientTimeout{ServerSettings.ClientTimeout},
          ShutdownTimeout{ServerSettings.ShutdownTimeout},
          server(ServerSettings.ServerPort),
          registers{registers} {};
    ~StdTeenyModbusTCPServer() {};

    void Initialize(TCPServerInit InitData)
    {
        // Add listeners
        // It's important to add these before doing anything with Ethernet
        // so no events are missed.

        // Listen for link changes
        Ethernet.onLinkState([](bool state)
                             { Serial.printf("[Ethernet] Link %s\r\n", state ? "ON" : "OFF"); });

        Serial.printf("Starting Ethernet with static IP...\r\n");
        Ethernet.begin(InitData.staticIP, InitData.subnetMask, InitData.gateway);

        // When setting a static IP, the address is changed immediately,
        // but the link may not be up; optionally wait for the link here
        if (InitData.LinkTimeout > 0)
        {
            if (!Ethernet.waitForLink(InitData.LinkTimeout))
            {
                Serial.printf("Failed to get link\r\n");
                // We may still see a link later, after the timeout, so
                // continue instead of returning
            }
        }

        Serial.printf("Starting server on port %u...", InitData.ServerPort);
        server.begin();
    }

    void processModbusClient(ClientState &state)
    {
        std::array<uint8_t, 256> ModbusFrame;
        uint16_t bufferIndex = 0;
        while (state.client.available())
        {
            state.lastRead = millis();
            if (bufferIndex <= ModbusFrame.size())
            {
                ModbusFrame[bufferIndex] = state.client.read();
                bufferIndex++;
            }
            else
            {
                state.client.read();
            }
        }

        const auto responseSize = ReceiveTCPStream(registers, ModbusFrame, bufferIndex);
        if (responseSize > 0)
        {
            state.client.writeFully(ModbusFrame.data(), responseSize);
            state.client.flush();
        }
    }

    void ConnectNewClients()
    {
        EthernetClient client = server.accept();
        if (client)
        {
            IPAddress ip = client.remoteIP();
            Serial.printf("Client connected: %u.%u.%u.%u\r\n", ip[0], ip[1], ip[2], ip[3]);
            clients.emplace_back(std::move(client));
            Serial.printf("Client count: %u\r\n", clients.size());
        }
    }

    void ProcessClients()
    {
        // Process data from each client
        for (ClientState &state : clients)
        { // Use a reference so we don't copy
            if (!state.client.connected())
            {
                state.closed = true;
                continue;
            }

            // Check if we need to force close the client
            if (state.outputClosed)
            {
                if (millis() - state.closedTime >= ShutdownTimeout)
                {
                    IPAddress ip = state.client.remoteIP();
                    Serial.printf("Client shutdown timeout: %u.%u.%u.%u\r\n",
                                  ip[0], ip[1], ip[2], ip[3]);
                    state.client.close();
                    state.closed = true;
                    continue;
                }
            }
            else
            {
                if (millis() - state.lastRead >= ClientTimeout)
                {
                    IPAddress ip = state.client.remoteIP();
                    Serial.printf("Client timeout: %u.%u.%u.%u\r\n", ip[0], ip[1], ip[2], ip[3]);
                    state.client.close();
                    state.closed = true;
                    continue;
                }
            }

            processModbusClient(state);
        }

        // This looks stupid, but it's required as remove_if() doesn't shrink the vector
        clients.erase(std::remove_if(clients.begin(), clients.end(),
                                     [](const auto &state)
                                     { return state.closed; }),
                      clients.end());
    }

    PotentialClient GetUpdateClient()
    {
        // Process data from each client
        for (ClientState &state : clients)
        { // Use a reference so we don't copy
            if (!state.client.connected())
            {
                state.closed = true;
                continue;
            }

            // Check if we need to force close the client
            if (state.outputClosed)
            {
                if (millis() - state.closedTime >= ShutdownTimeout)
                {
                    IPAddress ip = state.client.remoteIP();
                    Serial.printf("Client shutdown timeout: %u.%u.%u.%u\r\n",
                                  ip[0], ip[1], ip[2], ip[3]);
                    state.client.close();
                    state.closed = true;
                    continue;
                }
            }
            else
            {
                if (millis() - state.lastRead >= ClientTimeout)
                {
                    IPAddress ip = state.client.remoteIP();
                    Serial.printf("Client timeout: %u.%u.%u.%u\r\n", ip[0], ip[1], ip[2], ip[3]);
                    state.client.close();
                    state.closed = true;
                    continue;
                }
            }
        }

        // This looks stupid, but it's required as remove_if() doesn't shrink the vector
        clients.erase(std::remove_if(clients.begin(), clients.end(),
                                     [](const auto &state)
                                     { return state.closed; }),
                      clients.end());

        if (clients.empty())
        {
            return {.exists = false};
        }
        return {.exists = true, .client = clients.back().client};
    }
};

#endif
#include "ServerUtils.h"

// checkPort binds a socket to see whether the port is free. The Berkeley API
// is the same on Windows through Winsock, but it lives in different headers,
// closes with closesocket rather than close, and needs the library started.
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#include "FileSystemUtils.h"
#include "LogUtils.h"
#include "RequestEnvironment.h"
#include "ThreadSafeContainers.h"
#include "exceptions/CancellationException.h"

#include "loguru.h"

#include <fstream>

namespace ServerUtils {
    using json = nlohmann::json;

    void setThreadOptions(grpc::ServerContext *context, bool testMode) {
        if (!CollectionUtils::containsKey(context->client_metadata(), "clientid")) {
            if (testMode) {
                std::string client = LogUtils::TEST_CLIENT;
                RequestEnvironment::setClientId(client);
                RequestEnvironment::setServerContext(nullptr);
                loguru::set_thread_name(client.c_str());
                return;
            }
            LOG_S(ERROR) << "Tried to set thread options for unnamed client\n Cancelling request\n";
            throw CancellationException();
        }
        auto &ref = context->client_metadata().find("clientid")->second;
        std::string id{ref.begin(), ref.end()};
        RequestEnvironment::setClientId(id);
        RequestEnvironment::setServerContext(context);
        loguru::set_thread_name(id.c_str());
    }

    void registerClient(concurrent_set<std::string> &clients, std::string client) {
        if (!client.empty()) {
            if (!clients.in(client)) {
                LOG_S(INFO)
                << "client " << client << " was not found in server storage; registering it.";
            }
        } else {
            client = LogUtils::UNNAMED_CLIENT;
            if (!clients.in(client)) {
                LOG_S(WARNING) << "Received registration of an unnamed client.\n"
                                  "Perhaps $USER variable could not be fetched?";
            }
        }
        clients.insert(client);
        clients.writeToJson();
    }

    void loadClientsData(concurrent_set<std::string> &result) {
        fs::path jsonPath = Paths::getClientsJsonPath();
        if (!fs::exists(jsonPath) || fs::is_empty(jsonPath)) {
            return;
        }
        std::ifstream stream(jsonPath);
        json j = json::parse(stream);
        auto clients = j.get<std::vector<std::string>>();
        for (const auto &client : clients) {
            auto logDir = Paths::getClientLogDir(client);
            if (fs::exists(logDir)) {
                auto ftime = fs::last_write_time(logDir);
                auto modified = TimeUtils::convertFileToSystemClock(ftime);
                auto now = std::chrono::system_clock::now();
                std::chrono::duration<double> elapsed_seconds{
                        std::chrono::duration_cast<std::chrono::duration<double>>(now - modified)
                };
                if (elapsed_seconds.count() < TimeUtils::DAY_DURATION.count()) {
                    result.insert(client);
                } else {
                    FileSystemUtils::removeAll(Paths::getClientLogDir(client));
                }
            } else {
                result.insert(client);
            }
        }
    }

    bool checkPort(std::string host, uint16_t port) {
        bool result = true;
#ifdef _WIN32
        // Winsock is not usable until it has been started, and every process
        // that uses it has to do so; gRPC starts its own copy but that says
        // nothing about this one.
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            return false;
        }
        struct WinsockScope {
            ~WinsockScope() { WSACleanup(); }
        } winsockScope;
        SOCKET sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sock != INVALID_SOCKET) {
#else
        int sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sock != -1) {
#endif
            sockaddr_in socketIn;
            socketIn.sin_family = AF_INET;
            socketIn.sin_port = htons(port);
            socketIn.sin_addr.s_addr = inet_addr(host.c_str());
            const int MAX_CONNECT = 10;
            int connect = 0;
            while (bind(sock, (sockaddr *) (&socketIn), sizeof(socketIn)) != 0 && connect < MAX_CONNECT) {
                connect++;
                if(connect > MAX_CONNECT) {
                    result = false;
                }
            }
            if (result) {
                int so_error;
#ifdef _WIN32
                int len = sizeof so_error;
                result = getsockopt(sock, SOL_SOCKET, SO_ERROR,
                                    reinterpret_cast<char *>(&so_error), &len) == 0;
#else
                socklen_t len = sizeof so_error;
                result = getsockopt(sock, SOL_SOCKET, SO_ERROR, &so_error, &len) == 0;
#endif
            }
#ifdef _WIN32
            closesocket(sock);
#else
            close(sock);
#endif
        }
        return result;
    }
}

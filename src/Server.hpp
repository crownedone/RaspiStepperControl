#pragma once

#include <boost/signals2/signal.hpp>

#include <memory>
#include <string>
#include <set>

#include <thread>
#include <condition_variable>
#include <mutex>

// ignore datatype conversion warnings
#pragma warning(push)
#pragma warning(disable : 4267)
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#pragma warning(pop)

/// Simple tcp connection class that will keep a connection as long it lives and sent and receive asynchronous messages
class WebSocketServer {
public:
    WebSocketServer(uint16_t port);

    ~WebSocketServer();

    /// A message has been received.
    /// The message itself is passed a std::string even if the message is binary object.
    boost::signals2::signal<void(const std::string &)> onMessage;

    /// A client has connected.
    /// The connection handle is passed as first argument.
    boost::signals2::signal<void(websocketpp::connection_hdl)> onConnected;

    /// A client has disconnected.
    /// The connection handle is passed as first argument
    boost::signals2::signal<void(websocketpp::connection_hdl)> onDisconnected;

    /// Send a message to all registered connections.
    /// @param message Message to send.
    /// @param hdl If not empty the message will only be sent to this connection.
    void send(const std::string &message, websocketpp::connection_hdl hdl = websocketpp::connection_hdl());

    /// Send a message to all registered connections.
    /// @param message Message to send.
    /// @param hdl If not empty the message will only be sent to this connection.
    void send(std::shared_ptr<const std::string> message,
              websocketpp::connection_hdl hdl = websocketpp::connection_hdl());

    typedef websocketpp::server<websocketpp::config::asio> Server;

    /// Called on every new connection.
    virtual void onOpen(websocketpp::connection_hdl hdl);

    /// Called on a connection failure e.g. timeout.
    virtual void onFail(websocketpp::connection_hdl hdl);

    /// Called on a closed connection.
    virtual void onClose(websocketpp::connection_hdl hdl);

    /// Called on a received message.
    virtual void onMessageReceived(websocketpp::connection_hdl hdl, Server::message_ptr msg);

    void setMaxMessageSize(size_t newSize);

private:
    typedef std::set<websocketpp::connection_hdl, std::owner_less<websocketpp::connection_hdl>> ConnectionList;

    /// List of WebSocket connections.
    ConnectionList m_ConnectionList;

    /// WebSocket server.
    std::shared_ptr<Server> m_Server;

    std::atomic<bool> m_Running;
    std::mutex m_mutex;
    std::condition_variable m_not_empty;
    /// Queue for send
    std::deque<std::pair<std::shared_ptr<const std::string>, websocketpp::connection_hdl>> m_Work;
    /// Worker thread
    std::thread m_WorkerThread;

    std::thread m_AsioThread;

    inline bool has_new_data() const {
        return !m_Work.empty() || !m_Running;
    }
};
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
#include <websocketpp/client.hpp>

#pragma warning(pop)

/// Simple tcp connection class that will keep a connection as long it lives and sent and receive asynchronous messages
class WebSocketClient {
public:
    WebSocketClient();

    ~WebSocketClient();

    // asynchronous try to connect
    bool connect(std::string address, uint16_t port);

    /// A message has been received.
    /// The message itself is passed a std::string even if the message is binary object.
    boost::signals2::signal<void(const std::string &)> onMessage;

    /// A client has connected.
    /// The connection handle is passed as first argument.
    boost::signals2::signal<void()> onConnected;

    /// Send a message to all registered connections.
    /// @param message Message to send.
    /// @param hdl If not empty the message will only be sent to this connection.
    void send(const std::string &message, websocketpp::connection_hdl hdl = websocketpp::connection_hdl());

    /// Send a message to all registered connections.
    /// @param message Message to send.
    /// @param hdl If not empty the message will only be sent to this connection.
    void send(std::shared_ptr<const std::string> message,
              websocketpp::connection_hdl hdl = websocketpp::connection_hdl());
    void setMaxMessageSize(size_t newSize);

private:
    typedef websocketpp::client<websocketpp::config::asio> Client;

    /// Called on every new connection.
    virtual void onOpen(websocketpp::connection_hdl hdl);

    /// Called on a connection failure e.g. timeout.
    virtual void onFail(websocketpp::connection_hdl hdl);

    /// Called on a closed connection.
    virtual void onClose(websocketpp::connection_hdl hdl);

    /// Called on a received message.
    virtual void onMessageReceived(websocketpp::connection_hdl hdl, Client::message_ptr msg);

private:
    Client::connection_ptr m_Connection;

    /// WebSocket server.
    std::shared_ptr<Client> m_Client;

    std::atomic<bool> m_Running;
    std::mutex m_mutex;
    std::condition_variable m_not_empty;
    /// Queue for send
    std::deque<std::pair<std::shared_ptr<const std::string>, websocketpp::connection_hdl>> m_Work;
    /// Worker thread
    std::thread m_WorkerThread;

    std::thread m_AsioThread;

    bool has_new_data() const {
        return !m_Work.empty() || !m_Running;
    }
};
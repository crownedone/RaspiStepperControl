#include "Client.hpp"

WebSocketClient::WebSocketClient() : m_Running(false) {
    m_Client = std::make_shared<Client>();

    m_Client->set_access_channels(websocketpp::log::alevel::none);
    m_Client->set_error_channels(websocketpp::log::elevel::none);

    m_Client->set_open_handler(bind(&WebSocketClient::onOpen, this, std::placeholders::_1));
    m_Client->set_fail_handler(bind(&WebSocketClient::onFail, this, std::placeholders::_1));
    m_Client->set_close_handler(bind(&WebSocketClient::onClose, this, std::placeholders::_1));
    m_Client->set_message_handler(
            bind(&WebSocketClient::onMessageReceived, this, std::placeholders::_1, std::placeholders::_2));

    m_Client->init_asio();

    m_Running = true;

    m_WorkerThread = std::thread([=]() {
        try {
            while (m_Running) {
                std::shared_ptr<const std::string> message;
                websocketpp::connection_hdl hdl;

                {
                    std::unique_lock<std::mutex> lock(m_mutex);
                    m_not_empty.wait_for(lock, std::chrono::milliseconds(100),
                                         std::bind(&WebSocketClient::has_new_data, this));

                    if (!m_Running) {
                        break;
                    }

                    auto work = m_Work[0];
                    message = work.first;
                    hdl = work.second;
                    m_Work.pop_front();
                }

                if (hdl.lock()) {
                    auto con = m_Client->get_con_from_hdl(hdl);
                    m_Client->send(con, *message,
                                   websocketpp::frame::opcode::TEXT);
                } else {
                    m_Client->send(m_Connection, *message,
                                   websocketpp::frame::opcode::TEXT);
                }
            }
        }
        catch (std::exception &ex) {
            std::cerr << "Exception in WebSocketClient: '%s'."<< ex.what() << std::endl;
        }
    });
}

bool WebSocketClient::connect(std::string address, uint16_t port) {
    try {
        websocketpp::lib::error_code ec;
        std::string uri("ws://");
        uri += address;
        uri += ":";
        uri += std::to_string(port);
        auto con = m_Client->get_connection(uri, ec);

        if (ec) {
            std::cerr << "Failed to start client, error = '%s'." << ec.message().c_str() << std::endl;
            m_Connection = nullptr;
            return false;
        }

        m_Client->connect(con);

        if (m_AsioThread.joinable()) {
            m_AsioThread.join();
        }

        m_AsioThread = std::thread([=]() {
            m_Client->run();
        });
    }
    catch (std::exception &ex) {
        std::cerr << "Failed to open client, error = '%s'." << ex.what() << std::endl;
        return false;
    }

    return true;
}


WebSocketClient::~WebSocketClient() {
    m_Running = false;

    if (m_WorkerThread.joinable()) {
        m_WorkerThread.join();
    }

    try {
        websocketpp::lib::error_code ec;
        m_Client->pause_reading(m_Connection);
        m_Client->close(m_Connection, websocketpp::close::status::going_away, "", ec);

        if (ec) {
            std::cerr << "Failed to shutdown connection, error = '%s'." << ec.message().c_str() << std::endl;
        }
    }
    catch (std::exception &ex) {
        std::cerr << "Failed to shutdown connection, error = '%s'." << ex.what() << std::endl;
    }

    m_Connection.reset();

    if (m_AsioThread.joinable()) {
        m_AsioThread.join();
    }
}

void WebSocketClient::send(const std::string &message, websocketpp::connection_hdl hdl) {
    if (!message.empty()) {
        send(std::make_shared<const std::string>(message), hdl);
    }
}

void WebSocketClient::send(std::shared_ptr<const std::string> message, websocketpp::connection_hdl hdl) {
    if (message && !message->empty()) {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_Work.push_back(std::make_pair(message, hdl));
        lock.unlock();
        m_not_empty.notify_one();
    }
}

void WebSocketClient::onOpen(websocketpp::connection_hdl hdl) {
    try {
        // make sure to only fire onConnected one time for a connection
        bool newConnection = !m_Connection;
        m_Connection = m_Client->get_con_from_hdl(hdl);

        if (newConnection) {
            onConnected();
        }
    }
    catch (std::exception &ex) {
        std::cerr << "Failed during onConnected, error '%s'." << ex.what() << std::endl;
    }
}

void WebSocketClient::onFail(websocketpp::connection_hdl hdl) {
    m_Running = false;

    if (m_WorkerThread.joinable()) {
        m_WorkerThread.join();
    }

    Client::connection_ptr con = m_Client->get_con_from_hdl(hdl);
    std::string error_message = con->get_ec().message();
    std::cerr << "Failed while trying to connect. %s" << error_message.c_str() << std::endl;
}

void WebSocketClient::onClose(websocketpp::connection_hdl hdl) {
    m_Running = false;

    if (m_WorkerThread.joinable()) {
        m_WorkerThread.join();
    }
}

void WebSocketClient::onMessageReceived(websocketpp::connection_hdl hdl, Client::message_ptr msg) {
    try {
        onMessage(msg->get_payload());
    }
    catch (std::exception &ex) {
        std::cerr << "Failed during onMessage, error '%s'." << ex.what() << std::endl;
    }
}

void WebSocketClient::setMaxMessageSize(size_t newSize) {
    if (!m_Client)
        return;
    m_Client->set_max_message_size(newSize);
}

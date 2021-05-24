#include "Server.hpp"

WebSocketServer::WebSocketServer(uint16_t port) : m_Running(false) {
    m_Server = std::make_shared<Server>();

    m_Server->set_access_channels(websocketpp::log::alevel::none);
    m_Server->set_error_channels(websocketpp::log::elevel::none);

    m_Server->set_open_handler(bind(&WebSocketServer::onOpen, this, std::placeholders::_1));
    m_Server->set_fail_handler(bind(&WebSocketServer::onFail, this, std::placeholders::_1));
    m_Server->set_close_handler(bind(&WebSocketServer::onClose, this, std::placeholders::_1));
    m_Server->set_message_handler(
            bind(&WebSocketServer::onMessageReceived, this, std::placeholders::_1, std::placeholders::_2));

    try {
        m_Server->set_reuse_addr(true);

        m_Server->init_asio();

        m_Server->listen(port);
        m_Server->start_accept();

        m_AsioThread = std::thread([=]() {
            try {
                m_Server->run();
            }
            catch (...) {
                // will throw abandoned wait if shutdown while not connected yet. We dont care
            }
        });
    }
    catch (std::exception &ex) {
        std::cerr << "Failed to open server, error = " << ex.what() << std::endl;
    }

    m_Running = true;
    m_WorkerThread = std::thread([=]() {
        while (m_Running) {
            std::shared_ptr<const std::string> message;
            websocketpp::connection_hdl hdl;

            {
                std::unique_lock<std::mutex> lock(m_mutex);

                m_not_empty.wait(lock, std::bind(&WebSocketServer::has_new_data, this));

                if (!m_Running) {
                    break;
                }

                auto work = m_Work[0];
                message = work.first;
                hdl = work.second;
                m_Work.pop_front();
            }

            if (hdl.lock()) {
                try {
                    auto con = m_Server->get_con_from_hdl(hdl);
                    m_Server->send(con, *message, websocketpp::frame::opcode::TEXT);
                }
                catch (std::exception &ex) {
                    std::cerr << "Failed to send message, error = " << ex.what() << std::endl;
                }
            } else {
                for (auto it : m_ConnectionList) {
                    try {
                        m_Server->send(it, *message,websocketpp::frame::opcode::TEXT);
                    }
                    catch (std::exception &ex) {
                        std::cerr << "Failed to send message, error = "<< ex.what() << std::endl;
                    }
                }
            }
        }
    });
}

void WebSocketServer::setMaxMessageSize(size_t newSize) {
    if (m_Server) {
        m_Server->set_max_message_size(newSize);
    }
}

WebSocketServer::~WebSocketServer() {
    m_Running = false;
    m_not_empty.notify_one(); // need notification for running stop

    if (m_WorkerThread.joinable()) {
        m_WorkerThread.join();
    }

    m_Server->stop_listening();

    for (auto it : m_ConnectionList) {
        try {
            websocketpp::lib::error_code ec;
            m_Server->pause_reading(it);
            m_Server->close(it, websocketpp::close::status::going_away, "", ec);

            if (ec) {
                std::cerr << "Failed to shutdown connection, error = '%s'."<< ec.message().c_str() << std::endl;
            }
        }
        catch (std::exception &ex) {
            std::cerr << "Failed to shutdown connection, error = '%s'."<< ex.what() << std::endl;
        }
    }

    m_ConnectionList.clear();
    m_Server.reset();

    if (m_AsioThread.joinable()) {
        m_AsioThread.join();
    }
}

void WebSocketServer::send(const std::string &message, websocketpp::connection_hdl hdl) {
    if (!message.empty()) {
        send(std::make_shared<const std::string>(message), hdl);
    }
}

void WebSocketServer::send(std::shared_ptr<const std::string> message, websocketpp::connection_hdl hdl) {
    if (message && !message->empty()) {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_Work.push_back(std::make_pair(message, hdl));
        lock.unlock();
        m_not_empty.notify_one();
    }
}

void WebSocketServer::onOpen(websocketpp::connection_hdl hdl) {
    m_ConnectionList.insert(hdl);

    try {
        onConnected(hdl);
    }
    catch (std::exception &ex) {
        std::cerr << "Failed during onConnected, error '%s'."<< ex.what() << std::endl;
    }
}

void WebSocketServer::onFail(websocketpp::connection_hdl hdl) {
    Server::connection_ptr con = m_Server->get_con_from_hdl(hdl);
    std::string error_message = con->get_ec().message();
    std::cerr << "Failed while trying to connect. %s" << error_message.c_str() << std::endl;

    m_ConnectionList.erase(hdl);
}

void WebSocketServer::onClose(websocketpp::connection_hdl hdl) {
    try {
        onDisconnected(hdl);
    }
    catch (std::exception &ex) {
        std::cerr << "Failed during onDisconnected, error '%s'." << ex.what() << std::endl;
    }

    m_ConnectionList.erase(hdl);
}

void WebSocketServer::onMessageReceived(websocketpp::connection_hdl hdl, Server::message_ptr msg) {
    try {
        onMessage(msg->get_payload());
    }
    catch (std::exception &ex) {
        std::cerr << "Failed during onMessage, error '%s'." << ex.what() << std::endl;
    }
}

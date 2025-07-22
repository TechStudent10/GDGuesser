#include "manager.hpp"

const std::string getServerUrl() {
    auto str = Mod::get()->getSettingValue<std::string>("server-url");
    if (str.ends_with("/")) {
        if (str.empty()) {
            log::error("server URL is empty... uh oh");
            return "";
        }
        str.pop_back();
    }
    if (str.empty()) {
        log::error("server URL is empty... uh oh");
    }

    return str;
}

NetworkManager::NetworkManager() {
    init();
}

void NetworkManager::init() {
    client.init_asio();

    #ifndef DEBUG_BUILD
    client.set_tls_init_handler([this](Handle hdl) -> TLSCtx {
        return this->onTlsInit(hdl);
    });
    #endif

    client.set_message_handler([this](Handle hdl, Client::message_ptr msg) {
        this->onMessage(hdl, msg);
    });
    client.set_open_handler([this](Handle hdl) {
        this->onOpen(hdl);
    });
    client.set_close_handler([this](Handle hdl) {
        this->onClose(hdl);
    });
    client.set_fail_handler([this](Handle hdl) {
        this->onFail(hdl);
    });

    client.start_perpetual();

    runThread = std::thread([this]() {
        this->run();
    });
    isRunning = true;
    heartbeatThread = std::thread([this]() {
        this->heartbeat();
    });
}

void NetworkManager::heartbeat() {
    while (isRunning) {
        using namespace std::chrono_literals;
        auto lock = std::unique_lock<std::mutex>(heartbeatMutex);
        if (!heartbeatCondition.wait_for(lock, 10000ms, [this]() { return bool(!isRunning); }) && isConnected) {
            auto error = std::error_code();

			client.ping(hdl, "", error);

			if (error) {
				continue;
			}
        }
    }
}

void NetworkManager::run() {
    client.run();
}

void NetworkManager::close() {
    client.stop_perpetual();
    isRunning = false;
}

void NetworkManager::connect(std::string token, std::function<void()> callback) {
    if (isConnected) {
        callback();
        return;
    }

    auto error = std::error_code();

    auto connection = client.get_connection(
        getServerUrl(),
        error
    );

    if (error || !connection) {
        log::error("no connection! {}", error.message());
        return;
    }

    connection->append_header("Authorization", token);
    this->token = token;

    connCallback = callback;
    client.connect(connection);
}

void NetworkManager::connect(std::string token) {
    connect(token, []() {});
}

void NetworkManager::disconnect() {
    auto error = std::error_code();
    client.close(hdl, websocketpp::close::status::normal, "User closed connection", error);
    if (error) {
        log::error("no disconnection! {}", error.message());
    }
    handlers.clear();
}

TLSCtx NetworkManager::onTlsInit(Handle hdl) {
    auto ctx = TLSCtx(new asio::ssl::context(asio::ssl::context::tlsv12_client));

    ctx->set_verify_mode(asio::ssl::verify_peer);

    return ctx;
}

void NetworkManager::onOpen(Handle hdl) {
    this->hdl = hdl;
    isConnected = true;
    connCallback();
    log::info("heyy we connected!");
}

void NetworkManager::onClose(Handle hdl) {
    isConnected = false;
    auto connection = client.get_con_from_hdl(hdl);
    auto const localCode = connection->get_local_close_code();
	auto const remoteCode = connection->get_remote_close_code();

    log::info("oh we no longer connected! ({}/{})", localCode, remoteCode);
    log::info("because: {}/{}", connection->get_local_close_reason(), connection->get_remote_close_reason());
}

void NetworkManager::onFail(Handle hdl) {
    isConnected = false;
    auto connection = client.get_con_from_hdl(hdl);
    auto const localCode = connection->get_local_close_code();
	auto const remoteCode = connection->get_remote_close_code();

    log::info("FAIL. ({}/{})", localCode, remoteCode);
    log::info("because: {}/{}", connection->get_local_close_reason(), connection->get_remote_close_reason());
}

void NetworkManager::onMessage(Handle hdl, Client::message_ptr msg) {
    log::debug("{}", msg->get_payload());
    auto payloadJSON = matjson::parse(msg->get_payload());
    if (payloadJSON.isErr()) {
        log::error("unable to convert raw payload to json: {}", payloadJSON.unwrapErr());
        log::error("json: {}", msg->get_payload());
        return;
    }

    RecvMessage payload = payloadJSON.unwrap().as<RecvMessage>().unwrap();
    if (handlers.contains(payload.eventName)) {
        Loader::get()->queueInMainThread([this, payload] {
            for (auto handler : handlers[payload.eventName]) {
                handler.second(payload.payload);
            }
        });
    } else {
        log::debug("no handler for {} registered", payload.eventName);
    }
}

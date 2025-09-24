#pragma once
#undef _WINSOCKAPI_

#define _WEBSOCKETPP_CPP11_STL_
#define _WEBSOCKETPP_CPP11_INTERNAL_
#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>

#include <asio/ssl/context.hpp>

#include <Geode/loader/Mod.hpp>
using namespace geode;

#include <matjson.hpp>

template<typename T>
concept HasEventName = requires(T t) {
    { t.EVENT_NAME } -> std::convertible_to<std::string>;
};

#ifdef DEBUG_BUILD
using Client = websocketpp::client<websocketpp::config::asio_client>;
#else
using Client = websocketpp::client<websocketpp::config::asio_tls_client>;
#endif
using Handle = websocketpp::connection_hdl;
using TLSCtx = std::shared_ptr<asio::ssl::context>;

struct RecvMessage {
    std::string eventName;
    matjson::Value payload;
};

template <>
struct matjson::Serialize<RecvMessage> {
    static geode::Result<RecvMessage> fromJson(const matjson::Value& json) {
        GEODE_UNWRAP_INTO(std::string eventName, json["eventName"].asString());
        matjson::Value payload = json["payload"];
        return geode::Ok(RecvMessage { .eventName = eventName, .payload = payload });
    }
};

class NetworkManager {
protected:
    NetworkManager();

    Client client;
    Handle hdl;

    std::thread runThread;

    std::condition_variable heartbeatCondition;
    std::mutex heartbeatMutex;
    std::thread heartbeatThread;

    std::unordered_map<
        std::string,
        std::unordered_map<
            int,
            std::function<void(matjson::Value)>
        >
    > handlers = {};

    std::string token;

    std::function<void()> connCallback = nullptr;

    void init();

    // callbacks
    TLSCtx onTlsInit(Handle hdl);
    void onOpen(Handle hdl);
    void onMessage(Handle hdl, Client::message_ptr msg);
    void onFail(Handle hdl);
    void onClose(Handle hdl);

    void run();
    void heartbeat();
public:
    static NetworkManager& get() {
        static NetworkManager instance;
        return instance;
    }

    std::atomic_bool isConnected;
    std::atomic_bool isRunning;

    void connect(std::string token);
    void connect(std::string token, std::function<void()> callback);
    
    void disconnect();

    void close();

    void send(HasEventName auto& event) {
        // std::error_code error;
        matjson::Value payload = matjson::makeObject({
            { "payload", event },
            { "eventName", event.EVENT_NAME },
            { "token", token }
        });
        try {
            auto error = client.get_con_from_hdl(hdl)->send(payload.dump(), websocketpp::frame::opcode::text);
            if (error) {
                log::error("no message sent :( {}", error.message());
            }
        } catch (websocketpp::exception e) {
            log::error("{}", e.what());
        }

    }

    // i have no clue why intelisense dies for
    // these functions but alright
    template<HasEventName T>
    int on(std::function<void(T)> callback) {
        if (!handlers.contains(T::EVENT_NAME)) handlers[T::EVENT_NAME] = {};

        auto idx = handlers[T::EVENT_NAME].size();
        handlers[T::EVENT_NAME][idx] = [callback](matjson::Value json) {
            T ret = json.as<T>().unwrap();
            callback(ret);
        };

        log::debug("Created event listener {}", T::EVENT_NAME);

        return idx;
    }

    template<HasEventName T>
    int once(std::function<void(T)> callback) {
        auto id = std::make_shared<int>(-1);
        log::debug("Creating one time listener {}", T::EVENT_NAME);
        int real = this->on<T>([this, callback, id](T ev) {
            callback(ev);
            this->unbind<T>(*id);
        });
        *id = real;
        return real;
    }

    template<HasEventName T>
    void unbind(int idx) {
        handlers[T::EVENT_NAME].erase(idx);

        log::debug("Deleted event listener {}", T::EVENT_NAME);
    }
};

///
/// Created by Anonymous275 on 7/28/2020
///
#include "Client.hpp"
#include "Curl/Http.h"
#include "Logger.h"
#include "Security/Enc.h"
#include "Settings.h"
#include <chrono>
#include <future>
#include <sstream>
#include <thread>

void WebsocketInit();
std::string GetPlayers() {
    std::string Return;
    for (auto& c : CI->Clients) {
        if (c != nullptr) {
            Return += c->GetName() + ";";
        }
    }
    return Return;
}
std::string GenerateCall() {
    std::stringstream Ret;
    Ret << "uuid=" << Key << "&players=" << CI->Size()
        << "&maxplayers=" << MaxPlayers << "&port=" << Port
        << "&map=" << MapName << "&private=" << (Private ? "true" : "false")
        << "&version=" << GetSVer() << "&clientversion=" << GetCVer()
        << "&name=" << ServerName << "&pps=" << StatReport
        << "&modlist=" << FileList << "&modstotalsize=" << MaxModSize
        << "&modstotal=" << ModsLoaded << "&playerslist=" << GetPlayers()
        << "&desc=" << ServerDesc;
    return Ret.str();
}
std::string RunPromise(const std::string& IP, const std::string& R) {
    std::packaged_task<std::string()> task([&] { return PostHTTP(IP, R, false); });
    std::future<std::string> f1 = task.get_future();
    std::thread t(std::move(task));
    t.detach();
    auto status = f1.wait_for(std::chrono::seconds(10));
    if (status != std::future_status::timeout)
        return f1.get();
    error(Sec("Backend system Timeout please try again later"));
    std::this_thread::sleep_for(std::chrono::seconds(3));
    _Exit(0);
}

[[noreturn]] void Heartbeat() {
    DebugPrintTID();
    std::string R, T;
    bool isAuth = false;
    while (true) {
        R = GenerateCall();
        if (!CustomIP.empty())
            R += "&ip=" + CustomIP;
        std::string link ="https://beammp.com/heartbeatv2";
        T = RunPromise(link, R);

        if (T.substr(0,2) != "20") {
            //Backend system refused server startup!
            std::this_thread::sleep_for(std::chrono::seconds(10));
            std::string Backup = "https://backup1.beammp.com/heartbeatv2";
            T = RunPromise(Backup, R);
            if (T.substr(0,2) != "20") {
                warn("Backend system refused server! Server might not show in the public list");
            }
        }
        //Server Authenticated
        if (T.length() == 4)
            info(Sec("Server authenticated"));
        R.clear();
        T.clear();
        if (!isAuth) {
            WebsocketInit();
            isAuth = true;
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}
void HBInit() {
    std::thread HB(Heartbeat);
    HB.detach();
}

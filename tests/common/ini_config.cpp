#include <wui/config/config_impl_ini.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

// Run with a writable temporary filename. Exercise the real INI reader and
// writer: losing MeshNodes during either operation breaks the required RTC.
int main(int argc, char** argv)
{
    if (argc != 2) return 2;
    const std::filesystem::path path(argv[1]);
    const std::string nodes = R"([{"node_id":"local-rtc","address":"127.0.0.1","port":45010},{"node_id":"remote","address":"10.70.0.2","port":45010}])";
    {
        std::ofstream file(path);
        file << "# A comment containing [brackets]\n  [WebRTC]\nMeshNodes = " << nodes
             << "\nLocalAddress = 127.0.0.1\n[Network]\nPort = 443\n";
    }
    try
    {
        {
            wui::config::config_impl_ini config(path.string());
            if (config.get_string("WebRTC", "MeshNodes", "") != nodes ||
                config.get_string("WebRTC", "LocalAddress", "") != "127.0.0.1" ||
                config.get_int("Network", "Port", 0) != 443)
                throw std::runtime_error("JSON mesh or following INI settings were lost on read");
            config.set_int("Network", "Port", 8443);
        }
        wui::config::config_impl_ini saved(path.string());
        if (saved.get_string("WebRTC", "MeshNodes", "") != nodes ||
            saved.get_int("Network", "Port", 0) != 8443)
            throw std::runtime_error("JSON mesh was lost on config save/reload");
        std::filesystem::remove(path);
        std::cout << "INI mesh read and save/reload: passed\n";
    }
    catch (const std::exception& error)
    {
        std::filesystem::remove(path);
        std::cerr << error.what() << '\n';
        return 1;
    }
}

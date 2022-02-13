#include "driver/upgrade.hpp"
#include "driver/network.hpp"

#include <cstdlib>
#include <fstream>
#include <filesystem>

namespace driver {
auto upgrade(const iop::Network &network, const iop::StaticString path, const std::string_view authorization_header) noexcept -> iop::NetworkStatus {
    const auto variant = network.httpGet(path, authorization_header, "");
    if (const auto *response = std::get_if<iop::Response>(&variant)) {
        const auto &binary = response->payload();
        if (!binary) {
            network.logger().error(IOP_STATIC_STRING("Upgrade failed, no firmware returned"));
            return iop::NetworkStatus::BROKEN_SERVER;
        }

        const auto filename = std::filesystem::current_path();

        network.logger().info(IOP_STATIC_STRING("Upgrading binary file: "), std::string_view(filename.c_str()));

        // TODO: writing to yourself doesn't work everywhere, we need a better way
        std::ofstream file(filename);
        if (!file.is_open()) {
            network.logger().error(IOP_STATIC_STRING("Unable to open firmware file"));
            return iop::NetworkStatus::IO_ERROR;
        }

        file.write(binary->begin(), static_cast<std::streamsize>(binary->length()));
        if (file.fail()) {
            network.logger().error(IOP_STATIC_STRING("Unable to write to firmware file"));
            return iop::NetworkStatus::IO_ERROR;
        }

        file.close();
        if (file.fail()) {
            network.logger().error(IOP_STATIC_STRING("Unable to close firmware file"));
            return iop::NetworkStatus::IO_ERROR;
        }

        std::error_code code;
        // TODO: improve this perm
        std::filesystem::permissions(filename, std::filesystem::perms::owner_all | std::filesystem::perms::others_exec | std::filesystem::perms::others_read | std::filesystem::perms::group_exec | std::filesystem::perms::group_read, code);
        if (code) {
            network.logger().error(IOP_STATIC_STRING("Unable to set file as executable: "), std::to_string(code.value()));
            return iop::NetworkStatus::IO_ERROR;
        }

        network.logger().info(IOP_STATIC_STRING("Upgrading runtime"));
        exit(system(filename.c_str()));
    } else if (const auto *status = std::get_if<int>(&variant)) {
        network.logger().error(IOP_STATIC_STRING("Invalid status returned by the server on upgrade: "), std::to_string(*status));
    }

    return iop::NetworkStatus::BROKEN_SERVER;
}
} // namespace driver
#include "driver/upgrade.hpp"
#include "driver/network.hpp"
#include "driver/runtime.hpp"

#include <cstdlib>
#include <fstream>
#include <filesystem>

namespace driver {
auto upgrade(const iop::Network &network, const iop::StaticString path, const std::string_view authorization_header) noexcept -> iop::NetworkStatus {
    const auto variant = network.httpGet(path, authorization_header, "");
    if (const auto *response = std::get_if<iop::Response>(&variant)) {
        const auto &binary = response->payload();
        if (!binary) {
            network.logger().error(IOP_STR("Upgrade failed, no firmware returned"));
            return iop::NetworkStatus::BROKEN_SERVER;
        }

        // Possible race here, but it's the best cross platform solution
        const std::string tmpFile = std::tmpnam(nullptr);
        network.logger().info(IOP_STR("Upgrading binary file: "), tmpFile);

        std::ofstream file(tmpFile);
        if (!file.is_open()) {
            network.logger().error(IOP_STR("Unable to open firmware file"));
            return iop::NetworkStatus::IO_ERROR;
        }

        file.write(binary->begin(), static_cast<std::streamsize>(binary->length()));
        if (file.fail()) {
            network.logger().error(IOP_STR("Unable to write to firmware file"));
            return iop::NetworkStatus::IO_ERROR;
        }

        file.close();
        if (file.fail()) {
            network.logger().error(IOP_STR("Unable to close firmware file"));
            return iop::NetworkStatus::IO_ERROR;
        }

        std::error_code code;
        // TODO: improve this perm
        std::filesystem::permissions(tmpFile, std::filesystem::perms::owner_all | std::filesystem::perms::others_exec | std::filesystem::perms::others_read | std::filesystem::perms::group_exec | std::filesystem::perms::group_read, code);
        if (code) {
            network.logger().error(IOP_STR("Unable to set file as executable: "), std::to_string(code.value()));
            return iop::NetworkStatus::IO_ERROR;
        }

        // FIXME: Race here
        const auto filename = std::filesystem::current_path().append(driver::execution_path());
        const auto renamedBinary = std::filesystem::current_path().append(std::string(driver::execution_path()).append(std::string_view(".old")));
        std::filesystem::rename(filename, renamedBinary);
        std::filesystem::rename(tmpFile, filename);
        std::filesystem::remove(renamedBinary);

        network.logger().info(IOP_STR("Upgrading runtime"));
        exit(system(filename.string().c_str()));
    } else if (const auto *status = std::get_if<int>(&variant)) {
        network.logger().error(IOP_STR("Invalid status returned by the server on upgrade: "), std::to_string(*status));
    }

    return iop::NetworkStatus::BROKEN_SERVER;
}
} // namespace driver
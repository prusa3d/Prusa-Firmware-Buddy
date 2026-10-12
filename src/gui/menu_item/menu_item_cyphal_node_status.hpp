/// @file
#pragma once

#include <WindowItemFormatableLabel.hpp>
#include <xbuddy_extension/shared_enums.hpp>

#include <optional>

struct CyphalNodeStatus {
    xbuddy_extension::NodeState state;
    /// Only while the node's own firmware is being flashed
    std::optional<uint8_t> percent;
    bool operator==(const CyphalNodeStatus &) const = default;
};

/// Live state of a cyphal node, including the progress of its firmware update
class MenuItemCyphalNodeStatus : public MenuItemAutoUpdatingLabel<CyphalNodeStatus> {
public:
    MenuItemCyphalNodeStatus(const string_view_utf8 &label, xbuddy_extension::NodeName node_name);

private:
    CyphalNodeStatus get_status() const;
    void print_val(const std::span<char> &buffer) const;

    const xbuddy_extension::NodeName node_name_;
};

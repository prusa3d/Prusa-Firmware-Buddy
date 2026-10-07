/// @file
#pragma once

#include <basic_screen_menu.hpp>
#include <MItem_menus.hpp>
#include <WindowItemFormatableLabel.hpp>
#include <WindowMenuInfo.hpp>

class MI_CYPHAL_INFO final : public MI_SCREEN_BASE {
public:
    MI_CYPHAL_INFO();
};

class MI_CYPHAL_NODE final : public IWindowMenuItem {
public:
    explicit MI_CYPHAL_NODE(uint8_t node_id);

protected:
    void click(IWindowMenu &) final;

private:
    const uint8_t node_id;
};

class MI_CYPHAL_NODE_ID final : public WI_FORMATABLE_LABEL_t<uint8_t> {
public:
    MI_CYPHAL_NODE_ID();
};

class MI_CYPHAL_NODE_STATE final : public WiInfoString {
public:
    MI_CYPHAL_NODE_STATE();

    void set_node_id(uint8_t);

    void Loop() override;

private:
    uint8_t node_id = 0;
};

using ScreenMenuCyphalNodeBase = BasicScreenMenu<MI_CYPHAL_NODE_ID, MI_CYPHAL_NODE_STATE>;

class ScreenMenuCyphalNode final : public ScreenMenuCyphalNodeBase {
public:
    explicit ScreenMenuCyphalNode(uint8_t node_id);
};

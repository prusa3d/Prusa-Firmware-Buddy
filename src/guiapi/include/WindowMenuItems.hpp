#pragma once

#include "IWindowMenuItem.hpp"

#pragma pack(push, 1)
//WI_LABEL
//where all values are divided by 1000
class WI_LABEL_t : public IWindowMenuItem {
public:
    WI_LABEL_t(window_menu_t &window_menu, const char *label, uint16_t id_icon = 0, bool enabled = true, bool hidden = false)
        : IWindowMenuItem(label, id_icon, enabled, hidden) {}

    virtual int OnClick() { return 0; } //to match template signature
    virtual bool Change(int dif);
};

//WI_SPIN
//where all values are divided by 1000
class WI_SPIN_t : public IWindowMenuItem {
public: //todo private
    int32_t value;
    const int32_t *range;

public:
    WI_SPIN_t(window_menu_t &window_menu, int32_t value, const int32_t *range, const char *label, uint16_t id_icon = 0, bool enabled = true, bool hidden = false);
    virtual bool Change(int dif);
};

//WI_SPIN_FL
class WI_SPIN_FL_t : public IWindowMenuItem {
public: //todo private
    float value;
    const float *range;
    const char *prt_format;

public:
    WI_SPIN_FL_t(window_menu_t &window_menu, float value, const float *range, const char *prt_format, const char *label, uint16_t id_icon = 0, bool enabled = true, bool hidden = false);
    virtual bool Change(int dif);
};

//WI_SWITCH
//array of char strings ended by NULL for array length variability.
//char * strings[3] = {"Low", "High", "Medium", NULL}
class WI_SWITCH_t : public IWindowMenuItem {
public: //todo private
    uint32_t index;
    const char **strings;

public:
    WI_SWITCH_t(window_menu_t &window_menu, int32_t index, const char **strings, const char *label, uint16_t id_icon = 0, bool enabled = true, bool hidden = false);
    virtual bool Change(int dif);
};

//WI_SELECT
//array of char strings ended by NULL for array length variability.
//char * strings[3] = {"Low", "High", "Medium", NULL}
class WI_SELECT_t : public IWindowMenuItem {
public: //todo private
    uint32_t index;
    const char **strings;

public:
    WI_SELECT_t(window_menu_t &window_menu, int32_t index, const char **strings, const char *label, uint16_t id_icon, bool enabled = true, bool hidden = false);
    virtual bool Change(int dif);
};

/*
class WindowMenuItem {
    using mem_space = std::aligned_union<0, IWindowMenuItem, WI_LABEL_t, WI_SPIN_t, WI_SPIN_FL_t, WI_SWITCH_t, WI_SELECT_t>::type;
    mem_space data_mem;
public:
    WindowMenuItem(const WI_LABEL_t& label){}//{ ::new (static_cast<void*>(std::addressof(data_mem))) WI_LABEL_t(label);}
    WindowMenuItem(const WI_SPIN_t& wi_spin){}//{ new (pdata) WI_SPIN_t (wi_spin); }
    WindowMenuItem(const WI_SPIN_FL_t& wi_spin_fl){}//{ new WI_SPIN_FL_t(wi_spin_fl);}
    WindowMenuItem(const WI_SWITCH_t& wi_switch){}
    WindowMenuItem(const WI_SELECT_t& wi_select){}



    //recall virtual functions of IWindowMenuItem
    void Enable() { Get().Enable(); }
    void Disable() { Get().Disable(); }
    bool IsEnabled() const { return Get().IsEnabled(); }
    void SetHidden() { Get().SetHidden(); }
    void SetNotHidden() { Get().SetNotHidden(); }
    bool IsHidden() const { return Get().IsHidden(); }
    void SetIconId(uint16_t id){Get().SetIconId(id);}
    uint16_t GetIconId() const{ return Get().GetIconId(); }
    void SetLabel(const char* text) {Get().SetLabel(text);}
    const char* GetLabel() const {return Get().GetLabel();}
    void Change(int dif){ Get().Change(dif); };

    IWindowMenuItem &Get(){return *reinterpret_cast<IWindowMenuItem*>(&data_mem);}
    const IWindowMenuItem &Get()const {return *reinterpret_cast<const IWindowMenuItem*>(&data_mem);}
};

typedef void(window_menu_items_t)(window_menu_t *pwindow_menu,
    uint16_t index, WindowMenuItem **ppitem, void *data);

*/

/*****************************************************************************/
//advanced types
class MI_RETURN : public WI_LABEL_t {
    static const char *const label;

public:
    MI_RETURN(window_menu_t &window_menu);
    virtual int OnClick();
};

class MI_FILAMENT : public WI_LABEL_t {
    static const char *const label;

public:
    MI_FILAMENT(window_menu_t &window_menu);
    virtual int OnClick();
};

class MI_LAN_SETTINGS : public WI_LABEL_t {
    static const char *const label;

public:
    MI_LAN_SETTINGS(window_menu_t &window_menu);
    virtual int OnClick();
};

class MI_VERSION_INFO : public WI_LABEL_t {
    static const char *const label;

public:
    MI_VERSION_INFO(window_menu_t &window_menu);
    virtual int OnClick();
};

#pragma pack(pop)

#pragma once

#include "IWindowMenuItem.hpp"

#pragma pack(push, 1)
//WI_LABEL
class WI_LABEL_t : public IWindowMenuItem {
public:
    WI_LABEL_t(const char *label, uint16_t id_icon = 0, bool enabled = true, bool hidden = false);
    virtual bool Change(int dif);
};

//WI_SPIN
//where all values are divided by 1000
class WI_SPIN_t : public IWindowMenuItem {
public: //todo private
    int32_t value;
    const int32_t *range;

public:
    WI_SPIN_t(int32_t value, const int32_t *range, const char *label, uint16_t id_icon = 0, bool enabled = true, bool hidden = false);
    virtual bool Change(int dif);
};

//WI_SPIN_FL
class WI_SPIN_FL_t : public IWindowMenuItem {
public: //todo private
    float value;
    const float *range;
    const char *prt_format;

public:
    WI_SPIN_FL_t(float value, const float *range, const char *prt_format, const char *label, uint16_t id_icon = 0, bool enabled = true, bool hidden = false);
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
    WI_SWITCH_t(int32_t index, const char **strings, const char *label, uint16_t id_icon = 0, bool enabled = true, bool hidden = false);
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
    WI_SELECT_t(int32_t index, const char **strings, const char *label, uint16_t id_icon, bool enabled = true, bool hidden = false);
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
    static constexpr const char *const label = "Return";

public:
    MI_RETURN();
    virtual void Click(Iwindow_menu_t &window_menu);
};

class MI_FILAMENT : public WI_LABEL_t {
    static constexpr const char *const label = "Filament";

public:
    MI_FILAMENT();
};

class MI_LAN_SETTINGS : public WI_LABEL_t {
    static constexpr const char *const label = "Lan settings";

public:
    MI_LAN_SETTINGS();
};

class MI_VERSION_INFO : public WI_LABEL_t {
    static constexpr const char *const label = "Version Info";

public:
    MI_VERSION_INFO();
    virtual void Click(Iwindow_menu_t &window_menu);
};

class MI_SYS_INFO : public WI_LABEL_t {
    static constexpr const char *const label = "System Info";

public:
    MI_SYS_INFO();
    virtual void Click(Iwindow_menu_t &window_menu);
};

class MI_STATISTIC_disabled : public WI_LABEL_t {
    static constexpr const char *const label = "Statistic";

public:
    MI_STATISTIC_disabled();
    virtual void Click(Iwindow_menu_t &window_menu) {}
};

class MI_FAIL_STAT_disabled : public WI_LABEL_t {
    static constexpr const char *const label = "Fail Stats";

public:
    MI_FAIL_STAT_disabled();
    virtual void Click(Iwindow_menu_t &window_menu) {}
};

class MI_SUPPORT_disabled : public WI_LABEL_t {
    static constexpr const char *const label = "Support";

public:
    MI_SUPPORT_disabled();
    virtual void Click(Iwindow_menu_t &window_menu) {}
};

class MI_QR_test : public WI_LABEL_t {
    static constexpr const char *const label = "QR test";

public:
    MI_QR_test();
    virtual void Click(Iwindow_menu_t &window_menu);
};

class MI_QR_info : public WI_LABEL_t {
    static constexpr const char *const label = "Send Info by QR";

public:
    MI_QR_info();
    virtual void Click(Iwindow_menu_t &window_menu);
};
#pragma pack(pop)

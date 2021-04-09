// window_filter.hpp

#pragma once

#include "window.hpp"

//inherit, use ctor ti pass additional param
class WinFilter {
public:
    virtual bool operator()(const window_t &) const = 0;
};

//dummy filter returns always true
class WinFilterTrue : public WinFilter {
public:
    virtual bool operator()(const window_t &) const override { return true; };
};

//filter windows contained in given rectangle
class WinFilterContained : public WinFilter {
    Rect16 rect;

public:
    constexpr WinFilterContained(Rect16 rc)
        : rect(rc) {}
    virtual bool operator()(const window_t &win) const override {
        return rect.Contain(win.rect);
    }
};

//filter popup windows
class WinFilterPopUp : public WinFilter {
public:
    virtual bool operator()(const window_t &win) const override { return win.GetType() == win_type_t::popup; };
};

class WinFilterIntersectingPopUp : public WinFilter {
    Rect16 rect;

public:
    constexpr WinFilterIntersectingPopUp(Rect16 rc)
        : rect(rc) {}
    virtual bool operator()(const window_t &win) const override {
        return ((win.GetType() == win_type_t::popup) && rect.HasIntersection(win.rect));
    };
};

class WinFilterIntersectingNonPopUp : public WinFilter {
    Rect16 rect;

public:
    constexpr WinFilterIntersectingNonPopUp(Rect16 rc)
        : rect(rc) {}
    virtual bool operator()(const window_t &win) const override {
        return ((win.GetType() != win_type_t::popup) && rect.HasIntersection(win.rect));
    };
};

//filter dialog windows
class WinFilterDialog : public WinFilter {
public:
    virtual bool operator()(const window_t &win) const override { return win.IsDialog(); };
};

//filter dialog windows
class WinFilterDialogNonStrong : public WinFilter {
public:
    virtual bool operator()(const window_t &win) const override { return win.GetType() == win_type_t::dialog; };
};

class WinFilterIntersectingDialog : public WinFilter {
    Rect16 rect;

public:
    constexpr WinFilterIntersectingDialog(Rect16 rc)
        : rect(rc) {}
    virtual bool operator()(const window_t &win) const override {
        return (win.IsDialog() && rect.HasIntersection(win.rect));
    };
};

//filter strong dialog windows
class WinFilterStrongDialog : public WinFilter {
public:
    virtual bool operator()(const window_t &win) const override { return win.GetType() == win_type_t::strong_dialog; };
};

//filter normal windows
class WinFilterNormal : public WinFilter {
public:
    virtual bool operator()(const window_t &win) const override { return win.GetType() == win_type_t::normal; };
};

//filters without window type
class WinFilterVisible : public WinFilter {
public:
    virtual bool operator()(const window_t &win) const override { return win.IsVisible(); };
};

class WinFilterIntersectingVisible : public WinFilter {
    Rect16 rect;

public:
    constexpr WinFilterIntersectingVisible(Rect16 rc)
        : rect(rc) {}
    virtual bool operator()(const window_t &win) const override {
        return (win.IsVisible() && rect.HasIntersection(win.rect));
    };
};

class WinFilterCapturable : public WinFilter {
public:
    virtual bool operator()(const window_t &win) const override { return win.IsCapturable(); };
};

//filter dialog or popup windows
class WinFilterDialogOrPopUp : public WinFilter {
public:
    virtual bool operator()(const window_t &win) const override { return win.IsDialog() || win.GetType() == win_type_t::popup; };
};

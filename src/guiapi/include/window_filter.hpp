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

//filter dialog windows
class WinFilterDialog : public WinFilter {
public:
    virtual bool operator()(const window_t &win) const override { return win.IsDialog(); };
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

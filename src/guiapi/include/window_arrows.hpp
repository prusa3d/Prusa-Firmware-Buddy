
class WindowArrows : public window_aligned_t {
    static const uint16_t id_res_ok;
    static const uint16_t id_res_ng;

public:
    WindowArrows(window_t *parent, point_i16_t pt, padding_ui8_t padding = { 0, 0, 0, 0 });
    enum class State_t : uint8_t { undef,
        ok,
        ng };
    State_t GetState() const;
    void SetState(State_t s);

protected:
    virtual void unconditionalDraw() override;
};

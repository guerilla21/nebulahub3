#pragma once
#include "../nebula.h"
#include <orbis/Pad.h>
#include <orbis/UserService.h>

class Controller {
public:
    explicit Controller(OrbisUserServiceUserId userId);
    ~Controller();

    void poll();

    bool pressed(uint32_t btn)  const; // true only on frame btn went down
    bool held(uint32_t btn)     const; // true while held
    bool released(uint32_t btn) const;

    // Analog sticks — range -1.0 … 1.0
    float leftX()  const;
    float leftY()  const;
    float rightX() const;
    float rightY() const;

    // Repeat helper (for scrolling — fires pressed on first, then every 150ms)
    bool repeat(uint32_t btn) const;

private:
    int            padHandle_ = -1;
    OrbisPadData   cur_{};
    OrbisPadData   prev_{};
    uint64_t       heldSince_[32]{};

    static int btnIndex(uint32_t btn);
};

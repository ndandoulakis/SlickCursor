#ifndef EMUPARAMS_H
#define EMUPARAMS_H

struct EmuParams {
    // Parameters
    bool useArrows;
    bool useNumpad;

    enum KEY {
        UP, DOWN, CLEAR, LEFT, RIGHT, LSHIFT, KEY_COUNT
    };

    bool keys[KEY_COUNT];

    EmuParams() {
        useArrows = false;
        useNumpad = true;
    }

    bool up() const {
        return keys[UP];
    }

    void setUp(bool pressed) {
        keys[UP] = pressed;
    }

    bool down() const {
        return keys[DOWN] || keys[CLEAR];
    }

    void setDown(bool pressed) {
        keys[DOWN] = pressed;
    }

    void setClear(bool pressed) {
        keys[CLEAR] = pressed;
    }

    bool left() const {
        return keys[LEFT];
    }

    void setLeft(bool pressed) {
        keys[LEFT] = pressed;
    }

    bool right() const {
        return keys[RIGHT];
    }

    void setRight(bool pressed) {
        keys[RIGHT] = pressed;
    }

    bool lShift() const {
        return keys[LSHIFT];
    }

    void setLShift(bool pressed) {
        keys[LSHIFT] = pressed;
    }

    bool arePressedKeys() const {
        for (int i=0; i<LSHIFT; i++) {
            if (keys[i]) return true;
        }
        return false;
    }

    bool sameKeyState(const EmuParams & other) const {
        for (int i=0; i<LSHIFT; i++) {
            if (keys[i] != other.keys[i]) return false;
        }
        return true;
    }

};

#endif

#ifndef NAVKEYS_H
#define NAVKEYS_H

struct NavKeys {
    enum KEY {
        UP, DOWN, CLEAR, LEFT, RIGHT, KEY_COUNT
    };

    bool keys[KEY_COUNT];

    bool up() {
        return keys[UP];
    }

    void setUp(bool pressed) {
        keys[UP] = pressed;
    }

    bool down() {
        return keys[DOWN] || keys[CLEAR];
    }

    void setDown(bool pressed) {
        keys[DOWN] = pressed;
    }

    void setClear(bool pressed) {
        keys[CLEAR] = pressed;
    }

    bool left() {
        return keys[LEFT];
    }

    void setLeft(bool pressed) {
        keys[LEFT] = pressed;
    }

    bool right() {
        return keys[RIGHT];
    }

    void setRight(bool pressed) {
        keys[RIGHT] = pressed;
    }

    bool arePressedKeys() {
        int cnt = sizeof(keys) / sizeof(keys[0]);
        for (int i=0; i<cnt; i++) {
            if (keys[i]) return true;
        }
        return false;
    }

    bool operator== (const NavKeys & other) const {
        int cnt = sizeof(keys) / sizeof(keys[0]);
        for (int i=0; i<cnt; i++) {
            if (keys[i] != other.keys[i]) return false;
        }
        return true;
    }

    bool operator!= (const NavKeys & other) const {
        return !(*this == other);
    }

};

#endif

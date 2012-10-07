#ifndef EMUPARAMS_H
#define EMUPARAMS_H

#include <stdio.h>
#include "windows.h"

struct EmuParams {
    // Parameters
    bool useExtended;
    bool useNumpad;
    bool useScreenAnalysis;

    enum KEY {
        UP, DOWN, CLEAR, LEFT, RIGHT, LSHIFT, KEY_COUNT
    };

    bool keys[KEY_COUNT];

    EmuParams() {
        useExtended = true;
        useNumpad = true;
        useScreenAnalysis = true;
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

    void LoadParams(const char * appName) {
        useExtended = GetProfileInt(appName, "useExtended", useExtended);
        useNumpad = GetProfileInt(appName, "useNumpad", useNumpad);
        useScreenAnalysis = GetProfileInt(appName, "useScreenAnalysis", useScreenAnalysis);
    }

    void SaveParams(const char * appName) const {
        // Note that WriteProfileString doesn't
        // work while the OS is shutting down.

        char buf[50];

        sprintf(buf, "%d", useExtended);
        WriteProfileString(appName, "useExtended", buf);

        sprintf(buf, "%d", useNumpad);
        WriteProfileString(appName, "useNumpad", buf);

        sprintf(buf, "%d", useScreenAnalysis);
        WriteProfileString(appName, "useScreenAnalysis", buf);
    }

};

#endif

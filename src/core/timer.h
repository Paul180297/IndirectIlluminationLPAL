#pragma once

#include "core/common.h"

class GLtimer {
public:
    GLtimer();

    void start();
    void end();
    void reset();
    double getDuration();
    double getDuration(int frameNum);
    void showDuration();
    void showDuration(int frameNum);
private:
    unsigned int m_queryID[2];
    GLint64 m_start;
    GLint64 m_end;
    GLint m_endAvailable;
    double m_duration = 0.0;
    int m_frameIndex = 0;
};

#include "timer.h"

GLtimer::GLtimer() {
    glGenQueries(2, m_queryID);

}

void GLtimer::start() {
    glQueryCounter(m_queryID[0], GL_TIMESTAMP);
    m_frameIndex++;
}

void GLtimer::end() {
    glQueryCounter(m_queryID[1], GL_TIMESTAMP);

    GLint stopTimerAvailable = 0;
    while (!stopTimerAvailable) {
        glGetQueryObjectiv(m_queryID[1], GL_QUERY_RESULT_AVAILABLE, &stopTimerAvailable);
    }

    glGetQueryObjecti64v(m_queryID[0], GL_QUERY_RESULT, &m_start);
    glGetQueryObjecti64v(m_queryID[1], GL_QUERY_RESULT, &m_end);

    m_duration += (m_end - m_start) / 1000000.0; // in milliseconds
}

void GLtimer::reset() {
    m_duration = 0;
    m_frameIndex = 0;
}

double GLtimer::getDuration() {
    double tmp = m_duration;
    reset();
    return tmp;
}

double GLtimer::getDuration(int frameNum) {
    double tmp = m_duration / frameNum;
    reset();
    return tmp;
}

void GLtimer::showDuration() {
    printf("GL timer duration : %f [ms]\n", m_duration);
    reset();
}

void GLtimer::showDuration(int frameNum) {
    if (m_frameIndex % frameNum == 0) {
        double aveDuration = m_duration / frameNum;
        printf("GLtimer aveDuration for %d frames : %lf [ms]\n", frameNum, aveDuration);
        printf("GLtimer aveFPS for %d frames : %lf [fps]\n", frameNum, 1000.0/aveDuration);
        reset();
    }
}

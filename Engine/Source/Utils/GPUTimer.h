#pragma once
#include <glad/gl.h>
#include "Core/Log.h"

namespace rv {

class GPUTimer {
public:
    GPUTimer(const char* name) : name(name) {
        glGenQueries(1, &query);
        glBeginQuery(GL_TIME_ELAPSED, query);
    }

    ~GPUTimer() {
        glEndQuery(GL_TIME_ELAPSED);

        GLuint64 timeNs = 0;
        glGetQueryObjectui64v(query, GL_QUERY_RESULT, &timeNs);
        double ms = timeNs / 1'000'000.0;

        LOG_INFO("{} : {:.3f} ms", name, ms);

        glDeleteQueries(1, &query);
    }

private:
    GLuint query;
    const char* name;
};

}
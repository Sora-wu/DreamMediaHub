//
// Author: sora
// Mail: sora-wu@foxmail.com
//

#pragma once

#include <cmath>

class Clock {
public:
    void initialize();

private:
    double pts_ = NAN;          // 单位：毫秒
    double speed_ = 1.0;
    bool paused_ = true;
};
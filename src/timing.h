#ifndef TIME_H
#define TIME_H

#include "scalar.h"
#include "script_export.h"

SCRIPT(time,

       extern Scalar timing_dt;

    )

void timing_update();

#endif

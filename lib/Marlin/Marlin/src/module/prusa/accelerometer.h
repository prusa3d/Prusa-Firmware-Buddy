/**
 * @file
 */
#pragma once
#include "../../inc/MarlinConfigPre.h"
#if ENABLED(ACCELEROMETER)

    #if ENABLED(LOCAL_ACCELEROMETER)
        #include "SparkFunLIS2DH.h"
    #elif ENABLED(REMOTE_ACCELEROMETER)
        #include "freertos_mutex.hpp"
        #include "circle_buffer.hpp"
        #include <puppies/fifo_coder.hpp>
    #endif

class PrusaAccelerometer {
public:
    #if ENABLED(LOCAL_ACCELEROMETER)
    using Acceleration = Fifo::Acceleration;
    #else
    struct Acceleration {
        float val[3];
    };
    #endif

    enum class Error {
        none,
        communication,
        no_active_tool,
        busy,
    };

    PrusaAccelerometer();
    ~PrusaAccelerometer();

    void clear();
    int get_sample(Acceleration &acceleration);

    #if ENABLED(REMOTE_ACCELEROMETER)
    static void put_sample(common::puppies::fifo::AccelerometerXyzSample sample);
    #endif
private:
    Error m_error;
    #if ENABLED(LOCAL_ACCELEROMETER)
    Fifo m_fifo;
    #elif ENABLED(REMOTE_ACCELEROMETER)
    //Mutex is very RAM (80B) consuming for this fast operation, consider switching to critical section
    static FreeRTOS_Mutex s_buffer_mutex;
    using Sample_buffer = CircleBuffer<common::puppies::fifo::AccelerometerXyzSample, 32>;
    static Sample_buffer *s_sample_buffer;
    Sample_buffer m_sample_buffer;
    #endif
};
#endif

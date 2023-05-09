#include <iostream>
#include <cmath>
#include <chrono>
#include <thread>
#include <vector>
#include <mutex>

const int BLOCK_SIZE = 480;
const int SAMPLE_RATE = 48000;
const float PI = 3.14;

class SineWaveGenerator {
public:
    SineWaveGenerator(float frequency) : frequency(frequency), phase(0.0f) {}

    void generateBlock(float* buffer) {
        for (int i = 0; i < BLOCK_SIZE; ++i) {
            buffer[i] = std::sin(phase);
            phase += 2.0f * PI * frequency / SAMPLE_RATE;
            if (phase >= 2.0f * PI) {
                phase -= 2.0f * PI;
            }
        }
    }

private:
    float frequency;
    float phase;
};

class Processor {
public:
    virtual void processBlock(float* buffer) = 0;
};

class StaticGainControl : public Processor {
public:
    StaticGainControl(float gain) : gain(gain) {}

    void processBlock(float* buffer) {
        //std::cout << "Gain" << std::endl;
        for (int i = 0; i < BLOCK_SIZE; ++i) {
            buffer[i] *= gain;
        }
    }

private:
    float gain;
};

class Delay : public Processor {
public:
    Delay(int delay_size) : delay_line(delay_size, 0.0f) {}

    void processBlock(float* buffer) {
        //std::cout << "Delay" << std::endl;
        int delay_index = 0;
        for (int i = 0; i < BLOCK_SIZE; ++i) {
            float delayedSample = delay_line[i];
            delay_line[delay_index] = buffer[i];
            buffer[i] = delayedSample;
            delay_index = (delay_index + 1) % delay_line.size();
        }
    }

private:
    std::vector<float> delay_line;
};

int main() {
    SineWaveGenerator sine_wave(1000.0f);

    std::vector<Processor*> processors;

    processors.push_back(new StaticGainControl(2.0f));
    processors.push_back(new Delay(4800));

    // output format settings
    std::cout.precision(8);
    std::cout.setf(std::ios::fixed);

    while (true) {
        float buffer[BLOCK_SIZE];
        sine_wave.generateBlock(buffer);

        // threaded processing
        std::mutex mutex;

        std::vector<std::thread> threads;

        for (auto& it : processors) {
            threads.push_back(std::thread([&]() {
                std::lock_guard<std::mutex> lock(mutex);
                it->processBlock(buffer);
                }));
        }

        for (auto& it : threads) {
            it.join();
        }


        // output samples
        for (int i = 0; i < BLOCK_SIZE; ++i) {
            std::cout << buffer[i] << std::endl;
        }
        //std::cout << "new loop" << std::endl;

        // wait for the next block
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    return 0;
}
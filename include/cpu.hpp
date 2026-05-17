#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

using namespace std;

#include <vector>

class CPU
{
public:
    static constexpr size_t kMemorySize = 65536;

    CPU();

    void reset();
    bool loadProgram(const vector<uint8_t> &program, uint16_t baseAddress = 0);

    bool step();
    bool run(size_t maxSteps = 1000000);

    void setTraceEnabled(bool enabled);
    bool isTraceEnabled() const;

    void setRegister(size_t index, uint8_t value);
    uint8_t getRegister(size_t index) const;

    uint8_t getA() const;
    uint16_t getPC() const;
    uint64_t getCycles() const;
    bool isRunning() const;

    const string &getLastError() const;

private:
    array<uint8_t, kMemorySize> memory_{};
    uint8_t A_ = 0;
    uint8_t B_ = 0;
    array<uint8_t, 8> R_{};
    uint16_t PC_ = 0;
    uint64_t cycles_ = 0;
    bool running_ = false;
    bool traceEnabled_ = false;
    string lastError_;

    bool requireBytes(uint16_t address, size_t count) const;
    void setError(const string &message);
    void traceStep(uint16_t pc, uint8_t opcode, const string &mnemonic) const;
};

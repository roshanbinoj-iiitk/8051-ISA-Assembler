#include "cpu.hpp"

#include <iomanip>
#include <iostream>
#include <sstream>

namespace
{
    std::string formatHex(uint32_t value, int width)
    {
        std::ostringstream oss;
        oss << "0x" << std::uppercase << std::hex << std::setw(width) << std::setfill('0') << value;
        return oss.str();
    }
} // namespace

CPU::CPU()
{
    reset();
}

void CPU::reset()
{
    memory_.fill(0);
    A_ = 0;
    B_ = 0;
    R_.fill(0);
    PC_ = 0;
    cycles_ = 0;
    running_ = false;
    lastError_.clear();
}

bool CPU::loadProgram(const std::vector<uint8_t> &program, uint16_t baseAddress)
{
    reset();

    if (program.empty())
    {
        setError("Program is empty.");
        return false;
    }

    const std::size_t start = static_cast<std::size_t>(baseAddress);
    if (start + program.size() > memory_.size())
    {
        setError("Program does not fit in memory.");
        return false;
    }

    for (std::size_t i = 0; i < program.size(); ++i)
    {
        memory_[start + i] = program[i];
    }

    PC_ = baseAddress;
    running_ = true;
    lastError_.clear();
    return true;
}

bool CPU::step()
{
    if (!running_)
    {
        setError("CPU is not running.");
        return false;
    }

    if (!requireBytes(PC_, 1))
    {
        setError("Program counter points outside memory.");
        return false;
    }

    const uint16_t currentPC = PC_;
    const uint8_t opcode = memory_[currentPC];
    std::string mnemonic;

    // Fetch-Decode-Execute cycle:
    // 1) FETCH opcode at PC
    // 2) DECODE via opcode switch
    // 3) EXECUTE semantics and update PC/cycles
    switch (opcode)
    {
    case 0x00:
    {
        mnemonic = "NOP";
        PC_ = static_cast<uint16_t>(currentPC + 1);
        cycles_ += 1;
        break;
    }
    case 0x74:
    {
        if (!requireBytes(currentPC, 2))
        {
            setError("MOV A,#imm8 needs one immediate byte.");
            return false;
        }
        const uint8_t imm = memory_[currentPC + 1];
        A_ = imm;
        mnemonic = "MOV A,#imm8";
        PC_ = static_cast<uint16_t>(currentPC + 2);
        cycles_ += 1;
        break;
    }
    case 0x80:
    {
        if (!requireBytes(currentPC, 2))
        {
            setError("SJMP needs one relative offset byte.");
            return false;
        }
        const int8_t rel = static_cast<int8_t>(memory_[currentPC + 1]);
        const int32_t nextPC = static_cast<int32_t>(currentPC) + 2;
        const int32_t target = nextPC + rel;
        if (target < 0 || target >= static_cast<int32_t>(kMemorySize))
        {
            setError("SJMP target is outside memory.");
            return false;
        }
        mnemonic = "SJMP rel8";
        PC_ = static_cast<uint16_t>(target);
        cycles_ += 2;
        break;
    }
    case 0xFF:
    {
        mnemonic = "HLT";
        PC_ = static_cast<uint16_t>(currentPC + 1);
        cycles_ += 1;
        running_ = false;
        break;
    }
    default:
    {
        if (opcode >= 0x28 && opcode <= 0x2F)
        {
            const std::size_t index = static_cast<std::size_t>(opcode - 0x28);
            A_ = static_cast<uint8_t>(A_ + R_[index]);
            mnemonic = "ADD A,R" + std::to_string(index);
            PC_ = static_cast<uint16_t>(currentPC + 1);
            cycles_ += 1;
            break;
        }

        std::ostringstream oss;
        oss << "Unknown opcode " << formatHex(opcode, 2)
            << " at PC=" << formatHex(currentPC, 4);
        setError(oss.str());
        return false;
    }
    }

    // Opcode handling keeps behavior explicit and easy to extend incrementally.
    if (traceEnabled_)
    {
        traceStep(currentPC, opcode, mnemonic);
    }

    return true;
}

bool CPU::run(std::size_t maxSteps)
{
    if (!running_)
    {
        running_ = true;
    }

    std::size_t executed = 0;
    while (running_ && executed < maxSteps)
    {
        if (!step())
        {
            return false;
        }
        ++executed;
    }

    if (running_ && executed >= maxSteps)
    {
        setError("Execution stopped: max step count reached.");
        return false;
    }

    return lastError_.empty();
}

void CPU::setTraceEnabled(bool enabled)
{
    traceEnabled_ = enabled;
}

bool CPU::isTraceEnabled() const
{
    return traceEnabled_;
}

void CPU::setRegister(std::size_t index, uint8_t value)
{
    if (index < R_.size())
    {
        R_[index] = value;
    }
}

uint8_t CPU::getRegister(std::size_t index) const
{
    if (index < R_.size())
    {
        return R_[index];
    }
    return 0;
}

uint8_t CPU::getA() const
{
    return A_;
}

uint16_t CPU::getPC() const
{
    return PC_;
}

uint64_t CPU::getCycles() const
{
    return cycles_;
}

bool CPU::isRunning() const
{
    return running_;
}

const std::string &CPU::getLastError() const
{
    return lastError_;
}

bool CPU::requireBytes(uint16_t address, std::size_t count) const
{
    const std::size_t start = static_cast<std::size_t>(address);
    return start + count <= memory_.size();
}

void CPU::setError(const std::string &message)
{
    running_ = false;
    lastError_ = message;
}

void CPU::traceStep(uint16_t pc, uint8_t opcode, const std::string &mnemonic) const
{
    std::cout << "PC=" << formatHex(pc, 4)
              << " OP=" << formatHex(opcode, 2)
              << " " << mnemonic
              << " | A=" << formatHex(A_, 2)
              << " B=" << formatHex(B_, 2)
              << " R0=" << formatHex(R_[0], 2)
              << " R1=" << formatHex(R_[1], 2)
              << " R2=" << formatHex(R_[2], 2)
              << " R3=" << formatHex(R_[3], 2)
              << " R4=" << formatHex(R_[4], 2)
              << " R5=" << formatHex(R_[5], 2)
              << " R6=" << formatHex(R_[6], 2)
              << " R7=" << formatHex(R_[7], 2)
              << " Cycles=" << cycles_
              << '\n';
}

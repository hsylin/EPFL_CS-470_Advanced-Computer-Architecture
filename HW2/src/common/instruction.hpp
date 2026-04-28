#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

enum class instruction_opcode_t 
{
    Unknown,
    Add,
    Addi,
    Sub,
    Mulu,
    Ld,
    St,
    Loop,
    LoopPip,
    Nop,
    Mov
};

enum class execution_unit_t 
{
    ALU,
    Mult,
    Mem,
    Branch,
    None
};

enum class register_kind_t 
{
    X,
    P,
    LC,
    EC,
    RRB,
    None
};

enum class basic_block_t 
{
    BB0,
    BB1,
    BB2,
    Unknown
};

struct register_ref_t 
{
    register_kind_t kind = register_kind_t::None;
    int index = -1;

    bool operator==(const register_ref_t& other) const {
        return kind == other.kind && index == other.index;
    }

    bool operator!=(const register_ref_t& other) const {
        return !(*this == other);
    }
};

const int MAX_REG_INDEX = 95;

inline bool is_x_register(const register_ref_t& reg) 
{
    return reg.kind == register_kind_t::X && reg.index >= 0 && reg.index <= 95;
}

inline bool is_p_register(const register_ref_t& reg) 
{
    return reg.kind == register_kind_t::P && reg.index >= 0 && reg.index <= 95;
}

inline bool is_special_register(const register_ref_t& reg) 
{
    return reg.kind == register_kind_t::LC ||
           reg.kind == register_kind_t::EC ||
           reg.kind == register_kind_t::RRB;
}










inline std::string register_to_string(const register_ref_t& reg)
{
    switch (reg.kind) {
        case register_kind_t::X:
            return "x" + std::to_string(reg.index);
        case register_kind_t::P:
            return "p" + std::to_string(reg.index);
        case register_kind_t::LC:
            return "LC";
        case register_kind_t::EC:
            return "EC";
        case register_kind_t::RRB:
            return "RRB";
        case register_kind_t::None:
            return "";
    }

    return "";
}

inline std::string basic_block_to_string(basic_block_t block) 
{
    switch (block) {
        case basic_block_t::BB0:
            return "BB0";
        case basic_block_t::BB1:
            return "BB1";
        case basic_block_t::BB2:
            return "BB2";
        case basic_block_t::Unknown:
            return "Unknown";
    }

    return "Unknown";
}



// Represents one assembly operand.
//
// Different instructions have different operand types:
//   addi x1, x2, 5      -> register, register, immediate
//   ld x5, 0(x2)        -> register, memory
//   mov p32, true       -> register, boolean
//
// However, std::vector<operand_t> requires every element to have the same C++ type.
// Therefore, operand_t uses the `kind` field as a tag to indicate which member is valid.

struct memory_operand_t 
{
    int64_t immediate = 0;
    register_ref_t base_register;
};

enum class operand_kind_t 
{
    None,
    Register,
    Immediate,
    Memory,
    Boolean
};

struct operand_t 
{
    operand_kind_t kind = operand_kind_t::None;

    register_ref_t reg;
    int64_t immediate = 0;
    memory_operand_t memory;
    bool boolean_value = false;

    std::string original_text;
};

struct instruction_t 
{

    int instruction_address = -1;
    basic_block_t block = basic_block_t::Unknown;
    instruction_opcode_t opcode = instruction_opcode_t::Unknown;
    execution_unit_t unit = execution_unit_t::None;
    int latency = 1;

    std::optional<register_ref_t> predicate;


    std::vector<operand_t> operands;
    std::optional<register_ref_t> dest;

    std::vector<register_ref_t> src_regs;

    int original_pc = -1;
    int scheduled_cycle = -1;
    int scheduled_stage = -1;
    std::string raw;
};
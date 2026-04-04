#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "state.hpp"
#include "view.hpp"
#include "cycle_context.hpp"
#include "instruction_memory.hpp"

class fetch_and_decode_stage
{
public:
    void propagate(const State& curr,
                   View& view,
                   State& next,
                   CycleContext& cycle_context,
                   const instruction_memory& imem) const;

private:
    static std::string trim(const std::string& s);
    static std::string to_lower(std::string s);
    static std::vector<std::string> split_operands(const std::string& s);
    static int parse_x_register(const std::string& token);
    static std::int64_t parse_immediate(const std::string& token);

    static instruction_decode_t decode_instruction(const std::string& raw_instruction,
                                                   std::uint64_t pc);
};








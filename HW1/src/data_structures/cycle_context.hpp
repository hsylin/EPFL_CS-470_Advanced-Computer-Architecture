#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

struct ForwardingResult
{
    unsigned int physical_register;
    uint64_t value;
    bool exception;
    unsigned int pc;
};

class CycleContext
{
public:
    CycleContext();

    // Clear all transient per-cycle signals/results
    void clear();

    // -------------------------
    // Forwarding-path results
    // -------------------------
    bool add_forwarding_result(unsigned int physical_register,
                               uint64_t value,
                               bool exception,
                               unsigned int pc);

                               
    std::size_t forwarding_count() const;
    const ForwardingResult& forwarding_at(std::size_t index) const;

    // -------------------------
    // Rename/Dispatch -> Fetch/Decode
    // -------------------------
    void set_rename_backpressure(bool value);
    bool is_rename_backpressure() const;

    // -------------------------
    // Commit -> exception/control requests
    // -------------------------
    void clear_forwarding_results();
    
    void request_execution_reset();
    bool is_execution_reset_requested() const;

    void request_halt_fetch_decode();
    bool is_halt_fetch_decode_requested() const;

    void request_halt_rename_dispatch();
    bool is_halt_rename_dispatch_requested() const;


    void request_integer_queue_reset();
    bool is_integer_queue_reset_requested() const;

    void request_suppress_head_exception_forwarding(unsigned int pc);
    bool should_suppress_forwarding(unsigned int pc) const;

private:
    static constexpr std::size_t MAX_FORWARDING_RESULTS = 4;

    std::array<ForwardingResult, MAX_FORWARDING_RESULTS> forwarding_results;
    std::size_t forwarding_size;

    bool rename_backpressure_flag;
    bool halt_fetch_decode_flag;
    bool halt_rename_dispatch_flag;
    bool execution_reset_flag;
    bool integer_queue_reset_flag;
    bool suppress_head_exception_forwarding_flag = false;
    unsigned int suppress_head_exception_forwarding_pc = 0;
};





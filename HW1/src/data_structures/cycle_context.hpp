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
    bool rename_backpressure() const;

    // -------------------------
    // Commit -> same-cycle exception/control requests
    // -------------------------
    void request_exception_entry(unsigned int pc);
    bool exception_entry_requested() const;
    unsigned int exception_entry_pc() const;

    void request_execution_reset();
    bool execution_reset_requested() const;

    void request_integer_queue_reset();
    bool integer_queue_reset_requested() const;

private:
    static constexpr std::size_t MAX_FORWARDING_RESULTS = 4;

    std::array<ForwardingResult, MAX_FORWARDING_RESULTS> forwarding_results;
    std::size_t forwarding_size;

    bool rename_backpressure_flag;

    bool exception_entry_requested_flag;
    unsigned int exception_entry_pc_value;

    bool execution_reset_flag;
    bool integer_queue_reset_flag;
};
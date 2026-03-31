#include "cycle_context.hpp"

CycleContext::CycleContext()
{
    clear();
}

void CycleContext::clear()
{
    forwarding_size = 0;

    rename_backpressure_flag = false;

    exception_entry_requested_flag = false;
    exception_entry_pc_value = 0;

    execution_reset_flag = false;
    integer_queue_reset_flag = false;
}

bool CycleContext::add_forwarding_result(unsigned int physical_register,
                                         uint64_t value,
                                         bool exception,
                                         unsigned int pc)
{
    if (forwarding_size >= MAX_FORWARDING_RESULTS)
    {
        return false;
    }

    forwarding_results[forwarding_size] = {
        physical_register,
        value,
        exception,
        pc
    };
    forwarding_size++;
    return true;
}

std::size_t CycleContext::forwarding_count() const
{
    return forwarding_size;
}

const ForwardingResult& CycleContext::forwarding_at(std::size_t index) const
{
    return forwarding_results[index];
}

void CycleContext::set_rename_backpressure(bool value)
{
    rename_backpressure_flag = value;
}

bool CycleContext::rename_backpressure() const
{
    return rename_backpressure_flag;
}

void CycleContext::request_exception_entry(unsigned int pc)
{
    exception_entry_requested_flag = true;
    exception_entry_pc_value = pc;
}

bool CycleContext::exception_entry_requested() const
{
    return exception_entry_requested_flag;
}

unsigned int CycleContext::exception_entry_pc() const
{
    return exception_entry_pc_value;
}

void CycleContext::request_execution_reset()
{
    execution_reset_flag = true;
}

bool CycleContext::execution_reset_requested() const
{
    return execution_reset_flag;
}

void CycleContext::request_integer_queue_reset()
{
    integer_queue_reset_flag = true;
}

bool CycleContext::integer_queue_reset_requested() const
{
    return integer_queue_reset_flag;
}
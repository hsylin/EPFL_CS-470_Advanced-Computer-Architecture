#include "cycle_context.hpp"

CycleContext::CycleContext()
{
    clear();
}

void CycleContext::clear()
{
    forwarding_size = 0;
    rename_backpressure_flag = false;
    execution_reset_flag = false;
    integer_queue_reset_flag = false;
    halt_fetch_decode_flag = false;
    halt_rename_dispatch_flag = false;

    suppress_head_exception_forwarding_flag = false;
    suppress_head_exception_forwarding_pc = 0;
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


void CycleContext::clear_forwarding_results()
{
    forwarding_size = 0;
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

bool CycleContext::is_rename_backpressure() const
{
    return rename_backpressure_flag;
}


void CycleContext::request_execution_reset()
{
    execution_reset_flag = true;
}
void CycleContext::request_halt_fetch_decode()
{
    halt_fetch_decode_flag = true;
}


bool CycleContext::is_halt_fetch_decode_requested() const
{
    return halt_fetch_decode_flag;
}


void CycleContext::request_halt_rename_dispatch()
{
    halt_rename_dispatch_flag = true;
}

bool CycleContext::is_halt_rename_dispatch_requested() const
{
    return halt_rename_dispatch_flag;
}


bool CycleContext::is_execution_reset_requested() const
{
    return execution_reset_flag;
}

void CycleContext::request_integer_queue_reset()
{
    integer_queue_reset_flag = true;
}

bool CycleContext::is_integer_queue_reset_requested() const
{
    return integer_queue_reset_flag;
}


void CycleContext::request_suppress_head_exception_forwarding(unsigned int pc)
{
    suppress_head_exception_forwarding_flag = true;
    suppress_head_exception_forwarding_pc = pc;
}

bool CycleContext::should_suppress_forwarding(unsigned int pc) const
{
    return suppress_head_exception_forwarding_flag &&
           suppress_head_exception_forwarding_pc == pc;
}
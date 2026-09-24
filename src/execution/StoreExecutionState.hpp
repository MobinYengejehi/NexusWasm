#pragma once

#ifndef NEXUSWASM_STORE_EXECUTION_STATE_HEADER
#define NEXUSWASM_STORE_EXECUTION_STATE_HEADER

#include <string>
#include <string_view>
#include <utility>

#include <nexuswasm/Error.hpp>
#include <nexuswasm/Result.hpp>

namespace nexus::detail
{
    enum class StoreExecutionOperation
    {
        SynchronousExecution,
        AsynchronousExecution,
        ExclusiveMutation
    };

    class StoreExecutionState;

    class StoreExecutionLease final
    {
    public:
        StoreExecutionLease() noexcept = default;

        StoreExecutionLease(const StoreExecutionLease&) = delete;
        StoreExecutionLease& operator=(const StoreExecutionLease&) = delete;

        StoreExecutionLease(StoreExecutionLease&& other) noexcept:
            m_pState{ std::exchange(other.m_pState, nullptr) }
        {}

        StoreExecutionLease& operator=(StoreExecutionLease&& other) noexcept
        {
            if (this == &other)
            {
                return *this;
            }

            Release();

            m_pState = std::exchange(other.m_pState, nullptr);

            return *this;
        }

        ~StoreExecutionLease()
        {
            Release();
        }

        void Release() noexcept;

        [[nodiscard]]
        explicit operator bool() const noexcept
        {
            return m_pState != nullptr;
        }

    private:
        StoreExecutionState* m_pState = nullptr;

        explicit StoreExecutionLease(StoreExecutionState* state) noexcept:
            m_pState{ state }
        {}

        friend class StoreExecutionState;
    };

    class StoreExecutionState final
    {
    public:
        /*
         * Threading contract:
         *
         * This object does NOT provide synchronization between OS threads.
         *
         * An ExecutionDomain / Wasmtime Store has exactly one logical owner
         * at a time. Sequential hand-off between threads is permitted when
         * the host provides the required synchronization.
         *
         * Concurrent access to the same ExecutionDomain is unsupported.
         *
         * This state machine prevents overlapping Store operations under
         * that ownership contract; for example, starting another Call()
         * while a native Wasmtime async future is still alive.
         */

        [[nodiscard]]
        Result<StoreExecutionLease> Acquire(
            const StoreExecutionOperation operation,
            const std::string_view operationName
        )
        {
            if (m_eActivity != Activity::Idle)
            {
                return Error{
                    ErrorCode::StoreBusy,
                    "Cannot perform '" +
                    std::string{ operationName } +
                    "' because another operation currently "
                    "owns the Wasmtime Store."
                };
            }

            if (operation == StoreExecutionOperation::SynchronousExecution && m_bAsyncRequired)
            {
                return Error{
                    ErrorCode::StoreRequiresAsync,
                    "Cannot perform synchronous operation '" +
                    std::string{ operationName } +
                    "' because this Wasmtime Store "
                    "requires asynchronous entrypoints."
                };
            }

            if (operation == StoreExecutionOperation::AsynchronousExecution)
            {
                m_eActivity = Activity::AsyncExecutionActive;
            }
            else
            {
                m_eActivity = Activity::ExclusiveOperationActive;
            }

            return StoreExecutionLease{ this };
        }

        void MarkAsyncRequired() noexcept
        {
            m_bAsyncRequired = true;
        }

        [[nodiscard]]
        bool RequiresAsync() const noexcept
        {
            return m_bAsyncRequired;
        }

        [[nodiscard]]
        bool IsIdle() const noexcept
        {
            return m_eActivity == Activity::Idle;
        }

    private:
        enum class Activity
        {
            Idle,

            /*
             * A synchronous execution or Store/Linker mutation is
             * currently in progress.
             */
            ExclusiveOperationActive,

            /*
             * A native wasmtime_call_future_t currently owns the
             * Store. No other Store operation may begin until that
             * future is deleted.
             */
            AsyncExecutionActive
        };

        Activity m_eActivity = Activity::Idle;

        bool m_bAsyncRequired = false;

        void Release() noexcept
        {
            m_eActivity = Activity::Idle;
        }

        friend class StoreExecutionLease;
    };

    inline void StoreExecutionLease::Release() noexcept
    {
        if (m_pState == nullptr)
        {
            return;
        }

        StoreExecutionState* state = std::exchange(m_pState, nullptr);

        state->Release();
    }
}

#endif

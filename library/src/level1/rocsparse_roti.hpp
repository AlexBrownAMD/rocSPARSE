/* ************************************************************************
 * Copyright 2018 Advanced Micro Devices, Inc.
 * ************************************************************************ */

#pragma once
#ifndef ROCSPARSE_ROTI_HPP
#define ROCSPARSE_ROTI_HPP

#include "rocsparse.h"
#include "handle.h"
#include "utility.h"
#include "roti_device.h"

#include <hip/hip_runtime.h>

template <typename T>
__global__ void roti_kernel_host_scalar(rocsparse_int nnz,
                                        T* x_val,
                                        const rocsparse_int* x_ind,
                                        T* y,
                                        T c,
                                        T s,
                                        rocsparse_index_base idx_base)
{
    roti_device(nnz, x_val, x_ind, y, c, s, idx_base);
}

template <typename T>
__global__ void roti_kernel_device_scalar(rocsparse_int nnz,
                                         T* x_val,
                                         const rocsparse_int* x_ind,
                                         T* y,
                                         const T* c,
                                         const T* s,
                                         rocsparse_index_base idx_base)
{
    if(*c == static_cast<T>(1) && *s == static_cast<T>(0))
    {
        return;
    }

    roti_device(nnz, x_val, x_ind, y, *c, *s, idx_base);
}

template <typename T>
rocsparse_status rocsparse_roti_template(rocsparse_handle handle,
                                         rocsparse_int nnz,
                                         T* x_val,
                                         const rocsparse_int* x_ind,
                                         T* y,
                                         const T* c,
                                         const T* s,
                                         rocsparse_index_base idx_base)
{
    // Check for valid handle
    if(handle == nullptr)
    {
        return rocsparse_status_invalid_handle;
    }

    // Logging // TODO bench logging
    if(handle->pointer_mode == rocsparse_pointer_mode_host)
    {
        log_trace(handle,
                  replaceX<T>("rocsparse_Xroti"),
                  nnz,
                  (const void*&)x_val,
                  (const void*&)x_ind,
                  (const void*&)y,
                  *c,
                  *s,
                  idx_base);
    }
    else
    {
        log_trace(handle,
                  replaceX<T>("rocsparse_Xroti"),
                  nnz,
                  (const void*&)x_val,
                  (const void*&)x_ind,
                  (const void*&)y,
                  (const void*&)c,
                  (const void*&)s,
                  idx_base);
    }

    // Check index base
    if(idx_base != rocsparse_index_base_zero && idx_base != rocsparse_index_base_one)
    {
        return rocsparse_status_invalid_value;
    }

    // Check size
    if(nnz < 0)
    {
        return rocsparse_status_invalid_size;
    }

    // Check pointer arguments
    if(c == nullptr)
    {
        return rocsparse_status_invalid_pointer;
    }
    else if(s == nullptr)
    {
        return rocsparse_status_invalid_pointer;
    }
    else if(x_val == nullptr)
    {
        return rocsparse_status_invalid_pointer;
    }
    else if(x_ind == nullptr)
    {
        return rocsparse_status_invalid_pointer;
    }
    else if(y == nullptr)
    {
        return rocsparse_status_invalid_pointer;
    }

    // Quick return if possible
    if(nnz == 0)
    {
        return rocsparse_status_success;
    }

    // Stream
    hipStream_t stream = handle->stream;

#define ROTI_DIM 512
    dim3 roti_blocks((nnz - 1) / ROTI_DIM + 1);
    dim3 roti_threads(ROTI_DIM);

    if(handle->pointer_mode == rocsparse_pointer_mode_device)
    {
        hipLaunchKernelGGL((roti_kernel_device_scalar<T>),
                           roti_blocks,
                           roti_threads,
                           0,
                           stream,
                           nnz,
                           x_val,
                           x_ind,
                           y,
                           c,
                           s,
                           idx_base);
    }
    else
    {
        if(*c == static_cast<T>(1) && *s == static_cast<T>(0))
        {
            return rocsparse_status_success;
        }

        hipLaunchKernelGGL((roti_kernel_host_scalar<T>),
                           roti_blocks,
                           roti_threads,
                           0,
                           stream,
                           nnz,
                           x_val,
                           x_ind,
                           y,
                           *c,
                           *s,
                           idx_base);
    }
#undef ROTI_DIM
    return rocsparse_status_success;
}

#endif // ROCSPARSE_ROTI_HPP

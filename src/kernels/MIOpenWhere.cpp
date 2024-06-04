/*******************************************************************************
 *
 * MIT License
 *
 * Copyright (c) 2024 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 *******************************************************************************/
#ifndef MIOPEN_DONT_USE_HIP_RUNTIME_HEADERS
#include <hip/hip_fp16.h>
#include <hip/hip_runtime.h>
#endif

#include "float_types.h"

template <typename TI, typename TO>
__device__ void WhereBroadcastedContiguousForward_Kernel(const char* condition,
                                                         const TI* input,
                                                         const TI* other,
                                                         TO* output,
                                                         size_t size,
                                                         size_t condition_off,
                                                         size_t input_off,
                                                         size_t other_off,
                                                         size_t output_off,
                                                         size_t condition_size,
                                                         size_t input_size,
                                                         size_t other_size)
{
    size_t gid = blockIdx.x * blockDim.x + threadIdx.x;

    if(gid >= size)
        return;

    if(condition[gid % condition_size + condition_off])
    {
        output[gid + output_off] = input[gid % input_size + input_off];
    }
    else
    {
        output[gid + output_off] = other[gid % other_size + other_off];
    }
}

extern "C" __global__ void WhereBroadcastedContiguousForward(const char* condition,
                                                             const INPUT_TYPE* input,
                                                             const INPUT_TYPE* other,
                                                             OUTPUT_TYPE* output,
                                                             size_t size,
                                                             size_t condition_off,
                                                             size_t input_off,
                                                             size_t other_off,
                                                             size_t output_off,
                                                             size_t condition_size,
                                                             size_t input_size,
                                                             size_t other_size)
{
    WhereBroadcastedContiguousForward_Kernel<INPUT_TYPE, OUTPUT_TYPE>(condition,
                                                                      input,
                                                                      other,
                                                                      output,
                                                                      size,
                                                                      condition_off,
                                                                      input_off,
                                                                      other_off,
                                                                      output_off,
                                                                      condition_size,
                                                                      input_size,
                                                                      other_size);
}

template <typename TI, typename TO>
__device__ void WhereConditionBroadcastedContiguousForward_Kernel(const char* condition,
                                                                  const TI* input,
                                                                  const TI* other,
                                                                  TO* output,
                                                                  size_t size,
                                                                  size_t condition_off,
                                                                  size_t input_off,
                                                                  size_t other_off,
                                                                  size_t output_off,
                                                                  size_t condition_size,
                                                                  size_t input_size,
                                                                  size_t other_size)
{
    size_t gid = blockIdx.x * blockDim.x + threadIdx.x;
    if(gid >= condition_size)
        return;

    char cond = condition[gid % condition_size + condition_off];

    if(cond)
    {
        for(int idx = gid; idx < size; idx += condition_size)
        {
            output[idx + output_off] = input[idx % input_size + input_off];
        }
    }
    else
    {
        for(int idx = gid; idx < size; idx += condition_size)
        {
            output[idx + output_off] = other[idx % other_size + other_off];
        }
    }
}

extern "C" __global__ void WhereConditionBroadcastedContiguousForward(const char* condition,
                                                                      const INPUT_TYPE* input,
                                                                      const INPUT_TYPE* other,
                                                                      OUTPUT_TYPE* output,
                                                                      size_t size,
                                                                      size_t condition_off,
                                                                      size_t input_off,
                                                                      size_t other_off,
                                                                      size_t output_off,
                                                                      size_t condition_size,
                                                                      size_t input_size,
                                                                      size_t other_size)
{
    WhereConditionBroadcastedContiguousForward_Kernel<INPUT_TYPE, OUTPUT_TYPE>(condition,
                                                                               input,
                                                                               other,
                                                                               output,
                                                                               size,
                                                                               condition_off,
                                                                               input_off,
                                                                               other_off,
                                                                               output_off,
                                                                               condition_size,
                                                                               input_size,
                                                                               other_size);
}

template <typename TI, typename TO>
__device__ void WhereBroadcastedContiguousBackward_Kernel(const char* condition,
                                                          const TI* output_grad,
                                                          TO* input_grad,
                                                          TO* other_grad,
                                                          size_t size,
                                                          size_t output_grad_off,
                                                          size_t condition_off,
                                                          size_t input_grad_off,
                                                          size_t other_grad_off,
                                                          size_t condition_size,
                                                          size_t input_size,
                                                          size_t other_size)
{
    size_t gid = blockIdx.x * blockDim.x + threadIdx.x;

    if(gid >= size)
        return;

    if(input_grad)
    {
        input_grad[gid % input_size + input_grad_off] =
            output_grad[gid + output_grad_off] * condition[gid % condition_size + condition_off];
    }
    if(other_grad)
    {
        other_grad[gid % other_size + other_grad_off] =
            output_grad[gid + output_grad_off] *
            (1 - condition[gid % condition_size + condition_off]);
    }
}

extern "C" __global__ void WhereBroadcastedContiguousBackward(const char* condition,
                                                              const OUTPUT_TYPE* output_grad,
                                                              INPUT_TYPE* input_grad,
                                                              INPUT_TYPE* other_grad,
                                                              size_t size,
                                                              size_t output_grad_off,
                                                              size_t condition_off,
                                                              size_t input_grad_off,
                                                              size_t other_grad_off,
                                                              size_t condition_size,
                                                              size_t input_size,
                                                              size_t other_size)
{
    WhereBroadcastedContiguousBackward_Kernel<OUTPUT_TYPE, INPUT_TYPE>(condition,
                                                                       output_grad,
                                                                       input_grad,
                                                                       other_grad,
                                                                       size,
                                                                       output_grad_off,
                                                                       condition_off,
                                                                       input_grad_off,
                                                                       other_grad_off,
                                                                       condition_size,
                                                                       input_size,
                                                                       other_size);
}

template <typename TI, typename TO>
__device__ void WhereConditionBroadcastedContiguousBackward_Kernel(const char* condition,
                                                                   const TI* output_grad,
                                                                   TO* input_grad,
                                                                   TO* other_grad,
                                                                   size_t size,
                                                                   size_t output_grad_off,
                                                                   size_t condition_off,
                                                                   size_t input_grad_off,
                                                                   size_t other_grad_off,
                                                                   size_t condition_size,
                                                                   size_t input_size,
                                                                   size_t other_size)
{
    size_t gid = blockIdx.x * blockDim.x + threadIdx.x;
    if(gid >= condition_size)
        return;

    char cond = condition[gid % condition_size + condition_off];

    if(input_grad)
    {
        for(int idx = gid; idx < input_size; idx += condition_size)
        {
            input_grad[idx + input_grad_off] = output_grad[idx % size + output_grad_off] * cond;
        }
    }
    if(other_grad)
    {
        for(int idx = gid; idx < other_size; idx += condition_size)
        {
            other_grad[idx + other_grad_off] =
                output_grad[idx % size + output_grad_off] * (1 - cond);
        }
    }
}

extern "C" __global__ void
WhereConditionBroadcastedContiguousBackward(const char* condition,
                                            const OUTPUT_TYPE* output_grad,
                                            INPUT_TYPE* input_grad,
                                            INPUT_TYPE* other_grad,
                                            size_t size,
                                            size_t output_grad_off,
                                            size_t condition_off,
                                            size_t input_grad_off,
                                            size_t other_grad_off,
                                            size_t condition_size,
                                            size_t input_size,
                                            size_t other_size)
{
    WhereConditionBroadcastedContiguousBackward_Kernel<OUTPUT_TYPE, INPUT_TYPE>(condition,
                                                                                output_grad,
                                                                                input_grad,
                                                                                other_grad,
                                                                                size,
                                                                                output_grad_off,
                                                                                condition_off,
                                                                                input_grad_off,
                                                                                other_grad_off,
                                                                                condition_size,
                                                                                input_size,
                                                                                other_size);
}

template <typename TI, typename TO>
__device__ void WhereContiguousBackward_Kernel(
    const char* condition, const TI* output_grad, TO* input_grad, TO* other_grad, size_t size)
{
    size_t gid = blockIdx.x * blockDim.x + threadIdx.x;

    if(gid >= size)
        return;

    if(input_grad)
    {
        input_grad[gid] = output_grad[gid] * condition[gid];
    }
    if(other_grad)
    {
        other_grad[gid] = output_grad[gid] * (1 - condition[gid]);
    }
}

extern "C" __global__ void WhereContiguousBackward(const char* condition,
                                                   const OUTPUT_TYPE* output_grad,
                                                   INPUT_TYPE* input_grad,
                                                   INPUT_TYPE* other_grad,
                                                   size_t size)
{
    WhereContiguousBackward_Kernel<OUTPUT_TYPE, INPUT_TYPE>(
        condition, output_grad, input_grad, other_grad, size);
}

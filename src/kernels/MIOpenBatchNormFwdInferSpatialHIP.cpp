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

#if MIOPEN_USE_FP16 == 1
#define FP_TYPE half
#define FP_PREC float
#endif

#if MIOPEN_USE_FP32 == 1
#define FP_TYPE float
#define FP_PREC float
#endif

// if MIOPEN_USE_FP16 is not defined, default is fp32
#ifndef FP_TYPE
#define FP_TYPE float
#define FP_PREC float
#endif

template <typename TIO, typename TPREC>
__forceinline__ __device__ void bn_fwd_infer_spatial(const TIO* __restrict in, /* x input */
                                                     TIO* __restrict out,      /* y output */
                                                     const TPREC* __restrict estimatedMean,
                                                     const TPREC* __restrict estimatedVariance,
                                                     const TPREC* __restrict scale,
                                                     const TPREC* __restrict bias,
                                                     double epsilon,
                                                     unsigned int batchSize,
                                                     unsigned int imageDims,
                                                     unsigned int batchStride)
{

    unsigned int xidx = blockIdx.x * blockDim.x + threadIdx.x;
    unsigned int yidx = blockIdx.y * blockDim.y + threadIdx.y;

    const TPREC mean        = estimatedMean[xidx];
    const TPREC variance    = estimatedVariance[xidx];
    const TPREC pscale      = scale[xidx];
    const TPREC pbias       = bias[xidx];
    const TPREC invVariance = static_cast<TPREC>(rsqrt(fabs(variance + epsilon)));

    for(unsigned int idx = yidx; idx < imageDims; idx += gridDim.y * blockDim.y)
    {
        for(unsigned int n = 0; n < batchSize; n++)
        {
            unsigned int index = (n * batchStride) + (xidx * imageDims) + idx;
            TPREC inhat        = (static_cast<TPREC>(in[index]) - mean) * invVariance;
            out[index]         = static_cast<TIO>(fma(pscale, inhat, pbias));
        }
    }
}

extern "C" __global__ void __launch_bounds__(MIO_BN_GRP0* MIO_BN_GRP1* MIO_BN_GRP2)
    MIOpenBatchNormFwdInferSpatialEstHIP(const FP_TYPE* __restrict in, /* x input */
                                         FP_TYPE* __restrict out,      /* y output */
                                         const FP_PREC* __restrict estimatedMean,
                                         const FP_PREC* __restrict estimatedVariance,
                                         const FP_PREC* __restrict scale,
                                         const FP_PREC* __restrict bias,
                                         double epsilon,
                                         unsigned int batchSize,
                                         unsigned int imageDims,
                                         unsigned int batchStride)
{
    bn_fwd_infer_spatial<FP_TYPE, FP_PREC>(in,
                                           out,
                                           estimatedMean,
                                           estimatedVariance,
                                           scale,
                                           bias,
                                           epsilon,
                                           batchSize,
                                           imageDims,
                                           batchStride);
}
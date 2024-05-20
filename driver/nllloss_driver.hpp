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
#ifndef GUARD_MIOPEN_NLLLOSS_DRIVER_HPP
#define GUARD_MIOPEN_NLLLOSS_DRIVER_HPP

#include "InputFlags.hpp"
#include "driver.hpp"
#include "mloNLLLossHost.hpp"
#include "random.hpp"
#include "tensor_driver.hpp"
#include "timer.hpp"
#include "util_driver.hpp"

#include <../test/tensor_holder.hpp>
#include <../test/verify.hpp>

#include <miopen/env.hpp>
#include <miopen/handle.hpp>
#include <miopen/miopen.h>
#include <miopen/tensor.hpp>
#include <vector>

inline std::vector<int> GetStrides(std::vector<int> lengths, int contiguous)
{
    if(contiguous != 0 && contiguous != 1)
        std::cerr << "Error Tensor Contiguous should be 0 or 1" << std::endl;
    if(contiguous == 0)
        std::swap(lengths.front(), lengths.back());
    std::vector<int> strides(lengths.size());
    strides.back() = 1;
    for(int i = lengths.size() - 2; i >= 0; --i)
        strides[i] = strides[i + 1] * lengths[i + 1];
    if(contiguous == 0)
        std::swap(strides.front(), strides.back());
    return strides;
}

template <typename Tgpu, typename Tref>
class NLLLossDriver : public Driver
{
public:
    NLLLossDriver() : Driver()
    {
        miopenCreateTensorDescriptor(&inputDesc);
        miopenCreateTensorDescriptor(&targetDesc);
        miopenCreateTensorDescriptor(&weightDesc);
        miopenCreateTensorDescriptor(&outputDesc);
        miopenCreateTensorDescriptor(&inputGradDesc);
        miopenCreateTensorDescriptor(&outputGradDesc);

        data_type = miopen_type<Tgpu>{};
    }

    int AddCmdLineArgs() override;
    int ParseCmdLineArgs(int argc, char* argv[]) override;
    InputFlags& GetInputFlags() override { return inflags; }

    std::vector<int> GetInputTensorDimsFromCmd();
    int GetandSetData() override;

    int AllocateBuffersAndCopy() override;

    int RunForwardGPU() override;
    int RunForwardCPU();

    int RunBackwardGPU() override;
    int RunBackwardCPU();

    Tref GetTolerance();
    int VerifyBackward() override;
    int VerifyForward() override;
    ~NLLLossDriver() override
    {
        miopenDestroyTensorDescriptor(inputDesc);
        miopenDestroyTensorDescriptor(targetDesc);
        miopenDestroyTensorDescriptor(weightDesc);
        miopenDestroyTensorDescriptor(outputDesc);
        miopenDestroyTensorDescriptor(inputGradDesc);
        miopenDestroyTensorDescriptor(outputGradDesc);
    }

private:
    InputFlags inflags;

    int forw;

    miopenTensorDescriptor_t inputDesc;
    miopenTensorDescriptor_t targetDesc;
    miopenTensorDescriptor_t weightDesc;
    miopenTensorDescriptor_t outputDesc;
    miopenTensorDescriptor_t inputGradDesc;
    miopenTensorDescriptor_t outputGradDesc;

    std::unique_ptr<GPUMem> in_dev;
    std::unique_ptr<GPUMem> target_dev;
    std::unique_ptr<GPUMem> weight_dev;
    std::unique_ptr<GPUMem> out_dev;
    std::unique_ptr<GPUMem> workspace_dev;
    std::unique_ptr<GPUMem> in_grad_dev;
    std::unique_ptr<GPUMem> out_grad_dev;

    std::vector<Tgpu> in;
    std::vector<int> target;
    std::vector<Tgpu> weight;
    std::vector<Tgpu> out;
    std::vector<Tref> out_host;
    std::vector<Tgpu> workspace;
    std::vector<Tref> workspace_host;

    std::vector<Tgpu> in_grad;
    std::vector<Tref> in_grad_host;
    std::vector<Tgpu> out_grad;

    size_t ws_sizeInBytes;

    std::vector<int> input_sizes;
    int32_t ignore_index;
    float divisor;
};

template <typename Tgpu, typename Tref>
int NLLLossDriver<Tgpu, Tref>::ParseCmdLineArgs(int argc, char* argv[])
{
    inflags.Parse(argc, argv);

    if(inflags.GetValueInt("time") == 1)
    {
        miopenEnableProfiling(GetHandle(), true);
    }
    return miopenStatusSuccess;
}

template <typename Tgpu, typename Tref>
std::vector<int> NLLLossDriver<Tgpu, Tref>::GetInputTensorDimsFromCmd()
{
    std::string lengthsStr = inflags.GetValueStr("input_dims");

    std::vector<int> lengths;
    std::size_t pos = 0;
    std::size_t new_pos;

    new_pos = lengthsStr.find(',', pos);
    while(new_pos != std::string::npos)
    {
        std::string sliceStr = lengthsStr.substr(pos, new_pos - pos);

        int len = std::stoi(sliceStr);

        lengths.push_back(len);

        pos     = new_pos + 1;
        new_pos = lengthsStr.find(',', pos);
    };

    std::string sliceStr = lengthsStr.substr(pos);
    int len              = std::stoi(sliceStr);

    lengths.push_back(len);

    return (lengths);
}

template <typename Tgpu, typename Tref>
int NLLLossDriver<Tgpu, Tref>::GetandSetData()
{
    auto reduction = inflags.GetValueStr("reduce");
    if(reduction != "none" && reduction != "mean" && reduction != "sum")
        return miopenStatusInvalidValue;

    input_sizes  = GetInputTensorDimsFromCmd();
    ignore_index = static_cast<int32_t>(inflags.GetValueInt("ignore_index"));

    std::vector<int> in_len     = input_sizes;
    std::vector<int> target_len = in_len;
    target_len.erase(std::next(target_len.begin()));
    std::vector<int> weight_len = {in_len[1]};
    std::vector<int> out_len    = target_len;

    auto in_strides     = GetStrides(in_len, 1);
    auto tar_strides    = GetStrides(target_len, inflags.GetValueInt("contiguous"));
    auto weight_strides = GetStrides(weight_len, 1);
    auto output_strides = GetStrides(out_len, 1);

    SetTensorNd(inputDesc, in_len, in_strides, data_type);
    SetTensorNd(targetDesc, target_len, tar_strides, data_type);
    SetTensorNd(weightDesc, weight_len, weight_strides, data_type);

    if(reduction == "none")
    {
        divisor = std::numeric_limits<float>::quiet_NaN();
        SetTensorNd(outputDesc, out_len, output_strides, data_type);
        SetTensorNd(outputGradDesc, out_len, output_strides, data_type);
    }
    else
    {
        std::vector<int> out_len_rd = {1};
        SetTensorNd(outputDesc, out_len_rd, data_type);
        SetTensorNd(outputGradDesc, out_len_rd, output_strides, data_type);
        if(reduction == "sum")
            divisor = 1;
        if(reduction == "mean")
            divisor = miopen::deref(targetDesc).GetElementSize();
    }

    SetTensorNd(inputGradDesc, in_len, in_strides, data_type);

    return miopenStatusSuccess;
}

template <typename Tgpu, typename Tref>
int NLLLossDriver<Tgpu, Tref>::AddCmdLineArgs()
{
    inflags.AddInputFlag("forw", 'F', "1", "Run only Forward NLLLoss (Default=1)", "int");
    inflags.AddInputFlag("input_dims",
                         'D',
                         "16,21,21,21,10",
                         "The dimensional lengths of the input tensor",
                         "string");
    inflags.AddInputFlag("ignore_index", 'g', "-1", "Ignore index", "int");
    inflags.AddInputFlag("reduce",
                         'R',
                         "none",
                         "Specifies the reduction to apply to the output ('none'|'mean'|'sum') "
                         "(Default=none to indicate no reduction)",
                         "string");
    inflags.AddInputFlag("contiguous",
                         'c',
                         "1",
                         "Is input tensor contiguous? (Default=1 for contiguous tensor)",
                         "int");

    inflags.AddInputFlag("iter", 'i', "10", "Number of Iterations (Default=10)", "int");
    inflags.AddInputFlag("verify", 'V', "1", "Verify (Default=1)", "int");
    inflags.AddInputFlag("time", 't', "1", "Time (Default=0)", "int");
    inflags.AddInputFlag(
        "wall", 'w', "0", "Wall-clock Time, Requires time == 1 (Default=0)", "int");

    return miopenStatusSuccess;
}

template <typename Tgpu, typename Tref>
int NLLLossDriver<Tgpu, Tref>::AllocateBuffersAndCopy()
{
    size_t in_sz     = GetTensorSize(inputDesc);
    size_t target_sz = GetTensorSize(targetDesc);
    size_t weight_sz = GetTensorSize(weightDesc);
    size_t out_sz    = GetTensorSize(outputDesc);

    if(!std::isnan(divisor))
    {
        miopenGetNLLLossReduceForwardWorkspaceSize(GetHandle(),
                                                   inputDesc,
                                                   targetDesc,
                                                   weightDesc,
                                                   outputDesc,
                                                   ignore_index,
                                                   divisor,
                                                   &ws_sizeInBytes);
        if(ws_sizeInBytes == static_cast<size_t>(-1))
            return miopenStatusAllocFailed;
    }
    else
        ws_sizeInBytes = 0;
    size_t ws_sz = ws_sizeInBytes / sizeof(Tgpu);

    uint32_t ctx = 0;

    in_dev        = std::unique_ptr<GPUMem>(new GPUMem(ctx, in_sz, sizeof(Tgpu)));
    target_dev    = std::unique_ptr<GPUMem>(new GPUMem(ctx, target_sz, sizeof(int)));
    weight_dev    = std::unique_ptr<GPUMem>(new GPUMem(ctx, weight_sz, sizeof(Tgpu)));
    out_dev       = std::unique_ptr<GPUMem>(new GPUMem(ctx, out_sz, sizeof(Tgpu)));
    workspace_dev = std::unique_ptr<GPUMem>(new GPUMem(ctx, ws_sizeInBytes, sizeof(std::byte)));
    in_grad_dev   = std::unique_ptr<GPUMem>(new GPUMem(ctx, in_sz, sizeof(Tgpu)));
    out_grad_dev  = std::unique_ptr<GPUMem>(new GPUMem(ctx, out_sz, sizeof(Tgpu)));

    in             = std::vector<Tgpu>(in_sz, static_cast<Tgpu>(0));
    target         = std::vector<int>(target_sz, static_cast<int>(0));
    weight         = std::vector<Tgpu>(weight_sz, static_cast<Tgpu>(1));
    out            = std::vector<Tgpu>(out_sz, static_cast<Tgpu>(0));
    out_host       = std::vector<Tref>(out_sz, static_cast<Tref>(0));
    workspace      = std::vector<Tgpu>(ws_sz, static_cast<Tgpu>(0));
    workspace_host = std::vector<Tref>(ws_sz, static_cast<Tref>(0));

    in_grad      = std::vector<Tgpu>(in_sz, static_cast<Tgpu>(0));
    in_grad_host = std::vector<Tref>(in_sz, static_cast<Tref>(0));
    out_grad     = std::vector<Tgpu>(out_sz, static_cast<Tgpu>(0));

    int status;

    for(int i = 0; i < in_sz; i++)
    {
        in[i] = prng::gen_A_to_B<Tgpu>(static_cast<Tgpu>(-1.0), static_cast<Tgpu>(-(1e-2)));
    }
    status = in_dev->ToGPU(q, in.data());

    for(int i = 0; i < target_sz; i++)
    {
        target[i] = prng::gen_A_to_B<int>(static_cast<int>(0), static_cast<int>(weight_sz - 1));
    }
    status |= target_dev->ToGPU(q, target.data());

    for(int i = 0; i < weight_sz; i++)
    {
        weight[i] = prng::gen_A_to_B<Tgpu>(static_cast<Tgpu>(-1.0), static_cast<Tgpu>(1.0));
    }
    status |= weight_dev->ToGPU(q, weight.data());

    status |= out_dev->ToGPU(q, out.data());

    status |= in_grad_dev->ToGPU(q, in_grad.data());

    for(int i = 0; i < out_sz; i++)
    {
        out_grad[i] = prng::gen_A_to_B<Tgpu>(static_cast<Tgpu>(-1.0), static_cast<Tgpu>(1.0));
    }
    status |= out_grad_dev->ToGPU(q, out_grad.data());

    if(status != 0)
        std::cout << "Error copying data to GPU\n" << std::endl;

    return miopenStatusSuccess;
}

template <typename Tgpu, typename Tref>
int NLLLossDriver<Tgpu, Tref>::RunForwardGPU()
{
    float kernel_total_time = 0.0;
    float kernel_first_time = 0.0;

    Timer t;
    START_TIME

    for(int i = 0; i < inflags.GetValueInt("iter"); i++)
    {
        if(!std::isnan(divisor))
        {
            miopenNLLLossReduceForward(GetHandle(),
                                       workspace_dev->GetMem(),
                                       ws_sizeInBytes,
                                       inputDesc,
                                       in_dev->GetMem(),
                                       targetDesc,
                                       target_dev->GetMem(),
                                       weightDesc,
                                       weight_dev->GetMem(),
                                       outputDesc,
                                       out_dev->GetMem(),
                                       ignore_index,
                                       divisor);
        }
        else
        {
            miopenNLLLossUnreduceForward(GetHandle(),
                                         inputDesc,
                                         in_dev->GetMem(),
                                         targetDesc,
                                         target_dev->GetMem(),
                                         weightDesc,
                                         weight_dev->GetMem(),
                                         outputDesc,
                                         out_dev->GetMem(),
                                         ignore_index);
        }
        float time = 0.0;
        miopenGetKernelTime(GetHandle(), &time);
        kernel_total_time += time;
        if(i == 0)
            kernel_first_time = time;
    }

    if(inflags.GetValueInt("time") == 1)
    {
        STOP_TIME
        int iter = inflags.GetValueInt("iter");
        if(WALL_CLOCK)
            printf("Wall-clock Time Forward NLLLoss Elapsed: %f ms\n", t.gettime_ms() / iter);

        float kernel_average_time =
            iter > 1 ? (kernel_total_time - kernel_first_time) / (iter - 1) : kernel_first_time;
        printf("GPU Kernel Time Forward NLLLoss Elapsed: %f ms\n", kernel_average_time);
    }

    out_dev->FromGPU(GetStream(), out.data());

    return miopenStatusSuccess;
}

template <typename Tgpu, typename Tref>
int NLLLossDriver<Tgpu, Tref>::RunForwardCPU()
{
    if(!std::isnan(divisor))
    {
        mloNLLLossReduceForward5dRunHost<Tgpu, Tref>(inputDesc,
                                                     targetDesc,
                                                     weightDesc,
                                                     in.data(),
                                                     target.data(),
                                                     weight.data(),
                                                     out_host.data(),
                                                     workspace_host.data(),
                                                     ignore_index,
                                                     divisor);
    }
    else
    {
        mloNLLLossUnreduceForwardRunHost<Tgpu, Tref>(inputDesc,
                                                     targetDesc,
                                                     weightDesc,
                                                     outputDesc,
                                                     in.data(),
                                                     target.data(),
                                                     weight.data(),
                                                     out_host.data(),
                                                     ignore_index);
    }
    return miopenStatusSuccess;
}

template <typename Tgpu, typename Tref>
int NLLLossDriver<Tgpu, Tref>::RunBackwardGPU()
{
    float kernel_total_time = 0.0;
    float kernel_first_time = 0.0;

    Timer t;
    START_TIME

    for(int i = 0; i < inflags.GetValueInt("iter"); i++)
    {
        if(!std::isnan(divisor))
        {
            miopenNLLLossReduceBackward(GetHandle(),
                                        inputGradDesc,
                                        in_grad_dev->GetMem(),
                                        targetDesc,
                                        target_dev->GetMem(),
                                        weightDesc,
                                        weight_dev->GetMem(),
                                        outputGradDesc,
                                        out_grad_dev->GetMem(),
                                        ignore_index,
                                        divisor);
        }
        else
        {
            miopenNLLLossUnreduceBackward(GetHandle(),
                                          inputGradDesc,
                                          in_grad_dev->GetMem(),
                                          targetDesc,
                                          target_dev->GetMem(),
                                          weightDesc,
                                          weight_dev->GetMem(),
                                          outputGradDesc,
                                          out_grad_dev->GetMem(),
                                          ignore_index);
        }

        float time = 0.0;
        miopenGetKernelTime(GetHandle(), &time);
        kernel_total_time += time;
        if(i == 0)
            kernel_first_time = time;
    }

    if(inflags.GetValueInt("time") == 1)
    {
        STOP_TIME
        int iter = inflags.GetValueInt("iter");
        if(WALL_CLOCK)
            printf("Wall-clock Time Backward NLLLoss Elapsed: %f ms\n", t.gettime_ms() / iter);

        float kernel_average_time =
            iter > 1 ? (kernel_total_time - kernel_first_time) / (iter - 1) : kernel_first_time;
        printf("GPU Kernel Time Backward NLLLoss Elapsed: %f ms\n", kernel_average_time);
    }

    in_grad_dev->FromGPU(GetStream(), in_grad.data());

    return miopenStatusSuccess;
}

template <typename Tgpu, typename Tref>
int NLLLossDriver<Tgpu, Tref>::RunBackwardCPU()
{
    if(!std::isnan(divisor))
    {
        mloNLLLossReduceBackwardRunHost<Tgpu, Tref>(inputGradDesc,
                                                    targetDesc,
                                                    weightDesc,
                                                    in_grad_host.data(),
                                                    target.data(),
                                                    weight.data(),
                                                    out_grad.data(),
                                                    ignore_index,
                                                    divisor);
    }
    else
    {
        mloNLLLossUnreduceBackwardRunHost<Tgpu, Tref>(inputGradDesc,
                                                      targetDesc,
                                                      weightDesc,
                                                      outputGradDesc,
                                                      in_grad_host.data(),
                                                      target.data(),
                                                      weight.data(),
                                                      out_grad.data(),
                                                      ignore_index);
    }
    return miopenStatusSuccess;
}

template <typename Tgpu, typename Tref>
Tref NLLLossDriver<Tgpu, Tref>::GetTolerance()
{
    // Computation error of fp16 is ~2^13 (=8192) bigger than
    // the one of fp32 because mantissa is shorter by 13 bits.
    auto tolerance = std::is_same<Tgpu, float>::value ? 1.5e-6 : 8.2e-3;

    // bf16 mantissa has 7 bits, by 3 bits shorter than fp16.
    if(std::is_same<Tgpu, bfloat16>::value)
        tolerance *= 8.0;
    return tolerance;
}

template <typename Tgpu, typename Tref>
int NLLLossDriver<Tgpu, Tref>::VerifyForward()
{
    RunForwardCPU();
    const Tref tolerance = GetTolerance();
    auto error           = miopen::rms_range(out_host, out);

    if(!std::isfinite(error) || error > tolerance)
    {
        std::cout << "Forward NLLLoss FAILED: " << error << std::endl;
        return EC_VerifyFwd;
    }
    else
    {
        printf("Forward NLLLoss Verifies on CPU and GPU (err=%f)\n", error);
    }

    return miopenStatusSuccess;
}

template <typename Tgpu, typename Tref>
int NLLLossDriver<Tgpu, Tref>::VerifyBackward()
{
    RunBackwardCPU();
    const Tref tolerance = GetTolerance();
    auto error           = miopen::rms_range(in_grad_host, in_grad);

    if(!std::isfinite(error) || error > tolerance)
    {
        std::cout << "Backward NLLLoss FAILED: " << error << std::endl;
        return EC_VerifyFwd;
    }
    else
    {
        printf("Backward NLLLoss Verifies on CPU and GPU (err=%f)\n", error);
    }
    return miopenStatusSuccess;
}

#endif // GUARD_MIOPEN_NLLLOSS_DRIVER_HPP
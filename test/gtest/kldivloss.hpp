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
#include "../driver/tensor_driver.hpp"
#include "cpu_kldivloss.hpp"
#include "get_handle.hpp"
#include "random.hpp"
#include "tensor_holder.hpp"
#include "verify.hpp"
#include <gtest/gtest.h>
#include <miopen/kldivloss.hpp>
#include <miopen/miopen.h>

inline std::ostream& operator<<(std::ostream& os, const std::vector<size_t>& v)
{
    os << '{';
    for(int i = 0; i < v.size(); ++i)
    {
        if(i != 0)
            os << ',';
        os << v[i];
    }
    os << '}';
    return os;
}

struct KLDivLossTestCase
{
    std::vector<size_t> input;
    bool log_target;
    float divisor;

    friend std::ostream& operator<<(std::ostream& os, const KLDivLossTestCase& tc)
    {
        return os << " input:" << tc.input << " log_target:" << tc.log_target
                << " divisor:" << tc.divisor;
    }

    std::vector<size_t> GetInput() const { return input; }
};

inline std::vector<KLDivLossTestCase> KLDivLossForwardTestConfigs()
{ // dim, dims
    // clang-format off
    return {{{256, 4, 8732},false, 0.0f},
            {{256, 4, 8732},true, 0.0f},
            {{34, 4},false, 0},
            {{34, 4},true, 0}};
    // clang-format on
}

inline std::vector<KLDivLossTestCase> KLDivLossBackwardTestConfigs()
{ // dim, dims
    // clang-format off
    return {{{256, 4, 8732},false, 0.0f},
            {{256, 4, 8732},true, 0.0f},
            {{34, 4},false, 0.0f},
            {{34, 4},true, 0.0f}};
    // clang-format on
}

inline std::vector<KLDivLossTestCase> KLDivLossTestConfigs()
{
    std::vector<KLDivLossTestCase> tcs, temp;
    temp = KLDivLossForwardTestConfigs();
    tcs.insert(tcs.end(), temp.begin(), temp.end());
    // temp = KLDivLossBackwardTestConfigs();
    // tcs.insert(tcs.end(), temp.begin(), temp.end());
    return tcs;
}

inline std::vector<size_t> GetStrides(std::vector<size_t> input, bool contiguous)
{
    if(!contiguous)
        std::swap(input.front(), input.back());
    std::vector<size_t> strides(input.size());
    strides.back() = 1;
    for(int i = input.size() - 2; i >= 0; --i)
        strides[i] = strides[i + 1] * input[i + 1];
    if(!contiguous)
        std::swap(strides.front(), strides.back());
    return strides;
}

// FORWARD TEST
template <typename T = float>
struct KLDivLossTest : public ::testing::TestWithParam<KLDivLossTestCase>
{
protected:
    void SetUp() override
    {
        auto&& handle  = get_handle();
        kldivloss_config = GetParam();

        log_target     = kldivloss_config.log_target;
        divisor         = kldivloss_config.divisor;

        auto in_dim                    = kldivloss_config.GetInput();
        auto target_dim = in_dim;

        auto gen_input_value = [](auto...) {
            return prng::gen_A_to_B<T>(static_cast<T>(-10.0f), static_cast<T>(10.0f));
        };
        
        auto gen_target_value = [](auto...) {
            return prng::gen_A_to_B<T>(static_cast<T>(1e-2), static_cast<T>(10.0f));
        };

        auto in_strides = GetStrides(in_dim, true);
        input           = tensor<T>{in_dim, in_strides}.generate(gen_input_value);

        auto tar_strides = GetStrides(target_dim, true);
        target          = tensor<T>{target_dim, tar_strides}.generate(gen_target_value);

        auto out_dim     = divisor == 0.f ? in_dim : std::vector<size_t>{1};
        auto out_strides = GetStrides(out_dim, true);
        output           = tensor<T>{out_dim, out_strides};
        std::fill(output.begin(), output.end(), std::numeric_limits<T>::quiet_NaN());

        ref_output = tensor<T>{out_dim, out_strides};
        std::fill(ref_output.begin(), ref_output.end(), std::numeric_limits<T>::quiet_NaN());

        std::vector<size_t> workspace_lengths;
        // ws_sizeInBytes = divisor == 0.f
        //                      ? 0
        //                      : miopen::GetKLDivLossReducedForwardWorkspaceSize(
        //                            handle, input.desc, target.desc, output.desc);
        // if(ws_sizeInBytes == static_cast<size_t>(-1))
        //     GTEST_SKIP();

        // if(ws_sizeInBytes != 0)
        // {
        //     std::vector<size_t> workspace_dims;
        //     workspace_dims.push_back(ws_sizeInBytes / sizeof(T));

        //     workspace = tensor<T>{workspace_dims};
        //     std::fill(workspace.begin(), workspace.end(), std::numeric_limits<T>::quiet_NaN());

        //     ref_workspace = tensor<T>{workspace_dims};
        //     std::fill(
        //         ref_workspace.begin(), ref_workspace.end(), std::numeric_limits<T>::quiet_NaN());

        //     workspace_dev = handle.Write(workspace.data);
        // }

        input_dev  = handle.Write(input.data);
        target_dev = handle.Write(target.data);
        output_dev = handle.Write(output.data);
    }

    void RunTest()
    {
        auto&& handle = get_handle();

        miopenStatus_t status;

        if(divisor == 0.f)
        {
            cpu_kldivloss_forward_5d<T>(input, target, ref_output, log_target);

            status = miopen::KLDivLossUnreducedForward(handle,
                                                    input.desc,
                                                    input_dev.get(),
                                                    target.desc,
                                                    target_dev.get(),
                                                    output.desc,
                                                    output_dev.get(),
                                                    log_target);
        }
        else
        {
            // cpu_kldivloss_reduced_forward_5d(
            //     input, target, weight, ref_output, ref_workspace, ignore_index, divisor);
            // status         = miopen::NLLLossReduceForward(handle,
            //                                       workspace_dev.get(),
            //                                       ws_sizeInBytes,
            //                                       input.desc,
            //                                       input_dev.get(),
            //                                       target.desc,
            //                                       target_dev.get(),
            //                                       weight.desc,
            //                                       weight_dev.get(),
            //                                       output.desc,
            //                                       output_dev.get(),
            //                                       ignore_index,
            //                                       divisor);
            // workspace.data = handle.Read<T>(workspace_dev, workspace.data.size());
        }
        fflush(stdout);

        EXPECT_EQ(status, miopenStatusSuccess);

        output.data = handle.Read<T>(output_dev, output.data.size());
    }

    void Verify()
    {
        double threshold = std::numeric_limits<T>::epsilon();

        auto error = miopen::rms_range(ref_output, output);

        EXPECT_TRUE(miopen::range_distance(ref_output) == miopen::range_distance(output));
        EXPECT_TRUE(error < threshold * 10) << "Error output beyond tolerance Error:" << error
                                            << ",  Thresholdx10: " << threshold * 10;
    }
    KLDivLossTestCase kldivloss_config;

    tensor<T> input;
    tensor<T> target;
    tensor<T> output;
    tensor<T> ref_output;
    tensor<T> workspace;
    tensor<T> ref_workspace;

    bool log_target;
    float divisor;

    miopen::Allocator::ManageDataPtr input_dev;
    miopen::Allocator::ManageDataPtr target_dev;
    miopen::Allocator::ManageDataPtr workspace_dev;
    miopen::Allocator::ManageDataPtr output_dev;

    size_t ws_sizeInBytes;
};

// // BACKWARD TEST
// template <typename T = float>
// struct KLDivLossTestBwd : public ::testing::TestWithParam<KLDivLossTestCase>
// {
// protected:
//     void SetUp() override
//     {
//         auto&& handle  = get_handle();
//         kldivloss_config = GetParam();

//         ignore_index    = kldivloss_config.ignore_index;
//         weight_mode     = kldivloss_config.weight_mode;
//         auto contiguous = kldivloss_config.contiguous;
//         divisor         = kldivloss_config.divisor;

//         auto in_dim                    = kldivloss_config.GetInput();
//         std::vector<size_t> target_dim = {in_dim[0], in_dim[2], in_dim[3]};
//         std::vector<size_t> weight_dim = {in_dim[1]};

//         size_t numclass_C     = in_dim[1];
//         auto gen_target_value = [numclass_C](auto...) {
//             return prng::gen_A_to_B<int32_t>(0, numclass_C - 1);
//         };
//         auto gen_weight_value = [](auto...) {
//             return prng::gen_A_to_B<T>(static_cast<T>(-10), static_cast<T>(10));
//         };
//         auto gen_weight_one = [](auto...) { return static_cast<T>(1); };

//         auto gen_output_grad_value = [](auto...) {
//             return prng::gen_A_to_B<T>(static_cast<T>(-10), static_cast<T>(10));
//         };

//         auto in_strides = GetStrides(in_dim, true);
//         input_grad      = tensor<T>{in_dim, in_strides};
//         std::fill(input_grad.begin(), input_grad.end(), static_cast<T>(0.0f));

//         ref_input_grad = tensor<T>{in_dim, in_strides};
//         std::fill(ref_input_grad.begin(), ref_input_grad.end(), static_cast<T>(0.0f));

//         auto tar_strides = GetStrides(target_dim, contiguous);
//         target           = tensor<int32_t>{target_dim, tar_strides}.generate(gen_target_value);

//         auto weight_strides = GetStrides(weight_dim, true);
//         if(!weight_mode)
//             weight = tensor<T>{weight_dim, weight_strides}.generate(gen_weight_one);
//         else
//             weight = tensor<T>{weight_dim, weight_strides}.generate(gen_weight_value);

//         std::vector<size_t> out_grad_dim =
//             divisor == 0.f ? std::vector<size_t>{in_dim[0], in_dim[2], in_dim[3]}
//                            : std::vector<size_t>{1};
//         auto out_strides = GetStrides(out_grad_dim, true);
//         output_grad      = tensor<T>{out_grad_dim, out_strides}.generate(gen_output_grad_value);

//         input_grad_dev  = handle.Write(input_grad.data);
//         target_dev      = handle.Write(target.data);
//         weight_dev      = handle.Write(weight.data);
//         output_grad_dev = handle.Write(output_grad.data);
//     }

//     void RunTest()
//     {
//         auto&& handle = get_handle();

//         miopenStatus_t status;

//         if(divisor != 0.f)
//         {
//             cpu_nllloss_reduce_backward_4d(
//                 ref_input_grad, target, weight, output_grad, ignore_index, divisor);

//             status = miopen::NLLLossReduceBackward(handle,
//                                                    input_grad.desc,
//                                                    input_grad_dev.get(),
//                                                    target.desc,
//                                                    target_dev.get(),
//                                                    weight.desc,
//                                                    weight_dev.get(),
//                                                    output_grad.desc,
//                                                    output_grad_dev.get(),
//                                                    ignore_index,
//                                                    divisor);
//         }
//         else
//         {
//             cpu_nllloss_backward_4d<T>(ref_input_grad, target, weight, output_grad, ignore_index);

//             status = miopen::KLDivLossUnreducedBackward(handle,
//                                                      input_grad.desc,
//                                                      input_grad_dev.get(),
//                                                      target.desc,
//                                                      target_dev.get(),
//                                                      weight.desc,
//                                                      weight_dev.get(),
//                                                      output_grad.desc,
//                                                      output_grad_dev.get(),
//                                                      ignore_index);
//         }

//         EXPECT_EQ(status, miopenStatusSuccess);

//         input_grad.data = handle.Read<T>(input_grad_dev, input_grad.data.size());
//     }

//     void Verify()
//     {
//         double threshold = std::numeric_limits<T>::epsilon();
//         auto error       = miopen::rms_range(ref_input_grad, input_grad);
//         EXPECT_TRUE(miopen::range_distance(ref_input_grad) == miopen::range_distance(input_grad));
//         EXPECT_TRUE(error < threshold * 10) << "Error output beyond tolerance Error:" << error
//                                             << ",  Thresholdx10: " << threshold * 10;
//     }
//     KLDivLossTestCase kldivloss_config;

//     tensor<T> input_grad;
//     tensor<T> ref_input_grad;
//     tensor<int32_t> target;
//     tensor<T> weight;
//     tensor<T> output_grad;

//     bool weight_mode;
//     int32_t ignore_index;
//     float divisor;

//     miopen::Allocator::ManageDataPtr input_grad_dev;
//     miopen::Allocator::ManageDataPtr target_dev;
//     miopen::Allocator::ManageDataPtr weight_dev;
//     miopen::Allocator::ManageDataPtr output_grad_dev;
// };
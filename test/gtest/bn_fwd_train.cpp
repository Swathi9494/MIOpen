/*******************************************************************************
 *
 * MIT License
 *
 * Copyright (c) 2023 Advanced Micro Devices, Inc.
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

#include "bn.hpp"

// XDataType       : half
// YDataYype       : half
// ScaleDataType   : half
// BiasDataType    : half
// MeanVarDataType : float
struct GPU_BN_FWD_Train_FP16_Small
    : BNFwdTrainTest<half_float::half, half_float::half, half_float::half, half_float::half, float>
{
};

struct GPU_BN_FWD_Train_FP16_Large
    : BNFwdTrainTest<half_float::half, half_float::half, half_float::half, half_float::half, float>
{
};

// XDataType       : bfloat16
// YDataYype       : bfloat16
// ScaleDataType   : bfloat16
// BiasDataType    : bfloat16
// MeanVarDataType : float
struct GPU_BN_FWD_Train_BFP16_Small : BNFwdTrainTest<bfloat16, bfloat16, bfloat16, bfloat16, float>
{
};

struct GPU_BN_FWD_Train_BFP16_Large : BNFwdTrainTest<bfloat16, bfloat16, bfloat16, bfloat16, float>
{
};

struct GPU_BN_FWD_Train_FP32_Small : BNFwdTrainTest<float, float, float, float, float>
{
};

struct GPU_BN_FWD_Train_FP32_Large : BNFwdTrainTest<float, float, float, float, float>
{
};

struct GPU_BN_FWD_Train_FP64_Small : BNFwdTrainTest<double, double, double, double, double>
{
};

struct GPU_BN_FWD_Train_FP64_Large : BNFwdTrainTest<double, double, double, double, double>
{
};

// fp16
TEST_P(GPU_BN_FWD_Train_FP16_Small, BnV1SmallFWD_TrainCKfp16) {}
TEST_P(GPU_BN_FWD_Train_FP16_Large, BnV2LargeFWD_TrainCKfp16) {}

// bfp16
TEST_P(GPU_BN_FWD_Train_BFP16_Small, BnV1SmallFWD_TrainCKbfp16) {}
TEST_P(GPU_BN_FWD_Train_BFP16_Large, BnV2LargeFWD_TrainCKbfp16) {}

// fp32 (float)
TEST_P(GPU_BN_FWD_Train_FP32_Small, BnV1SmallFWD_TrainCKfp32) {}
TEST_P(GPU_BN_FWD_Train_FP32_Large, BnV2LargeFWD_TrainCKfp32) {}

// fp64
TEST_P(GPU_BN_FWD_Train_FP64_Small, BnV1SmallFWD_TrainCKfp64) {}
TEST_P(GPU_BN_FWD_Train_FP64_Large, BnV2LargeFWD_TrainCKfp64) {}

// Assuming miopenTensorLayout_t and testAPI_t are the types of your enums
std::string LayoutToString(int tensor_format)
{
    switch(tensor_format)
    {
    case miopenTensorNCHW: return "NCHW";
    case miopenTensorNHWC: return "NHWC";
    default: return "UnknownTensorFormat";
    }
}

std::string ApiVerisonToString(int api_version)
{
    switch(api_version)
    {
    case testBNAPIV1: return "testBNAPIV1";
    case testBNAPIV2: return "testBNAPIV2";
    default: return "UnknownAPIVersion";
    }
}

// Custom test name generator to handle enums
struct TestNameGenerator
{
    std::string operator()(
        const testing::TestParamInfo<std::tuple<BNTestCase, miopenTensorLayout_t, BNApiType>>& info)
        const
    {
        const auto& layout_type = std::get<1>(info.param);
        const auto& api_type    = std::get<2>(info.param);

        std::string tensor_name = LayoutToString(layout_type);
        std::string api_name    = ApiVerisonToString(api_type);

        return tensor_name + "_" + api_name + "_" + std::to_string(info.index);
    }
};

// fp16
INSTANTIATE_TEST_SUITE_P(Smoke,
                         GPU_BN_FWD_Train_FP16_Small,
                         testing::Combine(testing::ValuesIn(NetworkSmall<BNTestCase>()),
                                          testing::ValuesIn({miopenTensorNCHW, miopenTensorNHWC}),
                                          testing::ValuesIn({testBNAPIV1})),
                         TestNameGenerator());

INSTANTIATE_TEST_SUITE_P(Smoke,
                         GPU_BN_FWD_Train_FP16_Large,
                         testing::Combine(testing::ValuesIn(NetworkLarge<BNTestCase>()),
                                          testing::ValuesIn({miopenTensorNCHW, miopenTensorNHWC}),
                                          testing::ValuesIn({testBNAPIV2})),
                         TestNameGenerator());

// bfp16
INSTANTIATE_TEST_SUITE_P(Smoke,
                         GPU_BN_FWD_Train_BFP16_Small,
                         testing::Combine(testing::ValuesIn(NetworkSmall<BNTestCase>()),
                                          testing::ValuesIn({miopenTensorNCHW, miopenTensorNHWC}),
                                          testing::ValuesIn({testBNAPIV1})),
                         TestNameGenerator());

INSTANTIATE_TEST_SUITE_P(Smoke,
                         GPU_BN_FWD_Train_BFP16_Large,
                         testing::Combine(testing::ValuesIn(NetworkLarge<BNTestCase>()),
                                          testing::ValuesIn({miopenTensorNCHW, miopenTensorNHWC}),
                                          testing::ValuesIn({testBNAPIV2})),
                         TestNameGenerator());

// fp32
INSTANTIATE_TEST_SUITE_P(Smoke,
                         GPU_BN_FWD_Train_FP32_Small,
                         testing::Combine(testing::ValuesIn(NetworkLarge<BNTestCase>()),
                                          testing::ValuesIn({miopenTensorNCHW, miopenTensorNHWC}),
                                          testing::ValuesIn({testBNAPIV2})),
                         TestNameGenerator());

INSTANTIATE_TEST_SUITE_P(Smoke,
                         GPU_BN_FWD_Train_FP32_Large,
                         testing::Combine(testing::ValuesIn(NetworkLarge<BNTestCase>()),
                                          testing::ValuesIn({miopenTensorNCHW, miopenTensorNHWC}),
                                          testing::ValuesIn({testBNAPIV2})),
                         TestNameGenerator());
// fp64
INSTANTIATE_TEST_SUITE_P(Smoke,
                         GPU_BN_FWD_Train_FP64_Small,
                         testing::Combine(testing::ValuesIn(NetworkLarge<BNTestCase>()),
                                          testing::ValuesIn({miopenTensorNCHW, miopenTensorNHWC}),
                                          testing::ValuesIn({testBNAPIV2})),
                         TestNameGenerator());

INSTANTIATE_TEST_SUITE_P(Smoke,
                         GPU_BN_FWD_Train_FP64_Large,
                         testing::Combine(testing::ValuesIn(NetworkLarge<BNTestCase>()),
                                          testing::ValuesIn({miopenTensorNCHW, miopenTensorNHWC}),
                                          testing::ValuesIn({testBNAPIV2})),
                         TestNameGenerator());

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
#include <tuple>
#include <string_view>

#include "gtest_common.hpp"

#include "../conv2d.hpp"

namespace {

auto GetTestCases()
{
    // MIOPEN_DEBUG_CONV_IMPLICIT_GEMM_HIP_BWD_V4R1_XDLOPS is reqired due to env_bwd case
    // for this particulaer case it's not needed, but must be there to simplify the code
    const auto env_fwd = std::tuple{
        std::pair{MIOPEN_FIND_ENFORCE, "SEARCH_DB_UPDATE"},
        std::pair{MIOPEN_DEBUG_TUNING_ITERATIONS_MAX, 5},
        std::pair{MIOPEN_DEBUG_CONV_IMPLICIT_GEMM_HIP_BWD_V4R1_XDLOPS, false},
        std::pair{MIOPEN_DEBUG_CONVOLUTION_ATTRIB_FP16_ALT_IMPL, 0},
        std::pair{MIOPEN_FIND_MODE, "normal"},
        std::pair{MIOPEN_DEBUG_FIND_ONLY_SOLVER, "ConvHipImplicitGemmForwardV4R4Xdlops"}};

    // MIOPEN_DEBUG_CONV_IMPLICIT_GEMM_HIP_BWD_V4R1_XDLOPS is reqired due to env_bwd case
    // for this particulaer case it's not needed, but must be there to simplify the code
    const auto env_fwd_padded =
        std::tuple{std::pair{MIOPEN_FIND_ENFORCE, "SEARCH_DB_UPDATE"},
                   std::pair{MIOPEN_DEBUG_TUNING_ITERATIONS_MAX, 5},
                   std::pair{MIOPEN_DEBUG_CONV_IMPLICIT_GEMM_HIP_BWD_V4R1_XDLOPS, false},
                   std::pair{MIOPEN_DEBUG_CONVOLUTION_ATTRIB_FP16_ALT_IMPL, 0},
                   std::pair{MIOPEN_FIND_MODE, "normal"},
                   std::pair{MIOPEN_DEBUG_FIND_ONLY_SOLVER,
                             "ConvHipImplicitGemmForwardV4R4Xdlops_Padded_Gemm"}};

    // MIOPEN_DEBUG_CONV_IMPLICIT_GEMM_HIP_BWD_V4R1_XDLOPS is reqired due to env_bwd case
    // for this particulaer case it's not needed, but must be there to simplify the code
    const auto env_fwd_v4r5 = std::tuple{
        std::pair{MIOPEN_FIND_ENFORCE, "SEARCH_DB_UPDATE"},
        std::pair{MIOPEN_DEBUG_TUNING_ITERATIONS_MAX, 5},
        std::pair{MIOPEN_DEBUG_CONV_IMPLICIT_GEMM_HIP_BWD_V4R1_XDLOPS, false},
        std::pair{MIOPEN_DEBUG_CONVOLUTION_ATTRIB_FP16_ALT_IMPL, 0},
        std::pair{MIOPEN_FIND_MODE, "normal"},
        std::pair{MIOPEN_DEBUG_FIND_ONLY_SOLVER, "ConvHipImplicitGemmForwardV4R5Xdlops"}};

    // WORKAROUND_ISSUE_1206 disables this solver for FP32 due to precision issues.
    // WORKAROUND_SWDEV_329642 disables this solver on MI200 for BF16.
    // However we still want to check that these cases are not broken and therefore use
    // MIOPEN_DEBUG_CONV_IMPLICIT_GEMM_HIP_BWD_V4R1_XDLOPS=1 to enable the solver.
    const auto env_bwd = std::tuple{
        std::pair{MIOPEN_FIND_ENFORCE, "SEARCH_DB_UPDATE"},
        std::pair{MIOPEN_DEBUG_TUNING_ITERATIONS_MAX, 5},
        std::pair{MIOPEN_DEBUG_CONV_IMPLICIT_GEMM_HIP_BWD_V4R1_XDLOPS, true},
        std::pair{MIOPEN_DEBUG_CONVOLUTION_ATTRIB_FP16_ALT_IMPL, 0},
        std::pair{MIOPEN_FIND_MODE, "normal"},
        std::pair{MIOPEN_DEBUG_FIND_ONLY_SOLVER, "ConvHipImplicitGemmBwdDataV4R1Xdlops"}};

    // MIOPEN_DEBUG_CONV_IMPLICIT_GEMM_HIP_BWD_V4R1_XDLOPS is reqired due to env_bwd case
    // for this particulaer case it's not needed, but must be there to simplify the code
    const auto env_wrw =
        std::tuple{std::pair{MIOPEN_FIND_ENFORCE, "SEARCH_DB_UPDATE"},
                   std::pair{MIOPEN_DEBUG_TUNING_ITERATIONS_MAX, 5},
                   std::pair{MIOPEN_DEBUG_CONV_IMPLICIT_GEMM_HIP_BWD_V4R1_XDLOPS, false},
                   std::pair{MIOPEN_DEBUG_CONVOLUTION_ATTRIB_FP16_ALT_IMPL, 0},
                   std::pair{MIOPEN_FIND_MODE, "normal"},
                   std::pair{MIOPEN_DEBUG_FIND_ONLY_SOLVER, "ConvHipImplicitGemmWrwV4R4Xdlops"}};

    // MIOPEN_DEBUG_CONV_IMPLICIT_GEMM_HIP_BWD_V4R1_XDLOPS is reqired due to env_bwd case
    // for this particulaer case it's not needed, but must be there to simplify the code
    const auto env_wrw_padded = std::tuple{
        std::pair{MIOPEN_FIND_ENFORCE, "SEARCH_DB_UPDATE"},
        std::pair{MIOPEN_DEBUG_TUNING_ITERATIONS_MAX, 5},
        std::pair{MIOPEN_DEBUG_CONV_IMPLICIT_GEMM_HIP_BWD_V4R1_XDLOPS, false},
        std::pair{MIOPEN_DEBUG_CONVOLUTION_ATTRIB_FP16_ALT_IMPL, 0},
        std::pair{MIOPEN_FIND_MODE, "normal"},
        std::pair{MIOPEN_DEBUG_FIND_ONLY_SOLVER, "ConvHipImplicitGemmWrwV4R4Xdlops_Padded_Gemm"}};

    const std::string vf = " --verbose --disable-backward-data --disable-backward-weights";
    const std::string vb = " --verbose --disable-forward --disable-backward-weights";
    const std::string vw = " --verbose --disable-forward --disable-backward-data";

    return std::vector{
        // clang-format off
    std::pair{env_fwd, vf + " --input 128 48 13 13 --weights 192 48 1 1 --pads_strides_dilations 0 0 1 1 1 1"},
    std::pair{env_bwd, vb + " --input 64 64 55 55 --weights 64 64 1 1 --pads_strides_dilations 0 0 1 1 1 1"},
    std::pair{env_wrw, vw + " --input 1 192 28 28 --weights 16 192 1 1 --pads_strides_dilations 0 0 1 1 1 1"},
    std::pair{env_fwd_padded, vf + " --input 16 1 7 7 --weights 1 1 3 3 --pads_strides_dilations 0 0 1 1 1 1"},
    std::pair{env_wrw_padded, vw + " --input 256 2 5 5 --weights 1 2 3 3 --pads_strides_dilations 1 1 2 2 1 1"},
    std::pair{env_fwd_v4r5, vf + " --input 128 16 54 54 --weights 64 16 3 3 --pads_strides_dilations 1 1 1 1 1 1"}
        // clang-format on
    };
}

using TestCase = decltype(GetTestCases())::value_type;

bool IsTestSupportedForDevice()
{
    using e_mask = enabled<Gpu::Default>;
    using d_mask = disabled<Gpu::gfx900, Gpu::gfx906>;
    return ::IsTestSupportedForDevMask<d_mask, e_mask>();
}

} // namespace

class GPU_Conv2dTuningV4Rx_FP16 : public HalfTestCase<std::vector<TestCase>>
{
};

class GPU_Conv2dTuningV4Rx_BFP16 : public Bf16TestCase<std::vector<TestCase>>
{
};

TEST_P(GPU_Conv2dTuningV4Rx_FP16, HalfTest_smoke_solver_ConvHipImplicitGemmDataV4RxXdlops)
{
    if(IsTestSupportedForDevice())
    {
        invoke_with_params<conv2d_driver, GPU_Conv2dTuningV4Rx_FP16>(tuning_check);
    }
    else
    {
        GTEST_SKIP();
    }
};

TEST_P(GPU_Conv2dTuningV4Rx_BFP16, Bf16Test_smoke_solver_ConvHipImplicitGemmDataV4RxXdlops)
{
    if(IsTestSupportedForDevice())
    {
        invoke_with_params<conv2d_driver, GPU_Conv2dTuningV4Rx_BFP16>(tuning_check);
    }
    else
    {
        GTEST_SKIP();
    }
};

INSTANTIATE_TEST_SUITE_P(Smoke, GPU_Conv2dTuningV4Rx_FP16, testing::Values(GetTestCases()));

INSTANTIATE_TEST_SUITE_P(Smoke, GPU_Conv2dTuningV4Rx_BFP16, testing::Values(GetTestCases()));

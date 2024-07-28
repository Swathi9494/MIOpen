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
#include "miopen/bfloat16.hpp"
#include <miopen/env.hpp>
#include "nllloss.hpp"

MIOPEN_DECLARE_ENV_VAR_STR(MIOPEN_TEST_FLOAT_ARG)
MIOPEN_DECLARE_ENV_VAR_BOOL(MIOPEN_TEST_ALL)

namespace nllloss {

std::string GetFloatArg()
{
    const auto& tmp = miopen::GetStringEnv(ENV(MIOPEN_TEST_FLOAT_ARG));
    if(tmp.empty())
    {
        return "";
    }
    return tmp;
}

struct NLLLossTestFloat : NLLLossTest<float>
{
};

struct NLLLossTestHalf : NLLLossTest<half>
{
};

struct NLLLossTestBFloat16 : NLLLossTest<bfloat16>
{
};

struct NLLLossTestFloatBwd : NLLLossTestBwd<float>
{
};

struct NLLLossTestHalfBwd : NLLLossTestBwd<half>
{
};

struct NLLLossTestBFloat16Bwd : NLLLossTestBwd<bfloat16>
{
};

} // namespace nllloss
using namespace nllloss;

// FORWARD TEST
TEST_P(NLLLossTestFloat, NLLLossTest)
{
    if((miopen::IsEnabled(ENV(MIOPEN_TEST_ALL)) && GetFloatArg() == "--float") ||
       miopen::IsUnset(ENV(MIOPEN_TEST_ALL)))
    {
        RunTest();
        Verify();
    }
    else
    {
        GTEST_SKIP();
    }
};

TEST_P(NLLLossTestHalf, NLLLossTest)
{
    if((miopen::IsEnabled(ENV(MIOPEN_TEST_ALL)) && GetFloatArg() == "--half") ||
       miopen::IsUnset(ENV(MIOPEN_TEST_ALL)))
    {
        RunTest();
        Verify();
    }
    else
    {
        GTEST_SKIP();
    }
};

TEST_P(NLLLossTestBFloat16, NLLLossTest)
{
    if((miopen::IsEnabled(ENV(MIOPEN_TEST_ALL)) && GetFloatArg() == "--bfloat16") ||
       miopen::IsUnset(ENV(MIOPEN_TEST_ALL)))
    {
        RunTest();
        Verify();
    }
    else
    {
        GTEST_SKIP();
    }
};

INSTANTIATE_TEST_SUITE_P(NLLLossTestSet, NLLLossTestFloat, testing::ValuesIn(NLLLossTestConfigs()));
INSTANTIATE_TEST_SUITE_P(NLLLossTestSet, NLLLossTestHalf, testing::ValuesIn(NLLLossTestConfigs()));
INSTANTIATE_TEST_SUITE_P(NLLLossTestSet,
                         NLLLossTestBFloat16,
                         testing::ValuesIn(NLLLossTestConfigs()));

// BACKWARD TEST
TEST_P(NLLLossTestFloatBwd, NLLLossTestBwd)
{
    if((miopen::IsEnabled(ENV(MIOPEN_TEST_ALL)) && GetFloatArg() == "--float") ||
       miopen::IsUnset(ENV(MIOPEN_TEST_ALL)))
    {
        RunTest();
        Verify();
    }
    else
    {
        GTEST_SKIP();
    }
};

TEST_P(NLLLossTestHalfBwd, NLLLossTestBwd)
{
    if((miopen::IsEnabled(ENV(MIOPEN_TEST_ALL)) && GetFloatArg() == "--half") ||
       miopen::IsUnset(ENV(MIOPEN_TEST_ALL)))
    {
        RunTest();
        Verify();
    }
    else
    {
        GTEST_SKIP();
    }
};

TEST_P(NLLLossTestBFloat16Bwd, NLLLossTestBwd)
{
    if((miopen::IsEnabled(ENV(MIOPEN_TEST_ALL)) && GetFloatArg() == "--bfloat16") ||
       miopen::IsUnset(ENV(MIOPEN_TEST_ALL)))
    {
        RunTest();
        Verify();
    }
    else
    {
        GTEST_SKIP();
    }
};

INSTANTIATE_TEST_SUITE_P(NLLLossTestSet,
                         NLLLossTestFloatBwd,
                         testing::ValuesIn(NLLLossTestConfigs()));
INSTANTIATE_TEST_SUITE_P(NLLLossTestSet,
                         NLLLossTestHalfBwd,
                         testing::ValuesIn(NLLLossTestConfigs()));
INSTANTIATE_TEST_SUITE_P(NLLLossTestSet,
                         NLLLossTestBFloat16Bwd,
                         testing::ValuesIn(NLLLossTestConfigs()));

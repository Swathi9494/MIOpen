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
#pragma once

#include <miopen/smoothl1loss/problem_description.hpp>
#include <miopen/solver.hpp>

namespace miopen {

namespace solver {

namespace smoothl1loss {

using SmoothL1LossForwardSolverBase =
    NonTunableSolverBase<ExecutionContext, miopen::smoothl1loss::ForwardProblemDescription>;

struct SmoothL1LossForward final : SmoothL1LossForwardSolverBase
{
    const std::string& SolverDbId() const override { return GetSolverDbId<SmoothL1LossForward>(); }

    bool
    IsApplicable(const ExecutionContext& context,
                 const miopen::smoothl1loss::ForwardProblemDescription& problem) const override;
    ConvSolution
    GetSolution(const ExecutionContext& context,
                const miopen::smoothl1loss::ForwardProblemDescription& problem) const override;
    std::size_t
    GetWorkspaceSize(const ExecutionContext& context,
                     const miopen::smoothl1loss::ForwardProblemDescription& problem) const override;
    bool MayNeedWorkspace() const override { return true; }
};

using SmoothL1LossBackwardSolverBase =
    NonTunableSolverBase<ExecutionContext, miopen::smoothl1loss::BackwardProblemDescription>;

struct SmoothL1LossBackward final : SmoothL1LossBackwardSolverBase
{
    const std::string& SolverDbId() const override { return GetSolverDbId<SmoothL1LossBackward>(); }

    bool
    IsApplicable(const ExecutionContext& context,
                 const miopen::smoothl1loss::BackwardProblemDescription& problem) const override;
    ConvSolution
    GetSolution(const ExecutionContext& context,
                const miopen::smoothl1loss::BackwardProblemDescription& problem) const override;
};

} // namespace smoothl1loss

} // namespace solver

} // namespace miopen

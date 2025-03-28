/***************************************************************************************************
 * Copyright (c) 2017 - 2024 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 **************************************************************************************************/

/*! \file snngrow/snngrow_backend/spikegemm/gemm/thread/mma_spike.h
    *
    * Copyright (c) 2024 Beijing Institute of Technology AETAS Lab. and Utarn Technology Co., Ltd.  All rights reserved.
    *
    * Unless required by applicable law or agreed to in writing,
    * software distributed under the License is distributed on an
    * "AS IS" BASIS, WITHOUT ARRANTIES OR CONDITIONS OF ANY KIND, either express or
    * implied.
*/
#pragma once

#include "cutlass/cutlass.h"
#include "cutlass/tensor_ref.h"
#include "cutlass/array.h"
#include "cutlass/numeric_types.h"
#include "cutlass/layout/matrix.h"
#include "cutlass/arch/mma.h"
#include "cutlass/gemm/gemm.h"
#include "cutlass/gemm/thread/mma.h"

#include "arch/mma_spike.h"

namespace cutlass {
namespace gemm {
namespace thread {

/////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////////

/// Gemplate that handles all packed matrix layouts
template <
    /// Size of the Gemm problem - concept: gemm::GemmShape<>
    typename Shape_,
    /// Layout of A matrix (concept: layout::MapFunc)
    typename LayoutA_,
    /// Data type of B elements
    typename ElementB_,
    /// Layout of B matrix (concept: layout::MapFunc)
    typename LayoutB_,
    /// Element type of C matrix
    typename ElementC_,
    /// Layout of C matrix (concept: layout::MapFunc)
    typename LayoutC_,
    /// Operator used to compute GEMM
    typename Operator_
>
struct MmaGeneric<
    Shape_,
    bool,
    LayoutA_,
    ElementB_,
    LayoutB_,
    ElementC_,
    LayoutC_,
    Operator_> {

    /// Size of the Gemm problem - concept: gemm::GemmShape<>
    using Shape = Shape_;

    /// Data type of operand A
    using ElementA = bool;

    /// Layout of A matrix (concept: layout::MapFunc)
    using LayoutA = LayoutA_;

    /// Data type of operand B
    using ElementB = ElementB_;

    /// Layout of B matrix (concept: layout::MapFunc)
    using LayoutB = LayoutB_;

    /// Element type of operand C
    using ElementC = ElementC_;

    /// Layout of C matrix (concept: layout::MapFunc)
    using LayoutC = LayoutC_;

    /// Underlying mathematical operator
    using Operator = Operator_;

    /// A operand storage
    using FragmentA = Array<ElementA, Shape::kMK>;

    /// B operand storage
    using FragmentB = Array<ElementB, Shape::kKN>;

    /// C operand storage
    using FragmentC = Array<ElementC, Shape::kMN>;

    /// Instruction
    using MmaOp = arch::Mma<
        gemm::GemmShape<1,1,1>,
        1,
        ElementA,
        LayoutA,
        ElementB,
        LayoutB,
        ElementC,
        LayoutC,
        Operator>;

    static bool const kMultipleOf2 = ((Shape::kM % 2 == 0) && (Shape::kN % 2 == 0));

    static bool const kAllFp32 = platform::is_same<ElementA, float>::value &&
        platform::is_same<ElementB, float>::value &&
        platform::is_same<ElementC, float>::value;
    //
    // Methods
    //

    /// Computes a matrix product D = A * B + C
    CUTLASS_HOST_DEVICE
    void operator()(
        FragmentC & D,
        FragmentA const & A,
        FragmentB const & B,
        FragmentC const & C) {

        // Array<bool, 4> const *a_ref = reinterpret_cast<Array<bool, 4> const *>(&A);
        TensorRef<ElementA const, LayoutA> a_ref(
        reinterpret_cast<ElementA const *>(&A), LayoutA::packed({Shape::kM, Shape::kK}));

        cutlass::TensorRef<ElementB const, LayoutB> b_ref(
        reinterpret_cast<ElementB const *>(&B), LayoutB::packed({Shape::kK, Shape::kN}));

        cutlass::TensorRef<ElementC, LayoutC> d_ref(
        reinterpret_cast<ElementC *>(&D), LayoutC::packed(make_Coord(Shape::kM, Shape::kN)));

        MmaOp mma_op;

        // Copy accumulators
        D = C;

        // Compute matrix product
        CUTLASS_PRAGMA_UNROLL
        for (int k = 0; k < Shape::kK; ++k) {
            CUTLASS_PRAGMA_UNROLL
            for (int n = 0; n < Shape::kN; ++n) {
    
                CUTLASS_PRAGMA_UNROLL
                for (int m = 0; m < Shape::kM; ++m) {
        
                    int m_serpentine = (n % 2) ? (Shape::kM - 1 - m) : m;
        
                    MatrixCoord mn(m_serpentine, n);
                    MatrixCoord mk(m_serpentine, k);
                    MatrixCoord kn(k, n);
        
                    Array<ElementC, 1> d;
                    Array<ElementA, 1> a;
                    Array<ElementB, 1> b;
        
                    d[0] = d_ref.at(mn);
                    a[0] = a_ref.at(mk);
                    b[0] = b_ref.at(kn);
        
                    mma_op(d, a, b, d);
        
                    d_ref.at(mn) = d[0];
                }
            }
        }
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////

/// Gemplate that handles all packed matrix layouts
template <
    /// Size of the Gemm problem - concept: gemm::GemmShape<>
    typename Shape_,
    /// Data type of A elements
    typename ElementA_,
    /// Layout of A matrix (concept: layout::MapFunc)
    typename LayoutA_,
    /// Layout of B matrix (concept: layout::MapFunc)
    typename LayoutB_,
    /// Element type of C matrix
    typename ElementC_,
    /// Layout of C matrix (concept: layout::MapFunc)
    typename LayoutC_,
    /// Operator used to compute GEMM
    typename Operator_
>
struct MmaGeneric<
    Shape_,
    ElementA_,
    LayoutA_,
    bool,
    LayoutB_,
    ElementC_,
    LayoutC_,
    Operator_> {

    /// Size of the Gemm problem - concept: gemm::GemmShape<>
    using Shape = Shape_;

    /// Data type of operand A
    using ElementA = ElementA_;

    /// Layout of A matrix (concept: layout::MapFunc)
    using LayoutA = LayoutA_;

    /// Data type of operand B
    using ElementB = bool;

    /// Layout of B matrix (concept: layout::MapFunc)
    using LayoutB = LayoutB_;

    /// Element type of operand C
    using ElementC = ElementC_;

    /// Layout of C matrix (concept: layout::MapFunc)
    using LayoutC = LayoutC_;

    /// Underlying mathematical operator
    using Operator = Operator_;

    /// A operand storage
    using FragmentA = Array<ElementA, Shape::kMK>;

    /// B operand storage
    using FragmentB = Array<ElementB, Shape::kKN>;

    /// C operand storage
    using FragmentC = Array<ElementC, Shape::kMN>;

    /// Instruction
    using MmaOp = arch::Mma<
        gemm::GemmShape<1,1,1>,
        1,
        ElementA,
        LayoutA,
        ElementB,
        LayoutB,
        ElementC,
        LayoutC,
        Operator>;

    static bool const kMultipleOf2 = ((Shape::kM % 2 == 0) && (Shape::kN % 2 == 0));

    static bool const kAllFp32 = platform::is_same<ElementA, float>::value &&
        platform::is_same<ElementB, float>::value &&
        platform::is_same<ElementC, float>::value;
    //
    // Methods
    //

    /// Computes a matrix product D = A * B + C
    CUTLASS_HOST_DEVICE
    void operator()(
        FragmentC & D,
        FragmentA const & A,
        FragmentB const & B,
        FragmentC const & C) {

        // Array<bool, 4> const *a_ref = reinterpret_cast<Array<bool, 4> const *>(&A);
        TensorRef<ElementA const, LayoutA> a_ref(
        reinterpret_cast<ElementA const *>(&A), LayoutA::packed({Shape::kM, Shape::kK}));

        cutlass::TensorRef<ElementB const, LayoutB> b_ref(
        reinterpret_cast<ElementB const *>(&B), LayoutB::packed({Shape::kK, Shape::kN}));

        cutlass::TensorRef<ElementC, LayoutC> d_ref(
        reinterpret_cast<ElementC *>(&D), LayoutC::packed(make_Coord(Shape::kM, Shape::kN)));

        MmaOp mma_op;

        // Copy accumulators
        D = C;

        // Compute matrix product
        CUTLASS_PRAGMA_UNROLL
        for (int k = 0; k < Shape::kK; ++k) {
            CUTLASS_PRAGMA_UNROLL
            for (int n = 0; n < Shape::kN; ++n) {
    
                CUTLASS_PRAGMA_UNROLL
                for (int m = 0; m < Shape::kM; ++m) {
        
                    int m_serpentine = (n % 2) ? (Shape::kM - 1 - m) : m;
        
                    MatrixCoord mn(m_serpentine, n);
                    MatrixCoord mk(m_serpentine, k);
                    MatrixCoord kn(k, n);
        
                    Array<ElementC, 1> d;
                    Array<ElementA, 1> a;
                    Array<ElementB, 1> b;
        
                    d[0] = d_ref.at(mn);
                    a[0] = a_ref.at(mk);
                    b[0] = b_ref.at(kn);
        
                    mma_op(d, a, b, d);
        
                    d_ref.at(mn) = d[0];
                }
            }
        }
    }
};

/////////////////////////////////////////////////////////////////////////////////////////////////

/// Gemplate that handles conventional layouts for FFMA and DFMA GEMM
template <
  /// Size of the Gemm problem - concept: gemm::GemmShape<>
  typename Shape_,
  /// Layout of A matrix (concept: layout::MapFunc)
  typename LayoutA_,
  /// Data type of B elements
  typename ElementB_,
  /// Layout of B matrix (concept: layout::MapFunc)
  typename LayoutB_,
  /// Element type of C matrix
  typename ElementC_,
  /// Layout of C matrix (concept: layout::MapFunc)
  typename LayoutC_
>
struct Mma<
  Shape_,
  bool,
  LayoutA_,
  ElementB_,
  LayoutB_,
  ElementC_,
  LayoutC_,
  arch::OpMultiplyAdd,
  bool> {

  /// Size of the Gemm problem - concept: gemm::GemmShape<>
  using Shape = Shape_;

  /// Data type of operand A
  using ElementA = bool;

  /// Layout of A matrix (concept: layout::MapFunc)
  using LayoutA = LayoutA_;

  /// Data type of operand B
  using ElementB = ElementB_;

  /// Layout of B matrix (concept: layout::MapFunc)
  using LayoutB = LayoutB_;

  /// Element type of operand C
  using ElementC = ElementC_;

  /// Layout of C matrix (concept: layout::MapFunc)
  using LayoutC = LayoutC_;

  /// Underlying mathematical operator
  using Operator = arch::OpMultiplyAdd;

  /// A operand storage
  using FragmentA = Array<ElementA, Shape::kMK>;

  /// B operand storage
  using FragmentB = Array<ElementB, Shape::kKN>;

  /// C operand storage
  using FragmentC = Array<ElementC, Shape::kMN>;

  /// Underlying matrix multiply operator (concept: arch::Mma)
  using ArchMmaOperator = typename MmaGeneric<
                                    Shape,
                                    bool,
                                    LayoutA,
                                    ElementB,
                                    LayoutB,
                                    ElementC,
                                    LayoutC,
                                    Operator>::MmaOp;
  //
  // Methods
  //

  /// Computes a matrix product D = A * B + C
  CUTLASS_HOST_DEVICE
  void operator()(
    FragmentC & D,
    FragmentA const & A,
    FragmentB const & B,
    FragmentC const & C) {

    MmaGeneric<
      Shape,
      bool,
      LayoutA,
      ElementB,
      LayoutB,
      ElementC,
      LayoutC,
      Operator> mma;
    mma(D, A, B, C);
  }
};

/////////////////////////////////////////////////////////////////////////////////////////////////

/// Gemplate that handles conventional layouts for FFMA and DFMA GEMM
template <
  /// Size of the Gemm problem - concept: gemm::GemmShape<>
  typename Shape_,
  /// Data type of A elements
  typename ElementA_,
  /// Layout of A matrix (concept: layout::MapFunc)
  typename LayoutA_,
  /// Layout of B matrix (concept: layout::MapFunc)
  typename LayoutB_,
  /// Element type of C matrix
  typename ElementC_,
  /// Layout of C matrix (concept: layout::MapFunc)
  typename LayoutC_
>
struct Mma<
  Shape_,
  ElementA_,
  LayoutA_,
  bool,
  LayoutB_,
  ElementC_,
  LayoutC_,
  arch::OpMultiplyAdd,
  bool> {

  /// Size of the Gemm problem - concept: gemm::GemmShape<>
  using Shape = Shape_;

  /// Data type of operand A
  using ElementA = ElementA_;

  /// Layout of A matrix (concept: layout::MapFunc)
  using LayoutA = LayoutA_;

  /// Data type of operand B
  using ElementB = bool;

  /// Layout of B matrix (concept: layout::MapFunc)
  using LayoutB = LayoutB_;

  /// Element type of operand C
  using ElementC = ElementC_;

  /// Layout of C matrix (concept: layout::MapFunc)
  using LayoutC = LayoutC_;

  /// Underlying mathematical operator
  using Operator = arch::OpMultiplyAdd;

  /// A operand storage
  using FragmentA = Array<ElementA, Shape::kMK>;

  /// B operand storage
  using FragmentB = Array<ElementB, Shape::kKN>;

  /// C operand storage
  using FragmentC = Array<ElementC, Shape::kMN>;

  /// Underlying matrix multiply operator (concept: arch::Mma)
  using ArchMmaOperator = typename MmaGeneric<
                                    Shape,
                                    ElementA,
                                    LayoutA,
                                    ElementB,
                                    LayoutB,
                                    ElementC,
                                    LayoutC,
                                    Operator>::MmaOp;
  //
  // Methods
  //

  /// Computes a matrix product D = A * B + C
  CUTLASS_HOST_DEVICE
  void operator()(
    FragmentC & D,
    FragmentA const & A,
    FragmentB const & B,
    FragmentC const & C) {

    MmaGeneric<
      Shape,
      ElementA,
      LayoutA,
      ElementB,
      LayoutB,
      ElementC,
      LayoutC,
      Operator> mma;
    mma(D, A, B, C);
  }
};

/////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace thread
} // namespace gemm
} // namespace cutlass
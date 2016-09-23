/* Copyright 2015 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#include "tensorflow/core/common_runtime/kernel_benchmark_testlib.h"
#include "tensorflow/core/framework/tensor.h"
#include "tensorflow/core/graph/node_builder.h"
#include "tensorflow/core/platform/test.h"
#include "tensorflow/core/platform/test_benchmark.h"

namespace tensorflow {

static Graph* ConstructSpaceToBatchGraph(
    const char* op_name, const TensorShape& input_shape, const int block_size,
    const std::vector<std::pair<int, int>>& paddings) {
  const int num_block_dims = 2;
  CHECK_EQ(num_block_dims, paddings.size());
  Graph* g = new Graph(OpRegistry::Global());
  Tensor input(DT_FLOAT, input_shape);
  input.flat<float>().setRandom();
  Tensor paddings_tensor(DT_INT32, TensorShape({num_block_dims, 2}));
  auto paddings_eigen_tensor = paddings_tensor.matrix<int32>();
  for (int block_dim = 0; block_dim < num_block_dims; ++block_dim) {
    paddings_eigen_tensor(block_dim, 0) = paddings[block_dim].first;
    paddings_eigen_tensor(block_dim, 1) = paddings[block_dim].second;
  }
  Node* ret;
  NodeBuilder(g->NewName("n"), op_name)
      .Input(test::graph::Constant(g, input))
      .Input(test::graph::Constant(g, paddings_tensor))
      .Attr("block_size", block_size)
      .Finalize(g, &ret);
  return g;
}

#define BM_SpaceToBatchDev(OP, DEVICE, B, H, W, D, BS, P00, P01, P10, P11)                    \
  static void                                                                                 \
      BM_##OP##_##DEVICE##_##B##_##H##_##W##_##D##_bs##BS##_pad##P00##_##P01##_##P10##_##P11( \
          int iters) {                                                                        \
    testing::ItemsProcessed(static_cast<int64>(iters) * B * (H + P00 + P01) *                 \
                            (W + P10 + P11) * D);                                             \
    test::Benchmark(#DEVICE,                                                                  \
                    ConstructSpaceToBatchGraph(#OP, TensorShape({B, H, W, D}),                \
                                               BS, {{P00, P01}, {P10, P11}}))                 \
        .Run(iters);                                                                          \
  }                                                                                           \
  BENCHMARK(                                                                                  \
      BM_##OP##_##DEVICE##_##B##_##H##_##W##_##D##_bs##BS##_pad##P00##_##P01##_##P10##_##P11);
#define BM_SpaceToBatch(OP, ...)            \
  BM_SpaceToBatchDev(OP, cpu, __VA_ARGS__); \
  BM_SpaceToBatchDev(OP, gpu, __VA_ARGS__);

BM_SpaceToBatch(SpaceToBatch, 64, 100, 100, 64, 2, 0, 0, 0, 0);
BM_SpaceToBatch(SpaceToBatch, 64, 100, 100, 1, 2, 0, 0, 0, 0);
BM_SpaceToBatch(SpaceToBatch, 64, 100, 100, 64, 2, 3, 3, 3, 3);

BM_SpaceToBatch(BatchToSpace, 256, 50, 50, 64, 2, 0, 0, 0, 0);
BM_SpaceToBatch(BatchToSpace, 256, 50, 50, 1, 2, 0, 0, 0, 0);
BM_SpaceToBatch(BatchToSpace, 256, 50, 50, 64, 2, 3, 3, 3, 3);

}  // namespace tensorflow

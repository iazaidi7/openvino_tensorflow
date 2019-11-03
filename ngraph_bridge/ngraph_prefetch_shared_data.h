/*******************************************************************************
 * Copyright 2019 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *******************************************************************************/

#ifndef NGRAPH_PREFETCH_SHARED_DATA_H_
#define NGRAPH_PREFETCH_SHARED_DATA_H_
#pragma once

#include <mutex>
#include <ostream>
#include <string>
#include <vector>

#include "tensorflow/core/framework/resource_mgr.h"

#include "ngraph/runtime/tensor.hpp"

#include "ngraph_bridge/thread_safe_queue.h"

namespace ng = ngraph;
namespace tf = tensorflow;

namespace tensorflow {

namespace ngraph_bridge {

class NGraphPrefetchSharedResouce : public ResourceBase {
 public:
  explicit NGraphPrefetchSharedResouce(const std::string& ng_enc_op_name,
                                       const std::string& backend_name,
                                       int cluster_id, int graph_id)
      : m_ng_enc_op_name(ng_enc_op_name),
        m_backend_name(backend_name),
        m_graph_id(graph_id),
        m_cluster_id(cluster_id) {}

  // Returns a debug string for *this.
  string DebugString() const override { return "NGraphPrefetchSharedResouce"; }

  // Returns memory used by this resource.
  int64 MemoryUsed() const override { return 0; }
  std::string GetName() const { return m_ng_enc_op_name; }
  std::string GetBackendName() const { return m_backend_name; }
  int GetGraphId() const { return m_graph_id; }
  int GetClusterId() const { return m_cluster_id; }

  static constexpr const char* RESOURCE_NAME = "NG_PREFETCH_DATA";
  static constexpr const char* CONTAINER_NAME = "NG_PREFETCH_DATA_CONTAINER";
  static constexpr const char* NGRAPH_TF_USE_PREFETCH =
      "NGRAPH_TF_USE_PREFETCH";

  struct IoTensorBundle {
    int Id;
    std::vector<shared_ptr<ng::runtime::Tensor>> Inputs;
    std::vector<shared_ptr<ng::runtime::Tensor>> Outputs;
  };

  // Adds the given nGraph tensor pair to write to
  // This is called by the NGraphEncapOp
  void AddNextIoTensorsForDeviceTransfer(IoTensorBundle next) {
    m_tf_2_ng.Add(std::move(next));
  }

  // Returns the IO tensors to be used top copy TF tensors to NG device
  // This will be called by the prefetcher
  IoTensorBundle GetNextIoTensorsForDeviceTransfer() {
    return std::move(m_tf_2_ng.GetNextAvailable());
  }

  // Adds the given nGraph tensor pair to write to
  // This is called by the prefetcher to add Tensors that are copied
  // from TF tensor and are now ready for the next iteration
  void AddNextIoTensorsReadyForDeviceExecution(IoTensorBundle next) {
    m_ng_2_tf.Add(std::move(next));
  }

  // Returns the IO tensors to be ready to be executed by NG device
  // This will be called by the NGEncOp
  IoTensorBundle GetNextIoTensorsReadyForDeviceExecution() {
    return std::move(m_ng_2_tf.GetNextAvailable());
  }

  void SetBufferDepth(int depth) {
    m_prefetch_buffer_depth = depth;
  }
  int GetBufferDepth() {
    return m_prefetch_buffer_depth;
  }

  void IncrSkipCount() {
    m_skip_count++;
  }
  int GetSkipCount() {
    return m_skip_count;
  }

 private:
  const std::string m_ng_enc_op_name;
  const std::string m_backend_name;
  const int m_graph_id;
  const int m_cluster_id;

  // We need to maintain two queues as follows:
  // ----------+------------+------------+------------------------------------+
  // Queue     | Writer     | Reader     | Comments                           |
  // ----------+------------+------------+------------------------------------+
  // m_tf_2_ng | Prefetcher | NgEncOp    | TF tensors copied to the nG tensor |
  // ----------+------------+------------+------------------------------------+
  // m_ng_2_tf | NgEncOp    | Prefetcher | NGEnc enqueus empty nGTensors here |
  // ----------+------------+------------+------------------------------------+
  //
  // The interaction is as follows:
  // Iteration  Action
  // 1          NGEncOp pushes the IO tensors to m_ng_2_tf queue
  // 2
  //            Prefetcher pulls IO tensors out of m_ng_2_tf queue and copies TF
  //            data
  //            Prefetcher pushes this item to the m_tf_2_ng queue
  //            NGEncOp pushes the IO tensors to m_ng_2_tf queue
  //            NGEncOp pulls IO tensors from m_tf_2_ng (from previous
  //            iteration) and executes
  // 3          Repeat

  ThreadSafeQueue<IoTensorBundle> m_tf_2_ng;
  ThreadSafeQueue<IoTensorBundle> m_ng_2_tf;

  int m_prefetch_buffer_depth{0};
  int m_skip_count{0};
};

}  // namespace ngraph_bridge

}  // namespace tensorflow
#endif  // NGRAPH_PREFETCH_SHARED_DATA_H_

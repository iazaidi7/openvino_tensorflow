/*******************************************************************************
 * Copyright (C) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *******************************************************************************/

#include "gtest/gtest.h"

#include "tensorflow/cc/client/client_session.h"
#include "tensorflow/cc/ops/standard_ops.h"
#include "tensorflow/core/framework/tensor.h"
#include "tensorflow/core/public/session.h"

#include "logging/tf_graph_writer.h"
#include "openvino_tensorflow/assign_clusters.h"
#include "openvino_tensorflow/backend_manager.h"
#include "openvino_tensorflow/mark_for_clustering.h"
#include "test/test_utilities.h"

using namespace std;
namespace ng = ngraph;

namespace tensorflow {
namespace openvino_tensorflow {
namespace testing {

#define ASSERT_NOT_OK(x) ASSERT_NE((x), ::tensorflow::Status::OK());

/*
These tests test the Backend Handling by the bridge.
*/

// Test SetBackendAPI
TEST(BackendManager, SetBackend) {
  auto env_map = StoreEnv({"OPENVINO_TF_BACKEND"});

  ASSERT_OK(BackendManager::SetBackend("CPU"));
  string backend;
  ASSERT_OK(BackendManager::GetBackendName(backend));
  ASSERT_EQ(backend, "CPU");
  ASSERT_NOT_OK(BackendManager::SetBackend("temp"));

  // Clean Up
  // If OPENVINO_TF_BACKEND was set, set it back
  RestoreEnv(env_map);
}

// Test GetBackend API
TEST(BackendManager, GetBackendName) {
  auto env_map = StoreEnv({"OPENVINO_TF_BACKEND"});

  ASSERT_OK(BackendManager::SetBackend("CPU"));
  string backend;
  ASSERT_OK(BackendManager::GetBackendName(backend));
  ASSERT_EQ(backend, "CPU");

  // expected ERROR
  SetBackendUsingEnvVar("DUMMY");
  ASSERT_OK(BackendManager::GetBackendName(backend));
  ASSERT_EQ(backend, "CPU");

  // set env variable to ""
  SetBackendUsingEnvVar("");
  ASSERT_OK(BackendManager::GetBackendName(backend));
  ASSERT_EQ(backend, "CPU");

  // set backend to dummy and env variable to CPU
  // expected CPU
  ASSERT_NOT_OK(BackendManager::SetBackend("DUMMY"));
  SetBackendUsingEnvVar("CPU");
  ASSERT_OK(BackendManager::GetBackendName(backend));
  ASSERT_EQ(backend, "CPU");

  // unset env variable
  // expected interpreter
  UnsetBackendUsingEnvVar();
  ASSERT_OK(BackendManager::GetBackendName(backend));
  ASSERT_EQ(backend, "CPU");

  // Clean up
  UnsetBackendUsingEnvVar();
  ASSERT_OK(BackendManager::SetBackend("CPU"));
  // restore
  // If OPENVINO_TF_BACKEND was set, set it back
  RestoreEnv(env_map);
}

}  // namespace testing
}  // namespace openvino_tensorflow
}  // namespace tensorflow

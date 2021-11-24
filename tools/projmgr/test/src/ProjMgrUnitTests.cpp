/*
 * Copyright (c) 2020-2021 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ProjMgr.h"
#include "ProjMgrTestEnv.h"

#include "gtest/gtest.h"

using namespace std;

class PackGenUnitTests : public ProjMgr, public ::testing::Test {
public:
  PackGenUnitTests() {}
  virtual ~PackGenUnitTests() {}
};

TEST_F(PackGenUnitTests, RunProjMgr) {
  char *argv[1];

  // Empty options
  EXPECT_EQ(0, RunProjMgr(1, argv));
}

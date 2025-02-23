// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_THIRD_PARTY_QUIC_PLATFORM_API_QUIC_TEST_RANDOM_H_
#define NET_THIRD_PARTY_QUIC_PLATFORM_API_QUIC_TEST_RANDOM_H_

#include <stdint.h>
#include <limits>

#include "net/third_party/quic/platform/api/quic_string.h"
#include "net/third_party/quic/platform/impl/quic_test_random_impl.h"

namespace quic {
namespace test {

using QuicTestRandomBase = QuicTestRandomBaseImpl;

// QuicTestRandomImpl must inherit from QuicTestRandomBaseImpl;
using QuicTestRandom = QuicTestRandomImpl;

}  // namespace test
}  // namespace quic

#endif  // NET_THIRD_PARTY_QUIC_PLATFORM_API_QUIC_TEST_RANDOM_H_

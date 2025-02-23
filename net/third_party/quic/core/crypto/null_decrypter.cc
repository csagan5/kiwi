// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/third_party/quic/core/crypto/null_decrypter.h"

#include <cstdint>

#include "net/third_party/quic/core/quic_data_reader.h"
#include "net/third_party/quic/core/quic_utils.h"
#include "net/third_party/quic/platform/api/quic_bug_tracker.h"
#include "net/third_party/quic/platform/api/quic_uint128.h"

using std::string;

namespace quic {

NullDecrypter::NullDecrypter(Perspective perspective)
    : perspective_(perspective) {}

bool NullDecrypter::SetKey(QuicStringPiece key) {
  return key.empty();
}

bool NullDecrypter::SetNoncePrefix(QuicStringPiece nonce_prefix) {
  return nonce_prefix.empty();
}

bool NullDecrypter::SetIV(QuicStringPiece iv) {
  return iv.empty();
}

bool NullDecrypter::SetPreliminaryKey(QuicStringPiece key) {
  QUIC_BUG << "Should not be called";
  return false;
}

bool NullDecrypter::SetDiversificationNonce(const DiversificationNonce& nonce) {
  QUIC_BUG << "Should not be called";
  return true;
}

bool NullDecrypter::DecryptPacket(QuicTransportVersion version,
                                  QuicPacketNumber /*packet_number*/,
                                  QuicStringPiece associated_data,
                                  QuicStringPiece ciphertext,
                                  char* output,
                                  size_t* output_length,
                                  size_t max_output_length) {
  QuicDataReader reader(ciphertext.data(), ciphertext.length(),
                        HOST_BYTE_ORDER);
  QuicUint128 hash;

  if (!ReadHash(&reader, &hash)) {
    return false;
  }

  QuicStringPiece plaintext = reader.ReadRemainingPayload();
  if (plaintext.length() > max_output_length) {
    QUIC_BUG << "Output buffer must be larger than the plaintext.";
    return false;
  }
  if (hash != ComputeHash(version, associated_data, plaintext)) {
    return false;
  }
  // Copy the plaintext to output.
  memcpy(output, plaintext.data(), plaintext.length());
  *output_length = plaintext.length();
  return true;
}

size_t NullDecrypter::GetKeySize() const {
  return 0;
}

size_t NullDecrypter::GetIVSize() const {
  return 0;
}

QuicStringPiece NullDecrypter::GetKey() const {
  return QuicStringPiece();
}

QuicStringPiece NullDecrypter::GetNoncePrefix() const {
  return QuicStringPiece();
}

uint32_t NullDecrypter::cipher_id() const {
  return 0;
}

bool NullDecrypter::ReadHash(QuicDataReader* reader, QuicUint128* hash) {
  uint64_t lo;
  uint32_t hi;
  if (!reader->ReadUInt64(&lo) || !reader->ReadUInt32(&hi)) {
    return false;
  }
  *hash = MakeQuicUint128(hi, lo);
  return true;
}

QuicUint128 NullDecrypter::ComputeHash(QuicTransportVersion version,
                                       const QuicStringPiece data1,
                                       const QuicStringPiece data2) const {
  QuicUint128 correct_hash;
  if (version > QUIC_VERSION_35) {
    if (perspective_ == Perspective::IS_CLIENT) {
      // Peer is a server.
      correct_hash = QuicUtils::FNV1a_128_Hash_Three(data1, data2, "Server");

    } else {
      // Peer is a client.
      correct_hash = QuicUtils::FNV1a_128_Hash_Three(data1, data2, "Client");
    }
  } else {
    correct_hash = QuicUtils::FNV1a_128_Hash_Two(data1, data2);
  }
  QuicUint128 mask = MakeQuicUint128(UINT64_C(0x0), UINT64_C(0xffffffff));
  mask <<= 96;
  correct_hash &= ~mask;
  return correct_hash;
}

}  // namespace quic

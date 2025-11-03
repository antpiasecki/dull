#pragma once
#include "common.h"
#include <botan/aead.h>
#include <botan/auto_rng.h>
#include <botan/hex.h>
#include <botan/pwdhash.h>
#include <vector>

namespace Crypto {

static Botan::AutoSeeded_RNG g_rng; // NOLINT

const Botan::secure_vector<u8> KEY = {
    0xaa, 0x1b, 0x6c, 0xd2, 0x8e, 0x54, 0x9d, 0xdb, 0xe8, 0xd6, 0x9e,
    0xe9, 0xa4, 0x19, 0xa4, 0xc1, 0x2,  0x73, 0x58, 0x82, 0xa3, 0x75,
    0x5f, 0x86, 0xbd, 0x0,  0x92, 0x97, 0x1e, 0xf3, 0x27, 0x5d};

inline Botan::secure_vector<u8>
encrypt_xchacha20_poly1305(const Botan::secure_vector<u8> &plaintext,
                           const Botan::secure_vector<u8> &key,
                           const std::vector<u8> &nonce) {
  ASSERT(key.size() == 32);
  ASSERT(nonce.size() == 24);

  // XChaCha20 is selected automatically based on the nonce size
  // https://github.com/randombit/botan/blob/master/src/lib/stream/chacha/chacha.cpp#L375
  auto cipher = Botan::AEAD_Mode::create_or_throw(
      "ChaCha20Poly1305", Botan::Cipher_Dir::Encryption);

  cipher->set_key(key);
  cipher->start(nonce);

  Botan::secure_vector<u8> ciphertext = plaintext;
  cipher->finish(ciphertext);

  return ciphertext;
}

inline Botan::secure_vector<u8>
decrypt_xchacha20_poly1305(const Botan::secure_vector<u8> &ciphertext,
                           const Botan::secure_vector<u8> &key,
                           const std::vector<u8> &nonce) {
  ASSERT(ciphertext.size() >= 16);
  ASSERT(key.size() == 32);
  ASSERT(nonce.size() == 24);

  auto cipher = Botan::AEAD_Mode::create_or_throw(
      "ChaCha20Poly1305", Botan::Cipher_Dir::Decryption);

  cipher->set_key(key);
  cipher->start(nonce);

  Botan::secure_vector<u8> plaintext = ciphertext;
  cipher->finish(plaintext);

  return plaintext;
}

inline Botan::secure_vector<u8>
derive_key_argon2id(const std::string &password, const std::vector<u8> &salt) {
  auto pwdhash = Botan::PasswordHashFamily::create_or_throw("Argon2id")
                     ->from_params(static_cast<u64>(1024 * 1024), 6, 4);

  Botan::secure_vector<u8> key(32);
  pwdhash->derive_key(key.data(), key.size(), password.data(), password.size(),
                      salt.data(), salt.size());
  return key;
}

}; // namespace Crypto
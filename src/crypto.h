#pragma once
#include "common.h"
#include <botan/aead.h>
#include <botan/auto_rng.h>
#include <botan/hex.h>
#include <botan/pwdhash.h>
#include <vector>

namespace Crypto {

// TODO: random salt per per-vault
const std::vector<u8> SALT = {0xab, 0xfb, 0x45, 0x1,  0x62, 0xcf, 0xd7, 0x46,
                              0xdf, 0xaf, 0x4e, 0xa9, 0xf0, 0x4b, 0x9f, 0x38};

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
                     ->from_params(static_cast<u64>(1024 * 1024), 8, 4);

  Botan::secure_vector<u8> key(32);
  pwdhash->derive_key(key.data(), key.size(), password.data(), password.size(),
                      salt.data(), salt.size());
  return key;
}

}; // namespace Crypto
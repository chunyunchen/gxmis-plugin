#include <eosiolib/crypto.h>      
#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>
#include <eosio/time.hpp>
#include <eosiolib/types.h>

using namespace eosio;
using namespace std;

size_t from_hex(const string& hex_str, char* out_data, size_t out_data_len);
string to_hex(const char* d, uint32_t s);
string sha256_to_hex(const capi_checksum256& sha256);
string sha1_to_hex(const capi_checksum160& sha1);
uint64_t uint64_hash(const string& hash);
uint64_t uint64_hash(const capi_checksum256& hash);
capi_checksum256 hex_to_sha256(const string& hex_str);
capi_checksum160 hex_to_sha1(const string& hex_str);
bool DecodeBase58(const char* psz, std::vector<unsigned char>& vch);
bool decode_base58(const string& str, vector<unsigned char>& vch);
capi_signature str_to_sig(const string& sig, const bool& checksumming = true);
capi_public_key str_to_pub(const string& pubkey, const bool& checksumming = true);


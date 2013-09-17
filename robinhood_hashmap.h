#ifndef HASHMAP_ROBINHOOD
#define HASHMAP_ROBINHOOD

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
#include <inttypes.h>
#include <string.h>
#include <stdio.h>

#include <string>
#include <iostream>

#include "murmurhash3.h"
#include "hamming.h"
#include "hashmap.h"

#include "monitoring.h"

namespace hashmap
{



class RobinHoodHashMap: public HashMap
{
public:

  RobinHoodHashMap(uint64_t size) {
    buckets_ = NULL;
    num_buckets_ = size;
    probing_max_ = size;
    DELETED_BUCKET = (Entry*)1;
  }

  virtual ~RobinHoodHashMap() {
    Close();
  }

  int Open();
  int Close();

  struct Entry
  {
    uint32_t size_key;
    uint32_t size_value;
    char *data;
  };

  struct Bucket
  {
    uint64_t hash;
    struct Entry* entry;
  };

  int Get(const std::string& key, std::string* value);
  int Put(const std::string& key, const std::string& value);
  int Exists(const std::string& key);
  int Remove(const std::string& key);
  int Resize();
  int Dump();
  int CheckDensity();
  int BucketCounts();
  int GetBucketState(int index);
  int FillInitIndex(uint64_t index_stored, uint64_t *index_init);
  int FillDistanceToInitIndex(uint64_t index_stored, uint64_t *distance);
  void GetMetadata(std::map< std::string, std::string >& metadata);
  uint64_t GetMinPSL();
  uint64_t GetMaxPSL();

private:
  Bucket* buckets_;
  uint64_t num_buckets_;

  uint64_t hash_function(const std::string& key) {
    static char hash[16];
    static uint64_t output;
    MurmurHash3_x64_128(key.c_str(), key.size(), 0, hash);
    memcpy(&output, hash, 8); 
    return output;
  }

  Entry* DELETED_BUCKET;
  uint64_t probing_max_;

  void UpdatePSL(uint64_t psl, int32_t increment);
  std::map<uint64_t, uint64_t> psl_;




};


}; // end namespace hashmap

#endif // HASHMAP_PROBING

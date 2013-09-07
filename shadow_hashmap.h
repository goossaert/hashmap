#ifndef HASHMAP_SHADOW
#define HASHMAP_SHADOW

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



class ShadowHashMap: public HashMap
{
public:

  ShadowHashMap(uint64_t size,
                uint64_t size_probing,
                uint64_t size_neighborhood_start,
                uint64_t size_neighborhood_end
               ) {
    buckets_ = NULL;
    num_buckets_ = size;
    size_neighborhood_ = size_neighborhood_start;
    size_neighborhood_max_ = size_neighborhood_end;
    size_probing_ = size_probing;
    monitoring_ = new hashmap::Monitoring(num_buckets_, size_neighborhood_max_);
  }

  virtual ~ShadowHashMap() {
  }

  int Open();

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


private:
  Bucket* buckets_;
  uint64_t num_buckets_;

  uint64_t FindEmptyBucket(uint64_t index_init);

  uint64_t hash_function(const std::string& key) {
    static char hash[16];
    static uint64_t output;
    MurmurHash3_x64_128(key.c_str(), key.size(), 0, hash);
    memcpy(&output, hash, 8); 
    return output;
  }

  uint32_t size_neighborhood_;
  uint32_t size_neighborhood_max_;
  uint32_t size_probing_;

};


}; // end namespace hashmap

#endif // HASHMAP_SHADOW

#ifndef HASHMAP_PROBING
#define HASHMAP_PROBING

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



class ProbingHashMap: public HashMap
{
public:

  ProbingHashMap(uint64_t size,
                 int      probing_max) {
    buckets_ = NULL;
    num_buckets_ = size;
    HASH_DELETED_BUCKET = 1;
    DELETED_BUCKET = (Entry*)1;
    probing_max_ = probing_max;
  }

  virtual ~ProbingHashMap() {
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
  void GetMetadata(std::map< std::string, std::string >& metadata);

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


  uint32_t probing_max_;
  uint64_t HASH_DELETED_BUCKET;
  Entry* DELETED_BUCKET;

};


}; // end namespace hashmap

#endif // HASHMAP_PROBING

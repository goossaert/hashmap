#ifndef HASHMAP_BACKSHIFT
#define HASHMAP_BACKSHIFT

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
#include <inttypes.h>
#include <string.h>
#include <stdio.h>

#include <string>
#include <iostream>
#include <limits>

#include "murmurhash3.h"
#include "hamming.h"
#include "hashmap.h"

#include "monitoring.h"

namespace hashmap
{



class BackshiftHashMap: public HashMap
{
public:

  BackshiftHashMap(uint64_t size) {
    buckets_ = NULL;
    num_buckets_ = size;
    probing_max_ = size;
  }

  virtual ~BackshiftHashMap() {
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
  uint64_t GetMinInitDistance();
  uint64_t GetMaxInitDistance();

private:
  Bucket* buckets_;
  uint64_t num_buckets_;
  uint64_t num_buckets_used_;

  uint64_t hash_function(const std::string& key) {
    static char hash[16];
    static uint64_t output;
    MurmurHash3_x64_128(key.c_str(), key.size(), 0, hash);
    memcpy(&output, hash, 8); 
    return output;
  }

  uint64_t probing_max_;

  void UpdateInitDistance(uint64_t distance, int32_t increment);
  void UpdateMinMaxInitDistance();
  std::map<uint64_t, uint64_t> distances_;
  uint64_t init_distance_min_;
  uint64_t init_distance_max_;



};


}; // end namespace hashmap

#endif // HASHMAP_BACKSHIFT

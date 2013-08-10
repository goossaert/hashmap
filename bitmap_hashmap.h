#ifndef HOPSCOTCHHASH_BITMAP
#define HOPSCOTCHHASH_BITMAP

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
#include <inttypes.h>

#include <string>
#include <iostream>

#include "murmurhash3.h"
#include "hamming.h"
#include "hashmap.h"

namespace hashmap
{



class BitmapHashMap: public HashMap
{
public:

  BitmapHashMap(uint64_t size,
                uint64_t size_probing
               ) {
    buckets_ = NULL;
    num_buckets_ = size;
    size_neighborhood_ = 32;
    size_probing_ = size_probing;
  }

  virtual ~BitmapHashMap() {
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
    uint32_t bitmap;
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

private:
  Bucket* buckets_;
  uint64_t num_buckets_;
  uint32_t size_neighborhood_;
  uint32_t size_probing_;

  uint64_t FindEmptyBucket(uint64_t index_init);

  uint64_t hash_function(const std::string& key) {
    static char hash[16];
    static uint64_t output;
    MurmurHash3_x64_128(key.c_str(), key.size(), 0, hash);
    memcpy(&output, hash, 8); 
    //std::cout << output << std::endl;
    return output;
  }
 
};


}; // end namespace hashmap

#endif // HOPSCOTCHHASH_BITMAP

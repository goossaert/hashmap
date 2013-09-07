#ifndef HASHMAP
#define HASHMAP

#include <string>
#include <iostream>
#include <map>

#include "murmurhash3.h"
#include "hamming.h"


namespace hashmap
{

class Monitoring;

class HashMap
{
public:

  HashMap() {
    monitoring_ = NULL;
  }

  virtual ~HashMap() {
  }

  virtual int Open() = 0;
  virtual int Get(const std::string& key, std::string* value) = 0;
  virtual int Put(const std::string& key, const std::string& value) = 0;
  virtual int Exists(const std::string& key) = 0;
  virtual int Remove(const std::string& key) = 0;
  virtual int Dump() = 0;
  virtual int CheckDensity() = 0;
  virtual int BucketCounts() = 0;
  virtual int GetBucketState(int index) = 0;
  virtual int FillInitIndex(uint64_t index_stored, uint64_t *index_init) = 0;
  virtual void GetMetadata(std::map< std::string, std::string >& metadata) = 0;

  Monitoring *monitoring_;
};

}; // end namespace hashmap

#endif // HASHMAP

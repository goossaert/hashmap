#ifndef HASHMAP
#define HASHMAP

#include <string>
#include <iostream>

#include "murmurhash3.h"
#include "hamming.h"

namespace hashmap
{

class HashMap
{
public:

  HashMap() {
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
};

}; // end namespace hashmap

#endif // HASHMAP

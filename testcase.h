#ifndef HASHMAP_TESTCASE
#define HASHMAP_TESTCASE

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
#include <inttypes.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include <string>
//#include <iostream>
//#include <map>
#include <set>
#include <algorithm>
#include <sys/stat.h>


#include "hashmap.h"
#include "monitoring.h"


namespace hashmap
{

class TestCase {
 public:
  TestCase() {}
  virtual ~TestCase() {}
  virtual void run() = 0;
  void InsertEntries(uint32_t nb_items, std::set<std::string>& keys);
  void RemoveEntries(uint32_t nb_items, std::set<std::string>& keys);
  HashMap *hm_;
};


class BatchTestCase: public TestCase {

 public:
  BatchTestCase(HashMap *hm, uint64_t num_buckets, double load_factor_max, double load_factor_remove) {
    hm_ = hm;
    num_buckets_ = num_buckets;
    load_factor_max_ = load_factor_max;
    load_factor_remove_ = load_factor_remove;
  }
  virtual void run();

 private:
  uint64_t num_buckets_;
  double load_factor_max_;
  double load_factor_remove_;
};



class RippleTestCase: public TestCase {

 public:
  RippleTestCase(HashMap *hm, uint64_t num_buckets, double load_factor_max, double load_factor_remove) {
    hm_ = hm;
    num_buckets_ = num_buckets;
    load_factor_max_ = load_factor_max;
    load_factor_remove_ = load_factor_remove;
  }
  virtual void run();

 private:
  uint64_t num_buckets_;
  double load_factor_max_;
  double load_factor_remove_;
};


class LoadingTestCase: public TestCase {

 public:
  LoadingTestCase(HashMap *hm, uint64_t num_buckets) {
    hm_ = hm;
    num_buckets_ = num_buckets;
  }
  virtual void run();

 private:
  uint64_t num_buckets_;


};

}; // namespace

#endif

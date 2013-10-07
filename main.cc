#include <iostream>
#include <string>
#include <sstream>
#include <stdlib.h>
#include <set>
#include <algorithm>
#include <sys/stat.h>

#include "hashmap.h"
#include "probing_hashmap.h"
#include "robinhood_hashmap.h"
#include "bitmap_hashmap.h"
#include "shadow_hashmap.h"

#include "testcase.h"



std::string concatenate(std::string const& str, int i)
{
    std::stringstream s;
    s << str << i;
    return s.str();
}

 
uint32_t NearestPowerOfTwo(const uint32_t number)	{
  uint32_t power = 1;
  while (power < number) {
    power <<= 1;
  }
  return power;
}


int exists_or_mkdir(const char *path) {
  struct stat sb;

  if (stat(path, &sb) == 0) {
    if (!S_ISDIR(sb.st_mode)) {
      return 1;
    }
  } else if (mkdir(path, 0777) != 0) {
    return 1;
  }

  return 0;
}


void show_usage() {
  fprintf(stdout, "Test program for implementations of open addressing hash table algorithms.\n");
  fprintf(stdout, "\n");

  fprintf(stdout, "General parameters (mandatory):\n");
  fprintf(stdout, " --algo            algorithm to use for the hash table. Possible values are:\n");
  fprintf(stdout, "                     * linear: linear probing\n");
  fprintf(stdout, "                     * robinhood: robin hood hashing\n");
  fprintf(stdout, "                     * bitmap: hopscotch hashing with bitmap representation\n");
  fprintf(stdout, "                     * shadow: hopscotch hashing with shadow representation\n");
  fprintf(stdout, " --testcase        test case to use. Possible values are:\n");
  fprintf(stdout, "                     * loading: load the table until it is full (does not perform any removals).\n");
  fprintf(stdout, "                     * batch: load the table, then remove a large batch, and re-insert a large batch.\n");
  fprintf(stdout, "                     * ripple: load the table, then do a series of remove-insetion operations.\n");
  fprintf(stdout, "\n");

  fprintf(stdout, "Parameters for linear probing algorithm (optional):\n");
  fprintf(stdout, " --num_buckets     number of buckets in the hash table (default=10000)\n");
  fprintf(stdout, "\n");

  fprintf(stdout, "Parameters for Robin Hood algorithm (optional):\n");
  fprintf(stdout, " --num_buckets     number of buckets in the hash table (default=10000)\n");
  fprintf(stdout, "\n");


  fprintf(stdout, "Parameters for bitmap algorithm (optional):\n");
  fprintf(stdout, " --num_buckets     number of buckets in the hash table (default=10000)\n");
  fprintf(stdout, " --size_probing    maximum number of buckets used in the probing (default=4096)\n");
  fprintf(stdout, "\n");

  fprintf(stdout, "Parameters for shadow algorithm (optional):\n");
  fprintf(stdout, " --num_buckets     number of buckets in the hash table (default=10000)\n");
  fprintf(stdout, " --size_probing    maximum number of buckets used in the probing (default=4096)\n");
  fprintf(stdout, " --size_nh_start   starting size of the neighborhoods (default=32)\n");
  fprintf(stdout, " --size_nh_end     ending size of the neighborhoods (default=32)\n");
  fprintf(stdout, "\n");

  fprintf(stdout, "Parameters for the batch test case (optional):\n");
  fprintf(stdout, " --load_factor_max   maxium load factor at which the table should be used (default=.7)\n");
  fprintf(stdout, " --load_factor_step  load factor by which items in the table should be removed and inserted (default=.1)\n");
  fprintf(stdout, "\n");

  fprintf(stdout, "Parameters for the ripple test case (optional):\n");
  fprintf(stdout, " --load_factor_max   maxium load factor at which the table should be used (default=.7)\n");
  fprintf(stdout, " --load_factor_step  load factor by which items in the table should be removed and inserted (default=.1)\n");
  fprintf(stdout, "\n");

  fprintf(stdout, "Examples:\n");
  fprintf(stdout, "./hashmap --algo bitmap --num_buckets 1000000\n");
  fprintf(stdout, "./hashmap --algo shadow --num_buckets 1000000 --size_nh_start 4 --size_nh_end 64\n");
}





int main(int argc, char **argv) {
  bool has_error;

  if (argc == 1 || (argc == 2 && strcmp(argv[1], "--help") == 0)) {
    show_usage(); 
    exit(-1);
  }

  if (argc % 2 == 0) {
    std::cerr << "Error: invalid number of arguments" << std::endl; 
    exit(-1);
  }

  uint32_t size_neighborhood_start = 32;
  uint32_t size_neighborhood_end = 32;
  uint32_t size_probing = 4096;
  uint32_t num_buckets = 10000;
  double load_factor_max = 0.7;
  double load_factor_step = 0.1;
  std::string algorithm = "";
  std::string testcase = "";

  if (argc > 2) {
    for (int i = 1; i < argc; i += 2 ) {
      if (strcmp(argv[i], "--algo" ) == 0) {
        algorithm = std::string(argv[i+1]);
      } else if (strcmp(argv[i], "--num_buckets" ) == 0) {
        num_buckets = atoi(argv[i+1]);
      } else if (strcmp(argv[i], "--size_nh_start" ) == 0) {
        size_neighborhood_start = atoi(argv[i+1]);
      } else if (strcmp(argv[i], "--size_nh_end" ) == 0) {
        size_neighborhood_end = atoi(argv[i+1]);
      } else if (strcmp(argv[i], "--size_probing" ) == 0) {
        size_probing = atoi(argv[i+1]);
      } else if (strcmp(argv[i], "--testcase" ) == 0) {
        testcase = std::string(argv[i+1]);
      } else if (strcmp(argv[i], "--load_factor_max" ) == 0) {
        load_factor_max = atof(argv[i+1]);
      } else if (strcmp(argv[i], "--load_factor_step" ) == 0) {
        load_factor_step = atof(argv[i+1]);
      } else {
        fprintf(stderr, "Unknown parameter [%s]\n", argv[i]);
        exit(-1); 
      }
    }
  }

  int num_items = num_buckets;
  //int num_items = NearestPowerOfTwo(num_buckets);
  hashmap::HashMap *hm;
  if (algorithm == "bitmap") {
    hm = new hashmap::BitmapHashMap(num_items, size_probing);
  } else if (algorithm == "shadow") {
    hm = new hashmap::ShadowHashMap(num_items, size_probing, size_neighborhood_start, size_neighborhood_end);
  } else if (algorithm == "linear") {
    hm = new hashmap::ProbingHashMap(num_items, size_probing);
  } else if (algorithm == "robinhood") {
    hm = new hashmap::RobinHoodHashMap(num_items);
  } else {
    fprintf(stderr, "Unknown algorithm [%s]\n", algorithm.c_str());
    exit(-1); 
  }

  if (testcase == "loading") {
    //run_testcase2(hm, num_items, load_factor_max);
    hashmap::LoadingTestCase tc(hm, num_items);
    tc.run();
    return 0;
  } else if (testcase == "batch") {
    //run_testcase2(hm, num_items, load_factor_max);
    hashmap::BatchTestCase tc(hm, num_items, load_factor_max, load_factor_step);
    tc.run();
    return 0;
  } else if (testcase == "ripple") {
    hashmap::RippleTestCase tc(hm, num_items, load_factor_max, load_factor_step);
    tc.run();
    return 0;
  } else if (testcase != "") {
    fprintf(stderr, "Error: testcase is unknown [%s]\n", testcase.c_str());
    return 1;
  }

  hm->Open();
  std::string value_out("value_out");



  int num_items_reached = 0;

  for (int i = 0; i < num_items; i++) {
    value_out = "value_out";
    std::string key = concatenate( "key", i );
    std::string value = concatenate( "value", i );
    int ret_put = hm->Put(key, value);
    hm->Get(key, &value_out);

    if (ret_put != 0) {
      std::cout << "Insertion stopped due to clustering at step: " << i << std::endl; 
      std::cout << "Load factor: " << (double)i/num_items << std::endl; 
      num_items_reached = i;
      break;
    }
  }


  has_error = false;
  for (int i = 0; i < num_items_reached; i++) {
    value_out = "value_out";
    std::string key = concatenate( "key", i );
    std::string value = concatenate( "value", i );
    int ret_get = hm->Get(key, &value_out);
    if (ret_get != 0 || value != value_out) {
      std::cout << "Final check: error at step [" << i << "]" << std::endl; 
      has_error = true;
      break;
    }
  }

  if (!has_error) {
      std::cout << "Final check: OK" << std::endl; 
  }


  /*
  if (hm->monitoring_ != NULL) {
      std::cout << "Monitoring: OK" << std::endl; 
  }

  // testcase-algo-metric-runnumber-step.json
  // batch50-shadow-density-00001-0001.json

  hm->monitoring_->PrintDensity("density.json");
  std::cout << "Clustering" << std::endl; 
  hm->monitoring_->PrintClustering(hm);

  hm->monitoring_->PrintProbingSequenceLengthSearch("probing_sequence_length_search.json");
  hm->monitoring_->PrintNumScannedBlocks("num_scanned_blocks.json");

  */
  //hm->CheckDensity();
  //hm->BucketCounts();
  

  has_error = false;
  for (int i = 0; i < num_items_reached; i++) {
    std::string key = concatenate( "key", i );
    std::string value = concatenate( "value", i );
    int ret_remove = hm->Remove(key);
    if (ret_remove != 0) {
      std::cout << "Remove: error at step [" << i << "]" << std::endl; 
      has_error = true;
      break;
    }
    int ret_get = hm->Get(key, &value_out);
    if (ret_get == 0) {
      std::cout << "Remove: error at step [" << i << "] -- can get after remove" << std::endl; 
      has_error = true;
      break;
    }
  }

  if (!has_error) {
      std::cout << "Removing items: OK" << std::endl; 
  }


  return 0;
}

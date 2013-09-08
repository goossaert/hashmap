#include <iostream>
#include <string>
#include <sstream>
#include <stdlib.h>
#include <set>
#include <algorithm>
#include <sys/stat.h>

#include "hashmap.h"
#include "bitmap_hashmap.h"
#include "shadow_hashmap.h"
#include "probing_hashmap.h"



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
  fprintf(stdout, "                     * bitmap: hopscotch hashing with bitmap representation\n");
  fprintf(stdout, "                     * shadow: hopscotch hashing with shadow representation\n");
  fprintf(stdout, "                     * probing: hopscotch hashing with shadow representation\n");
  fprintf(stdout, "\n");

  fprintf(stdout, "Parameters for bitmap algorithm (optional):\n");
  fprintf(stdout, " --num_buckets     number of buckets in the hash table (default=10000)\n");
  fprintf(stdout, " --size_probing    maximum number of buckets used in the linear probing (default=4096)\n");
  fprintf(stdout, "\n");

  fprintf(stdout, "Parameters for shadow algorithm (optional):\n");
  fprintf(stdout, " --num_buckets     number of buckets in the hash table (default=10000)\n");
  fprintf(stdout, " --size_probing    maximum number of buckets used in the linear probing (default=4096)\n");
  fprintf(stdout, " --size_nh_start   starting size of the neighborhoods (default=32)\n");
  fprintf(stdout, " --size_nh_end     ending size of the neighborhoods (default=32)\n");
  fprintf(stdout, "\n");

  fprintf(stdout, "Examples:\n");
  fprintf(stdout, "./hashmap --algo bitmap --num_buckets 1000000\n");
  fprintf(stdout, "./hashmap --algo shadow --num_buckets 1000000 --size_nh_start 4 --size_nh_end 64\n");
}



void run_testcase(hashmap::HashMap *hm, uint64_t num_buckets, double load_factor) {
  std::set<std::string> keys;
  std::string key;
  int key_size = 16;
  char buffer[key_size + 1];
  buffer[key_size] = '\0';
  char filename[256];
  char alpha[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  uint32_t num_items;
  uint32_t num_items_big = (uint32_t)((double)num_buckets * load_factor);
  uint32_t num_items_small = (uint32_t)((double)num_buckets * 0.1);
  fprintf(stdout, "num_items %llu %llu\n", num_items, num_items_small);

  std::string testcase = "batch";
  if (exists_or_mkdir(testcase.c_str()) != 0) {
    fprintf(stderr, "Could not create directory [%s]\n", testcase.c_str());
    exit(1);
  }

  std::set<std::string>::iterator it_find;
  for (int i = 0; i < 1; i++) {
    num_items = num_items_big;
    srand(i);
    keys.clear();
    for (int cycle = 0; cycle < 100; cycle++) {
      for (uint32_t j = 0; j < num_items; j++) {
        bool is_valid = false;
        while (!is_valid) {
          for (int k = 0; k < key_size; k++) {
            buffer[k] = alpha[rand() % 62];
          }
          key = buffer;
          it_find = keys.find(key);
          if (it_find == keys.end()) {
            is_valid = true;
          } else {
            //fprintf(stdout, "%s\n", key.c_str());
            //fprintf(stdout, "%d\n", keys.size());
          }
        }
        keys.insert(key);
        int ret_put = hm->Put(key, key);
        //fprintf(stderr, "Put() [%s]\n", key.c_str());
        if (ret_put != 0) {
          fprintf(stderr, "Put() error\n");
        }
      }
      printf("keys insert %d\n", keys.size());

      hm->monitoring_->SetInstance(i);
      hm->monitoring_->SetCycle(cycle);

      std::map<std::string, std::string> metadata;
      hm->GetMetadata(metadata);
      sprintf(filename, "%s/%s-%s-density-%05d-%04d.json", testcase.c_str(), testcase.c_str(), metadata["name"].c_str(), i, cycle);
      hm->monitoring_->PrintDensity(filename);

      //sprintf(filename, "%s/%s-%s-num_scanned_blocks-%05d-%04d.json", testcase.c_str(), testcase.c_str(), metadata["name"].c_str(), i, cycle);
      //hm->monitoring_->PrintNumScannedBlocks(filename);

      sprintf(filename, "%s/%s-%s-probing_sequence_length_search-%05d-%04d.json", testcase.c_str(), testcase.c_str(), metadata["name"].c_str(), i, cycle);
      hm->monitoring_->PrintProbingSequenceLengthSearch(filename);
      
      for (int index_del = 0; index_del < num_items_small; index_del++) {
        uint64_t r = rand();
        uint64_t offset = r % keys.size();
        //printf("delete index %d -- offset %llu -- rand %llu\n", index_del, offset, r);
        std::set<std::string>::iterator it(keys.begin());
        std::advance(it, offset);
        //fprintf(stdout, "str: %s\n", (*it).c_str());
        //key = buffer;
        int ret_remove = hm->Remove(*it);
        //fprintf(stderr, "Remove() [%s]\n", it->c_str());
        if (ret_remove != 0) fprintf(stderr, "Error while removing\n");
        keys.erase(it);
      }
      printf("keys erase %d\n", keys.size());
      num_items = num_items_small;
    }
  }

  // testcase-algo-metric-runnumber-step.json
  // batch50-shadow-density-00001-0001.json
  //hm->monitoring_->PrintDensity("density.json");
  //hm->monitoring_->PrintNumScannedBlocks("num_scanned_blocks.json");
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
  std::string algorithm = "";

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
  } else if (algorithm == "probing") {
    hm = new hashmap::ProbingHashMap(num_items, 500);
  } else {
    fprintf(stderr, "Algorithm unknown [%s]\n", algorithm.c_str());
    exit(-1); 
  }

  hm->Open();
  std::string value_out("value_out");

  run_testcase(hm, num_items, 0.8);
  return 0;


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


  if (hm->monitoring_ != NULL) {
      std::cout << "Monitoring: OK" << std::endl; 
  }

  // testcase-algo-metric-runnumber-step.json
  // batch50-shadow-density-00001-0001.json

  hm->monitoring_->PrintDensity("density.json");
  /*
  std::cout << "Clustering" << std::endl; 
  hm->monitoring_->PrintClustering(hm);
  */

  hm->monitoring_->PrintProbingSequenceLengthSearch("probing_sequence_length_search.json");
  hm->monitoring_->PrintNumScannedBlocks("num_scanned_blocks.json");

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

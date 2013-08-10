#include <iostream>
#include "hashmap.h"
#include "bitmap_hashmap.h"
#include "shadow_hashmap.h"

#include <string>
#include <sstream>


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


void show_usage() {
  fprintf(stdout, "Test program for implementations of open addressing hash table algorithms.\n");
  fprintf(stdout, "\n");

  fprintf(stdout, "General parameters (mandatory):\n");
  fprintf(stdout, " --algo            algorithm to use for the hash table. Possible values are:\n");
  fprintf(stdout, "                     * bitmap: hopscotch hashing with bitmap representation\n");
  fprintf(stdout, "                     * shadow: hopscotch hashing with shadow representation\n");
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


int main(int argc, char **argv) {
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
  hashmap::HashMap *bhm;
  if (algorithm == "bitmap") {
    bhm = new hashmap::BitmapHashMap(num_items, size_probing);
  } else if (algorithm == "shadow") {
    bhm = new hashmap::ShadowHashMap(num_items, size_probing, size_neighborhood_start, size_neighborhood_end);
  } else {
    fprintf(stderr, "Algorithm unknown [%s]\n", algorithm.c_str());
    exit(-1); 
  }

  bhm->Open();
  std::string value_out("value_out");

  int num_items_reached = 0;

  for (int i = 0; i < num_items; i++) {
    value_out = "value_out";
    std::string key = concatenate( "key", i );
    std::string value = concatenate( "value", i );
    int ret_put = bhm->Put(key, value);
    bhm->Get(key, &value_out);

    if (ret_put != 0) {
      std::cout << "Insertion stopped due to clustering at step: " << i << std::endl; 
      std::cout << "Load factor: " << (double)i/num_items << std::endl; 
      num_items_reached = i;
      break;
    }
  }


  bool has_error = false;
  for (int i = 0; i < num_items_reached; i++) {
    value_out = "value_out";
    std::string key = concatenate( "key", i );
    std::string value = concatenate( "value", i );
    int ret_get = bhm->Get(key, &value_out);
    if (ret_get != 0 || value != value_out) {
      std::cout << "Final check: error at step [" << i << "]" << std::endl; 
      has_error = true;
      break;
    }
  }

  if (!has_error) {
      std::cout << "Final check: OK" << std::endl; 
  }

  //bhm->CheckDensity();
  //bhm->BucketCounts();

  return 0;
}

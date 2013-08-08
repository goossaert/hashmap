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

 
uint32_t NearestPowerOfTwo(const uint32_t i)	{
  uint32_t rc = 1;
  while (rc < i) {
    rc <<= 1;
  }
  return rc;
}



int main(int argc, char **argv) {
  std::cout << "Hi there!" << std::endl;

  if (argc < 2) {
    std::cerr << "Error: invalid number of arguments" << std::endl; 
  }

  uint32_t size_neighborhood_start = 32;
  uint32_t size_neighborhood_end = 32;
  uint32_t size_probing = 4096;
  uint32_t num_buckets = 10;
  std::string algorithm = "bitmap";

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

  int num_items = NearestPowerOfTwo(num_buckets);
  int num_items_test = num_items;
  hhash::HashMap *bhm;
  if (algorithm == "bitmap") {
    bhm = new hhash::BitmapHashMap(num_items, size_probing);
  } else if (algorithm == "shadow") {
    bhm = new hhash::ShadowHashMap(num_items, size_probing, size_neighborhood_start, size_neighborhood_end);
  } else {
    fprintf(stderr, "Algorithm unknown [%s]\n", algorithm.c_str());
    exit(-1); 
  }

  bhm->Open();
  std::string value_out("value_out");

  //std::string key("key0002");
  //std::string value("Hello man, how are you!");

  //bhm->Put(key, value);
  //bhm->Get(key, &value_out);


  for( int i = 0; i < num_items_test; i++ ) {
    if( i == 9364 ) {
      value_out = "value_out";
    }
    value_out = "value_out";
    std::string key = concatenate( "key", i );
    std::string value = concatenate( "value", i );
    int ret_put = bhm->Put(key, value);
    int ret_get = bhm->Get(key, &value_out);
    //std::cout << ret_put << " | " << ret_get << std::endl;
    //std::cout << "Out: " << key << " | " << value_out << std::endl;
    if( i == 20 || i == 100 || i == 130 ) {
      //std::cout << "DUMP - " << i << std::endl;
      //bhm->Dump();
    }

    if (ret_put != 0) {
      std::cout << "error in put at step: " << i << std::endl; 
      std::cout << "load factor: " << (double)i/num_items << std::endl; 
      break;
    }

    if (ret_get != 0) {
      std::cout << "error in get at step: " << i << std::endl; 
      break;
    }

    //if (i % step == 0) {
    //  std::cout << "key: " << i << " - out: " << value_out << std::endl;
    //  std::cout << "status:" << s.ToString() << std::endl;
    //}
    
    
    //if (i % 10000 == 0) {
    //  std::cout << "step: " << i << std::endl; 
    //}

    if (i % 2000000 == 0) { // || (i > num_items_test - 700 && i % 1 == 0)) {
      std::cout << "check: " << i << std::endl; 
      for (int j = 0; j <= i; j++) {
        value_out = "value_out";
        std::string key = concatenate( "key", j );
        int ret_get = bhm->Get(key, &value_out);
        std::string value_out_cmp = concatenate( "value", j );
        if (ret_get != 0 || value_out_cmp != value_out) {
          std::cout << "Error at id [" << i << "] with key [" << j << "]" << std::endl; 
          exit(-1);
        }
        //std::cout << "Out: " << key << " | " << value_out << std::endl;
      }
    }

  }


  std::cout << "Final Check: " << std::endl;
  for( int i = 0; i < num_items_test; i++ ) {
    value_out = "value_out";
    std::string key = concatenate( "key", i );
    int ret_get = bhm->Get(key, &value_out);
    if (ret_get != 0) {
      std::cout << "Error at id [" << i << "]" << std::endl; 
      break;
    }
    //std::cout << "Out: " << key << " | " << value_out << std::endl;
  }

  //bhm->CheckDensity();
  //bhm->BucketCounts();

  return 0;
}

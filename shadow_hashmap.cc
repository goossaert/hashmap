#include "shadow_hashmap.h"

namespace hhash {

int ShadowHashMap::CheckDensity() {
}


int ShadowHashMap::BucketCounts() {

  std::cout << "current neighborhood: " << size_neighborhood_ << std::endl;

}


int ShadowHashMap::Dump() {
}



int ShadowHashMap::Open() {
  std::cout << "open enter" << std::endl;
  std::cout << "allocate memory" << std::endl;
  buckets_ = new Bucket[num_buckets_ + size_neighborhood_];
  std::cout << "allocate ok" << std::endl;
  memset(buckets_, 0, sizeof(Bucket) * (num_buckets_ + size_neighborhood_));

  return 0;
}


int ShadowHashMap::Get(const std::string& key, std::string* value) {
  uint64_t hash = hash_function(key);
  uint64_t index_init = hash % num_buckets_;
  bool found = false;
  for (uint32_t i = 0; i < size_neighborhood_; i++) {
    uint64_t index_current = (index_init + i) % num_buckets_;
    if (   buckets_[index_current].entry != NULL
        && key.size() == buckets_[index_current].entry->size_key
        && memcmp(buckets_[index_current].entry->data, key.c_str(), key.size()) == 0) {
      *value = std::string(buckets_[index_current].entry->data + key.size(),
                           buckets_[index_current].entry->size_value);
      found = true;
      break;
    }
  }

  if (found) return 0;

  return 1;
}


uint64_t ShadowHashMap::FindEmptyBucket(uint64_t index_init) {
  // In this function, the modulos function is being applied on indexes at the last moment,
  // when they are being used or returned. This allows to handle cases where the
  // indexes are cycling back to the beginning of the bucket array.
  bool found = false;
  uint64_t index_current = index_init;
  for (uint32_t i = 0; i < size_probing_; i++) {
    index_current = index_init + i;
    if (buckets_[index_current % num_buckets_].entry == NULL) {
      found = true;
      break;
    }
  }

  if (!found) {
    std::cerr << "Put(): cannot resize" << std::endl; 
    return num_buckets_;
  }

  uint64_t index_empty = index_current;
  while (index_empty - index_init >= size_neighborhood_) {
    uint64_t index_base_min = index_empty - (size_neighborhood_ - 1);
    bool found_swap = false;
    for (uint32_t i = size_neighborhood_ - 1; i > 0; i--) {
      uint64_t index_candidate = index_empty - i;
      if (index_candidate < index_init) continue;
      if (buckets_[index_candidate % num_buckets_].hash % num_buckets_ >= index_base_min) {
        // the candidate has its base bucket within the right scope, so we swap!
        buckets_[index_empty % num_buckets_].entry = buckets_[index_candidate % num_buckets_].entry;
        buckets_[index_empty % num_buckets_].hash = buckets_[index_candidate % num_buckets_].hash;

        buckets_[index_candidate % num_buckets_].entry = NULL;
        buckets_[index_candidate % num_buckets_].hash = 0;

        index_empty = index_candidate;
        found_swap = true;
        break;
      }
    }

    if (!found_swap) {
      if (size_neighborhood_ < size_neighborhood_max_) {
        size_neighborhood_ *= 2;
        std::cerr << "Increasing neighborhood, now " << size_neighborhood_ << std::endl;
      } else {
        if (false) {
          fprintf(stderr, "index [%llu] empty [%llu]\n", index_init, index_empty);
          uint32_t index_temp = index_empty - size_neighborhood_ + 1;
          if (index_temp > index_init) index_temp = index_init;
          index_temp -= 20;
          if (index_temp < 0) index_temp = 0;
          for (index_temp; index_temp <= index_empty + 20; index_temp++) {
            if (index_temp == index_empty - size_neighborhood_ + 1) {
              fprintf(stderr, "neigh ");
            } else if (index_temp == index_init) {
              fprintf(stderr, "index ");
            } else if (index_temp == index_empty) {
              fprintf(stderr, "empty ");
            } else {
              fprintf(stderr, "      ");
            }

            fprintf(stderr, " %7lu ", index_temp);

            if (buckets_[index_temp % num_buckets_].entry == NULL) {
              fprintf(stderr, "    EMP");
            } else {
              fprintf(stderr, "%7" PRIu64 " ", buckets_[index_temp % num_buckets_].hash % num_buckets_);
            }
            fprintf(stderr, "\n");
          }
          fprintf(stderr, "\n");
        }
        std::cerr << "Put(): cannot resize" << std::endl;
        return num_buckets_;
      }
    }
  }

  return index_empty % num_buckets_;
}

int ShadowHashMap::Put(const std::string& key, const std::string& value) {

  uint64_t hash = hash_function(key);
  uint64_t index_init = hash % num_buckets_;
  uint64_t index_empty = FindEmptyBucket(index_init);

  if (index_empty == num_buckets_) {
    return 1; 
  }

  char *data = new char[key.size() + value.size()];
  memcpy(data, key.c_str(), key.size());
  memcpy(data + key.size(), value.c_str(), value.size());

  ShadowHashMap::Entry *entry = new ShadowHashMap::Entry;
  entry->size_key = key.size();
  entry->size_value = value.size();
  entry->data = data;
  buckets_[index_empty].entry = entry;
  buckets_[index_empty].hash = hash;

  return 0;
}

int ShadowHashMap::Exists(const std::string& key) {
  return 0;
}

int ShadowHashMap::Remove(const std::string& key) {
  return 0;
}


};

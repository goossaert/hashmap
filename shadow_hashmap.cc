#include "shadow_hashmap.h"

namespace hashmap {


int ShadowHashMap::Open() {
  buckets_ = new Bucket[num_buckets_];
  memset(buckets_, 0, sizeof(Bucket) * (num_buckets_));
  monitoring_ = new hashmap::Monitoring(num_buckets_, size_neighborhood_max_, static_cast<HashMap*>(this));
  return 0;
}



int ShadowHashMap::Close() {
  if (buckets_ != NULL) {
    for (uint32_t i = 0; i < num_buckets_; i++) {
      if (buckets_[i].entry != NULL) {
        delete[] buckets_[i].entry->data;
        delete buckets_[i].entry;
      }
    }
    delete[] buckets_;
  }

  if (monitoring_ != NULL) {
    delete monitoring_;
  }
  return 0;
}




int ShadowHashMap::Get(const std::string& key, std::string* value) {
  uint64_t hash = hash_function(key);
  uint64_t index_init = hash % num_buckets_;
  bool found = false;
  uint32_t i;
  for (i = 0; i < size_neighborhood_; i++) {
    uint64_t index_current = (index_init + i) % num_buckets_;
    if (   buckets_[index_current].entry != NULL
        && buckets_[index_current].hash  == hash
        && key.size() == buckets_[index_current].entry->size_key
        && memcmp(buckets_[index_current].entry->data, key.c_str(), key.size()) == 0) {
      *value = std::string(buckets_[index_current].entry->data + key.size(),
                           buckets_[index_current].entry->size_value);
      found = true;
      break;
    }
  }

  if (found) return 0;

  monitoring_->AddDMB(size_neighborhood_);
  monitoring_->AddAlignedDMB(index_init, (index_init + i) % num_buckets_);
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
      monitoring_->AddDistanceToFreeBucket(i);
      monitoring_->AddAlignedDistanceToFreeBucket(index_init, index_current);
      break;
    }
  }

  if (!found) {
    return num_buckets_;
  }

  int num_swaps = 0;

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

        uint64_t psl = monitoring_->GetProbingSequenceLengthSearch(index_candidate % num_buckets_);
        monitoring_->RemoveProbingSequenceLengthSearch(index_candidate % num_buckets_);
        monitoring_->SetProbingSequenceLengthSearch(index_empty % num_buckets_, psl);

        index_empty = index_candidate;
        found_swap = true;
        num_swaps += 1;
        break;
      }
    }

    if (!found_swap) {
      if (size_neighborhood_ < size_neighborhood_max_) {
        size_neighborhood_ *= 2;
        //std::cerr << "Increasing neighborhood, now " << size_neighborhood_ << std::endl;
      } else {
        // For debugging only, dump of the area around the neighborhood
        if (false) {
          //fprintf(stderr, "index [%" PRIu64 "] empty [%" PRIu64 "]\n", index_init, index_empty);
          uint32_t index_temp = index_empty - size_neighborhood_ + 1;
          if (index_temp > index_init) index_temp = index_init;
          index_temp -= 20;
          if (index_temp < 0) index_temp = 0;
          for (; index_temp <= index_empty + 20; index_temp++) {
            if (index_temp == index_empty - size_neighborhood_ + 1) {
              fprintf(stderr, "neigh ");
            } else if (index_temp == index_init) {
              fprintf(stderr, "index ");
            } else if (index_temp == index_empty) {
              fprintf(stderr, "empty ");
            } else {
              fprintf(stderr, "      ");
            }

            fprintf(stderr, " %7du ", index_temp);

            if (buckets_[index_temp % num_buckets_].entry == NULL) {
              fprintf(stderr, "    EMP");
            } else {
              fprintf(stderr, "%7" PRIu64 " ", buckets_[index_temp % num_buckets_].hash % num_buckets_);
            }
            fprintf(stderr, "\n");
          }
          fprintf(stderr, "\n");
        }
        return num_buckets_;
      }
    }
  }

  monitoring_->SetProbingSequenceLengthSearch(index_empty % num_buckets_,
                                              index_empty - index_init);
  monitoring_->AddNumberOfSwaps(num_swaps);

  return index_empty % num_buckets_;
}

int ShadowHashMap::Put(const std::string& key, const std::string& value) {
  uint64_t hash = hash_function(key);
  uint64_t index_init = hash % num_buckets_;
  uint64_t index_empty = FindEmptyBucket(index_init);
  // TODO: Put() should use Exists() and perform a replacement if needed.
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
  // TODO: implement
  return 0;
}

int ShadowHashMap::Remove(const std::string& key) {
  uint64_t hash = hash_function(key);
  uint64_t index_init = hash % num_buckets_;
  bool found = false;
  uint64_t index_current;
  for (uint32_t i = 0; i < size_neighborhood_; i++) {
    index_current = (index_init + i) % num_buckets_;
    if (   buckets_[index_current].entry != NULL
        && buckets_[index_current].hash  == hash
        && key.size() == buckets_[index_current].entry->size_key
        && memcmp(buckets_[index_current].entry->data, key.c_str(), key.size()) == 0) {
      found = true;
      break;
    }
  }

  if (found) {
    delete[] buckets_[index_current].entry->data;
    delete buckets_[index_current].entry;
    buckets_[index_current].entry = NULL;
    monitoring_->RemoveProbingSequenceLengthSearch(index_current);
    return 0;
  }

  return 0;
}

int ShadowHashMap::Resize() {
  // TODO: implement
  return 0;
}


// For debugging
int ShadowHashMap::CheckDensity() {
  return 0;
}


int ShadowHashMap::BucketCounts() {
  std::cout << "current neighborhood: " << size_neighborhood_ << std::endl;
  return 0;
}


int ShadowHashMap::Dump() {
  return 0;
}


int ShadowHashMap::GetBucketState(int index) {
  if (buckets_[index].entry == NULL) {
    return 0;
  }

  return 1;

}

int ShadowHashMap::FillInitIndex(uint64_t index_stored, uint64_t *index_init) {
  if(buckets_[index_stored].entry == NULL) return -1;
  *index_init = buckets_[index_stored].hash % num_buckets_;
  return 0;
}


void ShadowHashMap::GetMetadata(std::map< std::string, std::string >& metadata) {
  metadata["name"] = "shadow";
  char buffer[1024]; 
  sprintf(buffer, "{\"num_buckets\": %" PRIu64 ", \"size_probing\": %u, \"size_neighborhood_start\": %u, \"size_neighborhood_end\": %u}", num_buckets_, size_probing_, size_neighborhood_start_, size_neighborhood_max_);
  metadata["parameters_hashmap"] = buffer;
  sprintf(buffer, "nb%" PRIu64 "-sp%u-sns%u-sne%u", num_buckets_, size_probing_, size_neighborhood_start_, size_neighborhood_max_);
  metadata["parameters_hashmap_string"] = buffer;
}



};

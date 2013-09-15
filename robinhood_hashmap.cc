#include "robinhood_hashmap.h"

namespace hashmap {



int RobinHoodHashMap::Open() {
  buckets_ = new Bucket[num_buckets_];
  memset(buckets_, 0, sizeof(Bucket) * (num_buckets_));
  monitoring_ = new hashmap::Monitoring(num_buckets_, num_buckets_, static_cast<HashMap*>(this));
  return 0;
}

int RobinHoodHashMap::Close() {
  if (buckets_ != NULL) {
    for (uint32_t i = 0; i < num_buckets_; i++) {
      if (buckets_[i].entry != NULL && buckets_[i].entry != DELETED_BUCKET) {
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



int RobinHoodHashMap::Get(const std::string& key, std::string* value) {
  uint64_t hash = hash_function(key);
  uint64_t index_init = hash % num_buckets_;
  uint64_t probe_distance = 0;
  bool found = false;
  for (uint32_t i = 0; i < probing_max_; i++) {
    uint64_t index_current = (index_init + i) % num_buckets_;
    FillDistanceToInitIndex(index_current, &probe_distance);
    if (   buckets_[index_current].entry == NULL
        || i > probe_distance) {
      break;
    }

    if (buckets_[index_current].entry == DELETED_BUCKET) {
      continue;
    }

    if (   key.size() == buckets_[index_current].entry->size_key
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




int RobinHoodHashMap::Put(const std::string& key, const std::string& value) {
  uint64_t hash = hash_function(key);
  uint64_t index_init = hash % num_buckets_;
  // TODO: add test if the number of entries in the table is equal to
  // num_buckets_

  char *data = new char[key.size() + value.size()];
  memcpy(data, key.c_str(), key.size());
  memcpy(data + key.size(), value.c_str(), value.size());

  RobinHoodHashMap::Entry *entry = new RobinHoodHashMap::Entry;
  entry->size_key = key.size();
  entry->size_value = value.size();
  entry->data = data;

  uint64_t index_current = index_init;
  uint64_t probe_distance = 0;
  uint64_t probe_current = 0;
  RobinHoodHashMap::Entry *entry_temp = NULL;
  uint64_t hash_temp = NULL;

  for (uint64_t i = 0; i < probing_max_; i++) {
    index_current = (index_init + i) % num_buckets_;
    if (buckets_[index_current].entry == NULL) {
      monitoring_->SetProbingSequenceLengthSearch(index_current, probe_current);
      UpdatePSL(probe_current, 1);
      buckets_[index_current].entry = entry;
      buckets_[index_current].hash = hash;
      break;
    } else {
      FillDistanceToInitIndex(index_current, &probe_distance);
      if (probe_distance <= i) {
        // Swapping the current bucket with the one to insert
        entry_temp = buckets_[index_current].entry;
        hash_temp = buckets_[index_current].hash;
        buckets_[index_current].entry = entry;
        buckets_[index_current].hash = hash;
        entry = entry_temp;
        hash = hash_temp;
        monitoring_->SetProbingSequenceLengthSearch(index_current, probe_current);
        UpdatePSL(probe_distance, -1);
        UpdatePSL(probe_current, 1);
        probe_current = probe_distance;
        if (entry == DELETED_BUCKET) {
          // The bucket we just swapped was a deleted bucket,
          // so the insertion process can stop here
          break;
        }
      }
    }
    probe_current++;
  }

  monitoring_->UpdateNumItemsInBucket(index_init, 1);

  return 0;
}


int RobinHoodHashMap::Exists(const std::string& key) {
  // TODO: implement
  return 0;
}


int RobinHoodHashMap::Remove(const std::string& key) {
  uint64_t hash = hash_function(key);
  uint64_t index_init = hash % num_buckets_;
  uint64_t probe_distance = 0;
  bool found = false;
  uint64_t index_current;
  for (uint32_t i = 0; i < probing_max_; i++) {
    index_current = (index_init + i) % num_buckets_;

    FillDistanceToInitIndex(index_current, &probe_distance);
    if (   buckets_[index_current].entry == NULL
        || i > probe_distance) {
      break;
    }

    if (buckets_[index_current].entry == DELETED_BUCKET) {
      continue;
    }

    if (   key.size() == buckets_[index_current].entry->size_key
        && memcmp(buckets_[index_current].entry->data, key.c_str(), key.size()) == 0) {
      found = true;
      break;
    }
  }

  if (found) {
    delete[] buckets_[index_current].entry->data;
    delete buckets_[index_current].entry;
    buckets_[index_current].entry = DELETED_BUCKET;
    monitoring_->UpdateNumItemsInBucket(index_init, -1);
    monitoring_->RemoveProbingSequenceLengthSearch(index_current);

    FillDistanceToInitIndex(index_current, &probe_distance);
    UpdatePSL(probe_distance, -1);
    return 0;
  }

  return 1;
}



int RobinHoodHashMap::Resize() {
  // TODO: implement
  return 0;
}


// For debugging
int RobinHoodHashMap::CheckDensity() {
  return 0;
}

int RobinHoodHashMap::BucketCounts() {
  return 0;
}

int RobinHoodHashMap::Dump() {
  return 0;
}


int RobinHoodHashMap::GetBucketState(int index) {
  //printf("GetBucketState %d\n", index);
  if (buckets_[index].entry == NULL) {
    return 0;
  }

  return 1;
}

int RobinHoodHashMap::FillInitIndex(uint64_t index_stored, uint64_t *index_init) {
  if(buckets_[index_stored].entry == NULL) return -1;
  *index_init = buckets_[index_stored].hash % num_buckets_;
  return 0;
}

int RobinHoodHashMap::FillDistanceToInitIndex(uint64_t index_stored, uint64_t *distance) {
  if(buckets_[index_stored].entry == NULL) return -1;
  uint64_t index_init = buckets_[index_stored].hash % num_buckets_;
  if (index_init <= index_stored) {
    *distance = index_stored - index_init; 
  } else {
    *distance = index_stored + (num_buckets_ - index_init); 
  }
  return 0;
}


void RobinHoodHashMap::GetMetadata(std::map< std::string, std::string >& metadata) {
  metadata["name"] = "robinhood";
}




// Computation of the probe
uint64_t RobinHoodHashMap::GetMinPSL() {
  std::map<uint64_t, uint64_t>::iterator it;
  uint64_t psl_min = std::numeric_limits<uint64_t>::max();
  for (it = psl_.begin(); it != psl_.end(); ++it) {
    fprintf(stderr, "GetMinPSL() %llu %llu\n", it->first, it->second);
    if (it->first < psl_min) {
      psl_min = it->first;
    }
  }
  return psl_min;
}


void RobinHoodHashMap::UpdatePSL(uint64_t psl, int32_t increment) {
  std::map<uint64_t, uint64_t>::iterator it;
  it = psl_.find(psl);
  if (it == psl_.end()) {
    if (increment > 0) {
      psl_[psl] = increment;
    }
  } else {
    psl_[psl] += increment;
    if (psl_[psl] <= 0) {
      psl_.erase(it); 
    }
  }
}




}; // end namespace hashmap

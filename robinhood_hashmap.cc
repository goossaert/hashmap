#include "robinhood_hashmap.h"

namespace hashmap {



int RobinHoodHashMap::Open() {
  buckets_ = new Bucket[num_buckets_];
  memset(buckets_, 0, sizeof(Bucket) * (num_buckets_));
  monitoring_ = new hashmap::Monitoring(num_buckets_, num_buckets_, static_cast<HashMap*>(this));
  num_buckets_used_ = 0;
  init_distance_min_ = 0;
  init_distance_max_ = 0;
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

  distances_.clear();

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
  if (num_buckets_used_ + 1 == num_buckets_) {
    return 1;
  }
  num_buckets_used_ += 1;

  uint64_t hash = hash_function(key);
  uint64_t index_init = hash % num_buckets_;

  char *data = new char[key.size() + value.size()];
  memcpy(data, key.c_str(), key.size());
  memcpy(data + key.size(), value.c_str(), value.size());

  RobinHoodHashMap::Entry *entry = new RobinHoodHashMap::Entry;
  entry->size_key = key.size();
  entry->size_value = value.size();
  entry->data = data;

  uint64_t index_current = index_init;
  uint64_t probe_distance = 0;
  uint64_t probe_current = GetMinInitDistance();
  RobinHoodHashMap::Entry *entry_temp = NULL;
  uint64_t hash_temp = NULL;
  uint64_t i;

  for (i = probe_current; i < probing_max_; i++) {
    index_current = (index_init + i) % num_buckets_;
    if (buckets_[index_current].entry == NULL) {
      monitoring_->SetProbingSequenceLengthSearch(index_current, probe_current);
      UpdateInitDistance(probe_current, 1);
      buckets_[index_current].entry = entry;
      buckets_[index_current].hash = hash;
      break;
    } else {
      FillDistanceToInitIndex(index_current, &probe_distance);
      if (probe_current > probe_distance) {
        // Swapping the current bucket with the one to insert
        entry_temp = buckets_[index_current].entry;
        hash_temp = buckets_[index_current].hash;
        buckets_[index_current].entry = entry;
        buckets_[index_current].hash = hash;
        entry = entry_temp;
        hash = hash_temp;
        monitoring_->SetProbingSequenceLengthSearch(index_current, probe_current);
        UpdateInitDistance(probe_current, 1);
        if (entry != DELETED_BUCKET) {
          UpdateInitDistance(probe_distance, -1);
          probe_current = probe_distance;
        } else {
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
  uint64_t distance_max = GetMaxInitDistance();

  //for (uint64_t i = 0; i < num_buckets_; i++) {
  for (uint64_t i = GetMinInitDistance(); i <= distance_max; i++) {
    index_current = (index_init + i) % num_buckets_;

    if (buckets_[index_current].entry == DELETED_BUCKET) {
      continue;
    }

    FillDistanceToInitIndex(index_current, &probe_distance);
    if (   buckets_[index_current].entry == NULL) {
       // || i > probe_distance) {
      //fprintf(stderr, "Remove() found NULL\n");
      continue;
    }

    if (   key.size() == buckets_[index_current].entry->size_key
        && memcmp(buckets_[index_current].entry->data, key.c_str(), key.size()) == 0) {
      found = true;
      uint64_t mind = GetMinInitDistance();
      if (i < mind) {
        fprintf(stderr, "Found at distance %llu and min at %llu\n", i, GetMinInitDistance());
      }
      break;
    }
  }

  if (found) {
    FillDistanceToInitIndex(index_current, &probe_distance);
    UpdateInitDistance(probe_distance, -1);

    delete[] buckets_[index_current].entry->data;
    delete buckets_[index_current].entry;
    buckets_[index_current].entry = DELETED_BUCKET;
    monitoring_->UpdateNumItemsInBucket(index_init, -1);
    monitoring_->RemoveProbingSequenceLengthSearch(index_current);
    num_buckets_used_ -= 1;

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
  metadata["name"] = "robinhood_inprogress";
}

uint64_t RobinHoodHashMap::GetMinInitDistance() {
  return init_distance_min_;
}

uint64_t RobinHoodHashMap::GetMaxInitDistance() {
  return init_distance_max_;
}



void RobinHoodHashMap::UpdateMinMaxInitDistance() {
  if (distances_.size() == 0) return;

  std::map<uint64_t, uint64_t>::iterator it;
  //fprintf(stderr, "GetMinInitDistance() ----------------------\n");

  uint64_t distance_min = std::numeric_limits<uint64_t>::max();
  for (it = distances_.begin(); it != distances_.end(); ++it) {
    //fprintf(stderr, "GetMinInitDistance() %llu %llu\n", it->first, it->second);
    if (it->first < distance_min) {
      distance_min = it->first;
    }
  }
  init_distance_min_ = distance_min;

  uint64_t distance_max = 0;
  for (it = distances_.begin(); it != distances_.end(); ++it) {
    if (it->first > distance_max) {
      distance_max = it->first;
    }
  }
  //fprintf(stderr, "GetMaxInitDistance() %llu\n", distances_max);
  init_distance_max_ = distance_max;

}


void RobinHoodHashMap::UpdateInitDistance(uint64_t distance, int32_t increment) {
  std::map<uint64_t, uint64_t>::iterator it;
  it = distances_.find(distance);
  if (it == distances_.end()) {
    if (increment > 0) {
      distances_[distance] = increment;
      UpdateMinMaxInitDistance();
    } else {
      fprintf(stderr, "UpdateInitDistance() neg on not exist %llu %d\n", distance, increment);
    }
  } else {
    distances_[distance] += increment;
    if (distances_[distance] <= 0) {
      distances_.erase(it); 
      UpdateMinMaxInitDistance();
    }
  }
}




}; // end namespace hashmap

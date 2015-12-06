#include "backshift_hashmap.h"

namespace hashmap {

int BackshiftHashMap::Open() {
  buckets_ = new Bucket[num_buckets_];
  memset(buckets_, 0, sizeof(Bucket) * (num_buckets_));
  monitoring_ = new hashmap::Monitoring(num_buckets_, num_buckets_, static_cast<HashMap*>(this));
  num_buckets_used_ = 0;
  return 0;
}

int BackshiftHashMap::Close() {
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



int BackshiftHashMap::Get(const std::string& key, std::string* value) {
  uint64_t hash = hash_function(key);
  uint64_t index_init = hash % num_buckets_;
  uint64_t probe_distance = 0;
  bool found = false;
  uint32_t i;
  for (i = 0; i < probing_max_; i++) {
    uint64_t index_current = (index_init + i) % num_buckets_;
    FillDistanceToInitIndex(index_current, &probe_distance);
    if (   buckets_[index_current].entry == NULL
        || i > probe_distance) {
      break;
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

  monitoring_->AddDMB(i);
  monitoring_->AddAlignedDMB(index_init, (index_init + i) % num_buckets_);
  return 1;
}




int BackshiftHashMap::Put(const std::string& key, const std::string& value) {
  if (num_buckets_used_ == num_buckets_) {
    return 1;
  }
  num_buckets_used_ += 1;

  uint64_t hash = hash_function(key);
  uint64_t index_init = hash % num_buckets_;

  char *data = new char[key.size() + value.size()];
  memcpy(data, key.c_str(), key.size());
  memcpy(data + key.size(), value.c_str(), value.size());

  BackshiftHashMap::Entry *entry = new BackshiftHashMap::Entry;
  entry->size_key = key.size();
  entry->size_value = value.size();
  entry->data = data;

  uint64_t index_current = index_init;
  uint64_t probe_distance = 0;
  uint64_t probe_current = 0;
  BackshiftHashMap::Entry *entry_temp = NULL;
  uint64_t hash_temp = 0;
  uint64_t i;
  int num_swaps = 0;

  for (i = 0; i < probing_max_; i++) {
    index_current = (index_init + i) % num_buckets_;
    if (buckets_[index_current].entry == NULL) {
      monitoring_->SetDIB(index_current, probe_current);
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
        monitoring_->SetDIB(index_current, probe_current);
        probe_current = probe_distance;
        num_swaps += 1;
      }
    }
    probe_current++;
  }

  monitoring_->AddDFB(i);
  monitoring_->AddAlignedDFB(index_init, index_current);
  monitoring_->AddNumberOfSwaps(num_swaps);

  return 0;
}


int BackshiftHashMap::Exists(const std::string& key) {
  // TODO: implement
  return 0;
}


int BackshiftHashMap::Remove(const std::string& key) {
  uint64_t hash = hash_function(key);
  uint64_t index_init = hash % num_buckets_;
  bool found = false;
  uint64_t index_current = 0;
  uint64_t probe_distance = 0;

  for (uint64_t i = 0; i < num_buckets_; i++) {
    index_current = (index_init + i) % num_buckets_;
    FillDistanceToInitIndex(index_current, &probe_distance);
    if (   buckets_[index_current].entry == NULL
        || i > probe_distance) {
      break;
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
    monitoring_->RemoveDIB(index_current);
    uint64_t i = 1;
    uint64_t index_previous = 0, index_swap = 0;
    for (i = 1; i < num_buckets_; i++) {
      index_previous = (index_current + i - 1) % num_buckets_;
      index_swap = (index_current + i) % num_buckets_;
      if (buckets_[index_swap].entry == NULL) {
        buckets_[index_previous].entry = NULL;
        monitoring_->RemoveDIB(index_previous);
        break;
      }
      uint64_t distance;
      if (FillDistanceToInitIndex(index_swap, &distance) != 0) {
        fprintf(stderr, "Error in FillDistanceToInitIndex()"); 
      }
      if (distance == 0) {
        buckets_[index_previous].entry = NULL;
        monitoring_->RemoveDIB(index_previous);
        break;
      }
      buckets_[index_previous].entry = buckets_[index_swap].entry;
      buckets_[index_previous].hash = buckets_[index_swap].hash;
      monitoring_->SetDIB(index_previous, distance-1);
    }
    monitoring_->AddDSB(i);
    monitoring_->AddAlignedDSB(index_current, index_swap);
    num_buckets_used_ -= 1;
    return 0;
  }

  return 1;
}



int BackshiftHashMap::Resize() {
  // TODO: implement
  return 0;
}


// For debugging
int BackshiftHashMap::CheckDensity() {
  return 0;
}

int BackshiftHashMap::BucketCounts() {
  return 0;
}

int BackshiftHashMap::Dump() {
  return 0;
}


int BackshiftHashMap::GetBucketState(int index) {
  //printf("GetBucketState %d\n", index);
  if (buckets_[index].entry == NULL) {
    return 0;
  }

  return 1;
}

int BackshiftHashMap::FillInitIndex(uint64_t index_stored, uint64_t *index_init) {
  if(buckets_[index_stored].entry == NULL) return -1;
  *index_init = buckets_[index_stored].hash % num_buckets_;
  return 0;
}

int BackshiftHashMap::FillDistanceToInitIndex(uint64_t index_stored, uint64_t *distance) {
  if(buckets_[index_stored].entry == NULL) return -1;
  uint64_t index_init = buckets_[index_stored].hash % num_buckets_;
  if (index_init <= index_stored) {
    *distance = index_stored - index_init; 
  } else {
    *distance = index_stored + (num_buckets_ - index_init); 
  }
  return 0;
}


void BackshiftHashMap::GetMetadata(std::map< std::string, std::string >& metadata) {
  metadata["name"] = "backshift";
  char buffer[1024]; 
  sprintf(buffer, "{\"num_buckets\": %" PRIu64 ", \"probing_max\": %" PRIu64 "}", num_buckets_, probing_max_);
  metadata["parameters_hashmap"] = buffer;
  sprintf(buffer, "nb%" PRIu64 "-pm%" PRIu64 "", num_buckets_, probing_max_);
  metadata["parameters_hashmap_string"] = buffer;
}

}; // end namespace hashmap

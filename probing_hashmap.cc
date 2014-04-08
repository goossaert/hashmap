#include "probing_hashmap.h"

namespace hashmap {



int ProbingHashMap::Open() {
  buckets_ = new Bucket[num_buckets_];
  memset(buckets_, 0, sizeof(Bucket) * (num_buckets_));
  monitoring_ = new hashmap::Monitoring(num_buckets_, probing_max_, static_cast<HashMap*>(this));
  return 0;
}

int ProbingHashMap::Close() {
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



int ProbingHashMap::Get(const std::string& key, std::string* value) {
  uint64_t hash = hash_function(key);
  uint64_t index_init = hash % num_buckets_;
  bool found = false;
  for (uint32_t i = 0; i < probing_max_; i++) {
    uint64_t index_current = (index_init + i) % num_buckets_;
    if (buckets_[index_current].entry == DELETED_BUCKET) {
      continue;
    } else if (buckets_[index_current].entry == NULL) {
      break;
    }

    if(   key.size() == buckets_[index_current].entry->size_key
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




uint64_t ProbingHashMap::FindEmptyBucket(uint64_t index_init) {
  bool found = false;
  uint64_t index_current = index_init;

  for (uint32_t i = 0; i < probing_max_; i++) {
    index_current = index_init + i;
    if (   buckets_[index_current % num_buckets_].entry == NULL
        || buckets_[index_current % num_buckets_].entry == DELETED_BUCKET) {
      found = true;
      monitoring_->SetProbingSequenceLengthSearch(index_current % num_buckets_, i);
      monitoring_->AddDistanceToFreeBucket(i);
      break;
    }
  }

  if (!found) {
    return num_buckets_;
  }

  return index_current % num_buckets_;
}





int ProbingHashMap::Put(const std::string& key, const std::string& value) {
  uint64_t hash = hash_function(key);
  uint64_t index_init = hash % num_buckets_;
  uint64_t index_empty = FindEmptyBucket(index_init);

  if (index_empty == num_buckets_) {
    return 1; 
  }

  char *data = new char[key.size() + value.size()];
  memcpy(data, key.c_str(), key.size());
  memcpy(data + key.size(), value.c_str(), value.size());

  ProbingHashMap::Entry *entry = new ProbingHashMap::Entry;
  entry->size_key = key.size();
  entry->size_value = value.size();
  entry->data = data;
  buckets_[index_empty].entry = entry;
  buckets_[index_empty].hash  = hash;

  monitoring_->UpdateNumItemsInBucket(index_init, 1);

  return 0;
}


int ProbingHashMap::Exists(const std::string& key) {
  // TODO: implement
  return 0;
}


int ProbingHashMap::Remove(const std::string& key) {
  uint64_t hash = hash_function(key);
  uint64_t index_init = hash % num_buckets_;

  bool found = false;
  uint64_t index_current;

  for (uint32_t i = 0; i < probing_max_; i++) {
    index_current = (index_init + i) % num_buckets_;
    if (buckets_[index_current].entry == DELETED_BUCKET) {
      continue;
    } else if (buckets_[index_current].entry == NULL) {
      break;
    } else if (   key.size() == buckets_[index_current].entry->size_key
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
    //fprintf(stderr, "Remove() OK\n");
    return 0;
  } else {
    //fprintf(stderr, "Remove() not found - %" PRIu64 " %p\n", buckets_[index_current].hash, buckets_[index_current].entry);
  }

  return 1;
}


int ProbingHashMap::Resize() {
  // TODO: implement
  return 0;
}


// For debugging
int ProbingHashMap::CheckDensity() {
  return 0;
}

int ProbingHashMap::BucketCounts() {
  return 0;
}

int ProbingHashMap::Dump() {
  return 0;
}


int ProbingHashMap::GetBucketState(int index) {
  //printf("GetBucketState %d\n", index);
  if (buckets_[index].entry == NULL) {
    return 0;
  }

  return 1;
}

int ProbingHashMap::FillInitIndex(uint64_t index_stored, uint64_t *index_init) {
  if(buckets_[index_stored].entry == NULL) return -1;
  *index_init = buckets_[index_stored].hash % num_buckets_;
  return 0;
}


void ProbingHashMap::GetMetadata(std::map< std::string, std::string >& metadata) {
  metadata["name"] = "probing";
  char buffer[1024]; 
  sprintf(buffer, "{\"num_buckets\": %" PRIu64 ", \"probing_max\": %u}", num_buckets_, probing_max_);
  metadata["parameters_hashmap"] = buffer;
  sprintf(buffer, "nb%" PRIu64 "-pm%u", num_buckets_, probing_max_);
  metadata["parameters_hashmap_string"] = buffer;
}


}; // end namespace hashmap

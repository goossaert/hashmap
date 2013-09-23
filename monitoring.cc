#include "monitoring.h"
#include "hashmap.h"

namespace hashmap {

void Monitoring::PrintInfo(FILE* fd, std::string metric) {
  std::map<std::string, std::string> metadata;
  hm_->GetMetadata(metadata);
  fprintf(fd, " \"algorithm\": \"%s\",\n", metadata["name"].c_str());
  fprintf(fd, " \"testcase\": \"%s\",\n", testcase_.c_str());
  fprintf(fd, " \"metric\": \"%s\",\n", metric.c_str());
  fprintf(fd, " \"parameters_testcase\": %s,\n", parameters_testcase_json_.c_str());
  fprintf(fd, " \"parameters_testcase_string\": \"%s\",\n", parameters_testcase_string_.c_str());
  fprintf(fd, " \"parameters_hashmap\": %s,\n", metadata["parameters_hashmap"].c_str());
  fprintf(fd, " \"parameters_hashmap_string\": \"%s\",\n", metadata["parameters_hashmap_string"].c_str());
  fprintf(fd, " \"instance\": %llu,\n", instance_);
  fprintf(fd, " \"cycle\": %llu,\n", cycle_);
}

uint64_t Monitoring::UpdateNumItemsInBucket(uint64_t index,
                                            int32_t  increment) {
  std::map<uint64_t, uint64_t>::iterator it;
  it = num_items_in_bucket_.find(index);
  if (it == num_items_in_bucket_.end()) {
    if(increment > 0) {
      num_items_in_bucket_[index] = 0;
    } else {
      //fprintf(stderr, "UpdateNumItemsInBucket %llu %d -- return 0\n", index, increment);
      return 0; 
    }
  }

  //fprintf(stdout, "UpdateNumItemsInBucket %d %d\n", index, increment);

  uint64_t num_items_new = num_items_in_bucket_[index] + increment;
  if (num_items_new <= 0) {
    num_items_in_bucket_.erase(it);
    return 0;
  } else {
    num_items_in_bucket_[index] = num_items_new;
  }
  
  return num_items_new;
}


uint64_t Monitoring::GetNumItemsInBucket(uint64_t index) {
  return UpdateNumItemsInBucket(index, 0);
}


void Monitoring::GetDensity() {
  fprintf(stdout, "GetDensity() %zu\n", num_items_in_bucket_.size());
  density_.clear();
  std::map<uint64_t, uint64_t>::iterator it;
  std::map<uint64_t, uint64_t>::iterator it_count;
  for (it = num_items_in_bucket_.begin(); it != num_items_in_bucket_.end(); ++it) {
    it_count = density_.find(it->second);
    if (it_count == density_.end()) {
      density_[it->second] = 0;
    }
    density_[it->second] += 1;
  }
  fprintf(stdout, "GetDensity() out %zu\n", density_.size());
  //return density_;
}


uint64_t** Monitoring::GetClustering(HashMap* hm) {
  // This is a O(n^2) solution, but there is a O(n) one. If this gets too slow,
  // replace with the O(n) solution.
  uint64_t sizes_window[8] = { 8, 16, 32, 64, 128, 256, 512, 1024 };

  uint64_t **clustering = (uint64_t**) new uint64_t*[8];
  for (unsigned int i = 0; i < 8; i++) {
    clustering[i] = new uint64_t[ sizes_window[i] + 1 ];
    for (unsigned int j = 0; j < sizes_window[i]; j++) {
      clustering[i][j] = 0;
    }
  }

  for (uint64_t index_bucket = 0; index_bucket < num_buckets_; index_bucket++) {
    for (uint64_t index_window = 0; index_window < 8; index_window++) {
      if (index_bucket >= num_buckets_ - sizes_window[index_window]) {
        continue;
      }

      uint64_t count = 0;
      for (uint64_t i = 0; i < sizes_window[index_window]; i++) {
        uint64_t index_bucket_current = index_bucket + i;
        if (hm->GetBucketState(index_bucket_current) == 1) {
          count += 1;
        }
      }

      //if (index_bucket > sizes_window[index_window]) {
      //}
      clustering[index_window][count] += 1;
    }
  }

  return clustering;
}


void Monitoring::PrintClustering(HashMap *hm) {
  int sizes_window[5] = { 8, 16, 32, 64, 128 };
  uint64_t** clustering = hm->monitoring_->GetClustering(hm);
  for (int i = 0; i < 5; i++) {
    fprintf(stdout, "Cluster for window of size %d:\n", sizes_window[i]);
    for (int j = 0; j < sizes_window[i] + 1; j++) {
      fprintf(stdout, "    %5d: %5llu\n", j, clustering[i][j]);
    }
  }

  for (int i = 0; i < 8; i++) {
    delete[] clustering[i];
  }
  delete[] clustering;
}



void Monitoring::PrintDensity(std::string filepath) {
  printf("Density (number of items colliding inside each bucket):\n");
  GetDensity();
  std::map<uint64_t, uint64_t>::const_iterator it;
  uint64_t count_total = 0;
  for (it = density_.begin(); it != density_.end(); ++it) {
    count_total += it->second;
  }
  fprintf(stderr, "0\n");

  FILE* fd = NULL;
  if (filepath == "stdout") {
    fd = stdout;
  } else {
    if ((fd = fopen(filepath.c_str(), "w")) == NULL) {
      fprintf(stderr, "Could not open file [%s]: %s\n", filepath.c_str(), strerror(errno));
    }
    fprintf(stderr, "1\n");
  }


  fprintf(fd, "{\n");
  fprintf(stderr, "2\n");
  PrintInfo(fd, "density");
  fprintf(stderr, "3\n");
  fprintf(fd, " \"datapoints\":\n");
  fprintf(fd, "    {");

  //fprintf(fd, "      \"0\": %llu", num_buckets_ - count_total);
  bool first_item = true;
  for (it = density_.begin(); it != density_.end(); ++it) {
    if (!first_item) fprintf(fd, ",");
    first_item = false;
    fprintf(fd, "\n");
    fprintf(fd, "      \"%llu\": %llu", it->first, it->second);
  }
  fprintf(fd, "\n");
  fprintf(fd, "    }\n");
  fprintf(fd, "}\n");

  if (filepath != "stdout") {
    fclose(fd); 
  }
}


uint64_t Monitoring::GetProbingSequenceLengthSearch(uint64_t index) {
  std::map<uint64_t, uint64_t>::iterator it;
  it = psl_search_.find(index);
  if (it == psl_search_.end()) {
    return num_buckets_;
  }
  return psl_search_[index];
}


void Monitoring::SetProbingSequenceLengthSearch(uint64_t index, uint64_t psl) {
  psl_search_[index] = psl;
  //fprintf(stderr, "SetPSL [%llu]\n", index);
}

void Monitoring::RemoveProbingSequenceLengthSearch(uint64_t index) {
  std::map<uint64_t, uint64_t>::iterator it;
  it = psl_search_.find(index);
  if (it != psl_search_.end()) {
    psl_search_.erase(it);
  } else {
    //fprintf(stderr, "RemovePSL error: cannot find index [%llu]\n", index); 
  }

}





void Monitoring::PrintProbingSequenceLengthSearch(std::string filepath) {
  std::map<uint64_t, uint64_t> counts;
  std::map<uint64_t, uint64_t>::iterator it_psl, it_count, it_find;

  fprintf(stderr, "psl search size:%zu\n", psl_search_.size());

  for (it_psl = psl_search_.begin(); it_psl != psl_search_.end(); it_psl++) {
    it_find = counts.find(it_psl->second);
    if (it_find == counts.end()) {
      counts[it_psl->second] = 0;
    }
    counts[it_psl->second] += 1;
  }

  FILE* fd = NULL;
  if (filepath == "stdout") {
    fd = stdout;
  } else {
    fd = fopen(filepath.c_str(), "w");
  }

  fprintf(fd, "{\n");
  PrintInfo(fd, "probing_sequence_length_search");
  fprintf(fd, " \"datapoints\":\n");
  fprintf(fd, "    {\n");

  bool first_item = true;
  for (it_count = counts.begin(); it_count != counts.end(); it_count++) {
    if (!first_item) fprintf(fd, ",\n");
    first_item = false;
    fprintf(fd, "     \"%llu\": %llu", it_count->first, it_count->second);
  }
  fprintf(fd, "\n");
  fprintf(fd, "    }\n");
  fprintf(fd, "}\n");

  if (filepath != "stdout") {
    fclose(fd); 
  }

}



void Monitoring::GetNumScannedBlocks(std::vector< std::map<uint64_t, uint64_t> >& out_num_scanned_blocks, HashMap *hm) {
  int size_blocks[6] = { 1, 8, 16, 32, 64, 128 };

  std::map< uint64_t, uint64_t>::iterator it_find;
  for (uint64_t index_stored = 0; index_stored < num_buckets_; index_stored++) {
    uint64_t index_init;
    for (int i = 0; i < 6; i++) {
      // OPTIMIZE: offset_end is being recomputed and doesn't need to
      uint64_t offset_end = AlignOffsetToBlock(num_buckets_ * size_bucket_, size_blocks[i]);
      if (hm->FillInitIndex(index_stored, &index_init) != 0) continue;
      uint64_t offset_init = AlignOffsetToBlock(index_init * size_bucket_, size_blocks[i]);
      uint64_t offset_stored = AlignOffsetToBlock(index_stored * size_bucket_, size_blocks[i]);
      uint64_t num_blocks;
      if (offset_init <= offset_stored) {
        num_blocks = offset_stored - offset_init;
      } else {
        num_blocks = offset_end - offset_init + offset_stored;
      }
      it_find = out_num_scanned_blocks[i].find(num_blocks);
      if (it_find == out_num_scanned_blocks[i].end()) {
        out_num_scanned_blocks[i][num_blocks] = 0;
      }
      out_num_scanned_blocks[i][num_blocks] += 1;
    }
  }
}


void Monitoring::PrintNumScannedBlocks(std::string filepath) {
  // TODO: fix bug that can be reached with:
  // ./hashmap --algo shadow --num_buckets 1000000 --size_nh_start 4 --size_nh_end 64
  FILE* fd = NULL;
  if (filepath == "stdout") {
    fd = stdout;
  } else {
    fd = fopen(filepath.c_str(), "w");
  }

  fprintf(fd, "{\n");
  PrintInfo(fd, "num_scanned_blocks");
  fprintf(fd, " \"datapoints\":\n");
  fprintf(fd, "    [");

  std::vector< std::map<uint64_t, uint64_t> > num_scanned_blocks(6);
  GetNumScannedBlocks(num_scanned_blocks, hm_);
  int size_blocks[6] = { 1, 8, 16, 32, 64, 128 };
  for (int i = 0; i < 6; i++) {
    if (i > 0) fprintf(fd, ",");
    fprintf(fd, "\n");
    fprintf(fd, "     {\"block_size\": %d,\n", size_blocks[i]);
    fprintf(fd, "      \"datapoints\": \n");
    fprintf(fd, "         [");
    std::map<uint64_t, uint64_t>::iterator it;
    bool first_item = true;
    for (it = num_scanned_blocks[i].begin(); it != num_scanned_blocks[i].end(); ++it) {
      if (!first_item) fprintf(fd, ",");
      first_item = false;
      fprintf(fd, "\n");
      fprintf(fd, "          {\"%llu\": %llu}", it->first, it->second);
    }
    fprintf(fd, "\n");
    fprintf(fd, "         ]\n");
    fprintf(fd, "     }");
  }

  fprintf(fd, "\n");
  fprintf(fd, "    ]\n");
  fprintf(fd, "}\n");

  if (filepath != "stdout") {
    fclose(fd); 
  }
}


}; // end namespace hashmap

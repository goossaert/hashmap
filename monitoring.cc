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
  fprintf(fd, " \"instance\": %" PRIu64 ",\n", instance_);
  fprintf(fd, " \"cycle\": %" PRIu64 ",\n", cycle_);
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
      fprintf(stdout, "    %5d: %5" PRIu64 "\n", j, clustering[i][j]);
    }
  }

  for (int i = 0; i < 8; i++) {
    delete[] clustering[i];
  }
  delete[] clustering;
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
  //fprintf(stderr, "SetPSL [%" PRIu64 "]\n", index);
}

void Monitoring::RemoveProbingSequenceLengthSearch(uint64_t index) {
  std::map<uint64_t, uint64_t>::iterator it;
  it = psl_search_.find(index);
  if (it != psl_search_.end()) {
    psl_search_.erase(it);
  } else {
    //fprintf(stderr, "RemovePSL error: cannot find index [%" PRIu64 "]\n", index); 
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
    fprintf(fd, "     \"%" PRIu64 "\": %" PRIu64, it_count->first, it_count->second);
  }
  fprintf(fd, "\n");
  fprintf(fd, "    }\n");
  fprintf(fd, "}\n");

  if (filepath != "stdout") {
    fclose(fd); 
  }

}


/*
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
*/


void Monitoring::GetNumScannedBlocks(std::vector< std::map<uint64_t, uint64_t> >& out_num_scanned_blocks, HashMap *hm) {
  int size_blocks[11] = { 1, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096 };

  std::map< uint64_t, uint64_t>::iterator it_find;
  for (uint64_t index_stored = 0; index_stored < num_buckets_; index_stored++) {
    uint64_t index_init;
    int index_selected = 10;
    for (int i = 10; i > 0; i--) {
      // OPTIMIZE: offset_end is being recomputed and doesn't need to
      uint64_t offset_end = AlignOffsetToBlock(num_buckets_ * size_bucket_, size_blocks[i]);
      if (hm->FillInitIndex(index_stored, &index_init) != 0) continue;
      uint64_t offset_init = AlignOffsetToBlock(index_init * size_bucket_, size_blocks[i]);
      uint64_t offset_stored = AlignOffsetToBlock(index_stored * size_bucket_, size_blocks[i]);
      int num_bytes;
      if (offset_init <= offset_stored) {
        num_bytes = offset_stored - offset_init;
      } else {
        num_bytes = offset_end - offset_init + offset_stored;
      }
      if (num_bytes > 0) {
        break;
      }
      index_selected = i;
    }

    it_find = out_num_scanned_blocks[0].find(index_selected);
    if (it_find == out_num_scanned_blocks[0].end()) {
      out_num_scanned_blocks[0][index_selected] = 0;
    }
    out_num_scanned_blocks[0][index_selected] += 1;
  }
}




void Monitoring::PrintNumScannedBlocks(std::string filepath) {
  FILE* fd = NULL;
  if (filepath == "stdout") {
    fd = stdout;
  } else {
    fd = fopen(filepath.c_str(), "w");
  }
  fprintf(fd, "[\n");

  char metric[1024];
  std::vector< std::map<uint64_t, uint64_t> > num_scanned_blocks(1);
  GetNumScannedBlocks(num_scanned_blocks, hm_);
  int size_blocks[1] = { 1 };//, 8, 16, 32, 64, 128 };
  for (int i = 0; i < 1; i++) {
    if (i > 0) fprintf(fd, ",");
    fprintf(fd, "{\n");
    sprintf(metric, "num_scanned_blocks_%d", size_blocks[i]);
    PrintInfo(fd, metric);
    fprintf(fd, " \"datapoints\":\n");
    fprintf(fd, "    {");
    std::map<uint64_t, uint64_t>::iterator it;
    bool first_item = true;
    for (it = num_scanned_blocks[i].begin(); it != num_scanned_blocks[i].end(); ++it) {
      if (!first_item) fprintf(fd, ",");
      first_item = false;
      fprintf(fd, "\n");
      fprintf(fd, "      \"%" PRIu64 "\": %" PRIu64, it->first, it->second);
    }
    fprintf(fd, "\n");
    fprintf(fd, "    }\n");
    fprintf(fd, "}\n");
  }

  fprintf(fd, "]\n");
  if (filepath != "stdout") {
    fclose(fd); 
  }
}



void Monitoring::AddDistanceToFreeBucket(uint64_t distance) {
                                            
  std::map<uint64_t, uint64_t>::iterator it;
  it = psl_insert_.find(distance);
  if (it == psl_insert_.end()) {
      psl_insert_[distance] = 0;
  }
  psl_insert_[distance] += 1;
}


void Monitoring::ResetDistanceToFreeBucket() {
  psl_insert_.clear();
}


void Monitoring::PrintDistanceToFreeBucket(std::string filepath) {
  std::map<uint64_t, uint64_t>::iterator it;

  FILE* fd = NULL;
  if (filepath == "stdout") {
    fd = stdout;
  } else {
    fd = fopen(filepath.c_str(), "w");
  }

  fprintf(fd, "{\n");
  PrintInfo(fd, "distance_free_bucket");
  fprintf(fd, " \"datapoints\":\n");
  fprintf(fd, "    {\n");

  bool first_item = true;
  for (it = psl_insert_.begin(); it != psl_insert_.end(); it++) {
    if (!first_item) fprintf(fd, ",\n");
    first_item = false;
    fprintf(fd, "     \"%" PRIu64 "\": %" PRIu64, it->first, it->second);
  }
  fprintf(fd, "\n");
  fprintf(fd, "    }\n");
  fprintf(fd, "}\n");

  if (filepath != "stdout") {
    fclose(fd);
  }
}


void Monitoring::AddAlignedDistanceToFreeBucket(uint64_t index_init, uint64_t index_free_bucket) {
  int size_blocks[11] = { 1, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096 };//, 
  std::map<uint64_t, uint64_t>::iterator it_find;
  int index_selected = 10;
  for (int i = 10; i > 0; i--) {
    uint64_t offset_end = AlignOffsetToBlock(num_buckets_ * size_bucket_, size_blocks[i]);
    uint64_t offset_init = AlignOffsetToBlock(index_init * size_bucket_, size_blocks[i]);
    uint64_t offset_stored = AlignOffsetToBlock(index_free_bucket * size_bucket_, size_blocks[i]);
    int num_bytes;
    if (offset_init <= offset_stored) {
      num_bytes = offset_stored - offset_init;
    } else {
      num_bytes = offset_end - offset_init + offset_stored;
    }

    if (num_bytes > 0) {
      //printf("index_init: %" PRIu64 " index_fb: %" PRIu64 " | num_bytes: %d offset_init: %" PRIu64 " offset_stored: %" PRIu64 " size_block: %d\n", index_init, index_free_bucket, num_bytes, offset_init, offset_stored, size_blocks[index_selected]);
      break;
    }
    index_selected = i;
  }

  it_find = aligned_psl_insert_.find(index_selected);
  if (it_find == aligned_psl_insert_.end()) {
    aligned_psl_insert_[index_selected] = 0;
  }
  aligned_psl_insert_[index_selected] += 1;
}



void Monitoring::ResetAlignedDistanceToFreeBucket() {
  aligned_psl_insert_.clear();
}


void Monitoring::PrintAlignedDistanceToFreeBucket(std::string filepath) {
  std::map<uint64_t, uint64_t>::iterator it;

  FILE* fd = NULL;
  if (filepath == "stdout") {
    fd = stdout;
  } else {
    fd = fopen(filepath.c_str(), "w");
  }

  fprintf(fd, "{\n");
  PrintInfo(fd, "aligned_distance_to_free_bucket");
  fprintf(fd, " \"datapoints\":\n");
  fprintf(fd, "    {\n");

  bool first_item = true;
  for (it = aligned_psl_insert_.begin(); it != aligned_psl_insert_.end(); it++) {
    if (!first_item) fprintf(fd, ",\n");
    first_item = false;
    fprintf(fd, "     \"%" PRIu64 "\": %" PRIu64, it->first, it->second);
  }
  fprintf(fd, "\n");
  fprintf(fd, "    }\n");
  fprintf(fd, "}\n");

  if (filepath != "stdout") {
    fclose(fd);
  }
}







void Monitoring::AddNumberOfSwaps(uint64_t distance) {
                                            
  std::map<uint64_t, uint64_t>::iterator it;
  it = swaps_.find(distance);
  if (it == swaps_.end()) {
      swaps_[distance] = 0;
  }
  swaps_[distance] += 1;
}


void Monitoring::ResetNumberOfSwaps() {
  swaps_.clear();
}


void Monitoring::PrintNumberOfSwaps(std::string filepath) {
  std::map<uint64_t, uint64_t>::iterator it;

  FILE* fd = NULL;
  if (filepath == "stdout") {
    fd = stdout;
  } else {
    fd = fopen(filepath.c_str(), "w");
  }

  fprintf(fd, "{\n");
  PrintInfo(fd, "swap");
  fprintf(fd, " \"datapoints\":\n");
  fprintf(fd, "    {\n");

  bool first_item = true;
  for (it = swaps_.begin(); it != swaps_.end(); it++) {
    if (!first_item) fprintf(fd, ",\n");
    first_item = false;
    fprintf(fd, "     \"%" PRIu64 "\": %" PRIu64, it->first, it->second);
  }
  fprintf(fd, "\n");
  fprintf(fd, "    }\n");
  fprintf(fd, "}\n");

  if (filepath != "stdout") {
    fclose(fd);
  }
}




void Monitoring::AddDMB(uint64_t distance) {
                                            
  std::map<uint64_t, uint64_t>::iterator it;
  it = dmb_.find(distance);
  if (it == dmb_.end()) {
      dmb_[distance] = 0;
  }
  dmb_[distance] += 1;
  //printf("Add DMB %" PRIu64 "\n", distance);
}


void Monitoring::ResetDMB() {
  dmb_.clear();
}


void Monitoring::PrintDMB(std::string filepath) {
  std::map<uint64_t, uint64_t>::iterator it;

  FILE* fd = NULL;
  if (filepath == "stdout") {
    fd = stdout;
  } else {
    fd = fopen(filepath.c_str(), "w");
  }

  fprintf(fd, "{\n");
  PrintInfo(fd, "distance_to_missing_bucket");
  fprintf(fd, " \"datapoints\":\n");
  fprintf(fd, "    {\n");

  bool first_item = true;
  for (it = dmb_.begin(); it != dmb_.end(); it++) {
    if (!first_item) fprintf(fd, ",\n");
    first_item = false;
    fprintf(fd, "     \"%" PRIu64 "\": %" PRIu64, it->first, it->second);
  }
  fprintf(fd, "\n");
  fprintf(fd, "    }\n");
  fprintf(fd, "}\n");

  if (filepath != "stdout") {
    fclose(fd);
  }
}




void Monitoring::AddAlignedDMB(uint64_t index_init, uint64_t index_free_bucket) {
  int size_blocks[11] = { 1, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096 };
  uint64_t powerOf2 = 8;
  std::map<uint64_t, uint64_t>::iterator it_find;
  if (index_init > index_free_bucket) {
    index_free_bucket += num_buckets_ * size_bucket_;
  }
  int index_selected = 63;
  for (int i = 1; i < 63; i++) {
    //uint64_t offset_end = AlignOffsetToBlock(num_buckets_ * size_bucket_, size_blocks[i]);
    //uint64_t offset_init = AlignOffsetToBlock(index_init * size_bucket_, size_blocks[i]);
    //uint64_t offset_stored = AlignOffsetToBlock(index_free_bucket * size_bucket_, size_blocks[i]);

    uint64_t offset_end = AlignOffsetToBlock(num_buckets_ * size_bucket_, powerOf2);
    uint64_t offset_init = AlignOffsetToBlock(index_init * size_bucket_, powerOf2);
    uint64_t offset_stored = AlignOffsetToBlock(index_free_bucket * size_bucket_, powerOf2);

    //int num_bytes;
    //if (offset_init <= offset_stored) {
    //  num_bytes = offset_stored - offset_init;
    //} else {
    //  num_bytes = offset_end - offset_init + offset_stored;
    //}

    if (offset_init == offset_stored) {
      index_selected = i;
      break;
    }

    powerOf2 *= 2;
  }

  it_find = aligned_dmb_.find(index_selected);
  if (it_find == aligned_dmb_.end()) {
    aligned_dmb_[index_selected] = 0;
  }
  aligned_dmb_[index_selected] += 1;

}




void Monitoring::ResetAlignedDMB() {
  aligned_dmb_.clear();
}


void Monitoring::PrintAlignedDMB(std::string filepath) {
  std::map<uint64_t, uint64_t>::iterator it;

  FILE* fd = NULL;
  if (filepath == "stdout") {
    fd = stdout;
  } else {
    fd = fopen(filepath.c_str(), "w");
  }

  fprintf(fd, "{\n");
  PrintInfo(fd, "aligned_distance_to_missing_bucket");
  fprintf(fd, " \"datapoints\":\n");
  fprintf(fd, "    {\n");

  bool first_item = true;
  for (it = aligned_dmb_.begin(); it != aligned_dmb_.end(); it++) {
    if (!first_item) fprintf(fd, ",\n");
    first_item = false;
    fprintf(fd, "     \"%" PRIu64 "\": %" PRIu64, it->first, it->second);
  }
  fprintf(fd, "\n");
  fprintf(fd, "    }\n");
  fprintf(fd, "}\n");

  if (filepath != "stdout") {
    fclose(fd);
  }
}









}; // end namespace hashmap

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



uint64_t Monitoring::GetDIB(uint64_t index) {
  std::map<uint64_t, uint64_t>::iterator it;
  it = dib_.find(index);
  if (it == dib_.end()) {
    return num_buckets_;
  }
  return dib_[index];
}


void Monitoring::SetDIB(uint64_t index, uint64_t dib) {
  dib_[index] = dib;
  //fprintf(stderr, "SetPSL [%" PRIu64 "]\n", index);
}

void Monitoring::RemoveDIB(uint64_t index) {
  std::map<uint64_t, uint64_t>::iterator it;
  it = dib_.find(index);
  if (it != dib_.end()) {
    dib_.erase(it);
  } else {
    //fprintf(stderr, "RemovePSL error: cannot find index [%" PRIu64 "]\n", index); 
  }

}





void Monitoring::PrintDIB(std::string filepath) {
  std::map<uint64_t, uint64_t> counts;
  std::map<uint64_t, uint64_t>::iterator it_dib, it_count, it_find;

  fprintf(stderr, "dib search size:%zu\n", dib_.size());

  for (it_dib = dib_.begin(); it_dib != dib_.end(); it_dib++) {
    it_find = counts.find(it_dib->second);
    if (it_find == counts.end()) {
      counts[it_dib->second] = 0;
    }
    counts[it_dib->second] += 1;
  }

  FILE* fd = NULL;
  if (filepath == "stdout") {
    fd = stdout;
  } else {
    fd = fopen(filepath.c_str(), "w");
  }

  fprintf(fd, "{\n");
  PrintInfo(fd, "DIB");
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



void Monitoring::GetNumScannedBlocks(std::map<uint64_t, uint64_t>& out_num_scanned_blocks, HashMap *hm) {

  std::map< uint64_t, uint64_t>::iterator it_find;
  for (uint64_t index_stored = 0; index_stored < num_buckets_; index_stored++) {
    uint64_t index_init;
    if (hm->FillInitIndex(index_stored, &index_init) != 0) continue;

    uint64_t index_stored_adjusted;
    if (index_init <= index_stored) {
      index_stored_adjusted = index_stored;
    } else {
      index_stored_adjusted = index_stored + num_buckets_;
    }

    //for (int i = 10; i > 0; i--) {
    int index_selected = 64;
    uint64_t chunk_size = 16;
    for (int i = 4; i < 64; i++) {
      uint64_t offset_init = AlignOffsetToBlock(index_init * size_bucket_, chunk_size);
      uint64_t offset_stored = AlignOffsetToBlock(index_stored_adjusted * size_bucket_, chunk_size);

      if (offset_init == offset_stored) {
        index_selected = i;
        break;
      }

      chunk_size *= 2;
    }

    it_find = out_num_scanned_blocks.find(index_selected);
    if (it_find == out_num_scanned_blocks.end()) {
      out_num_scanned_blocks[index_selected] = 0;
    }
    out_num_scanned_blocks[index_selected] += 1;
  }
}




void Monitoring::PrintNumScannedBlocks(std::string filepath) {
  FILE* fd = NULL;
  if (filepath == "stdout") {
    fd = stdout;
  } else {
    fd = fopen(filepath.c_str(), "w");
  }

  char metric[1024];
  std::map<uint64_t, uint64_t> num_scanned_blocks;
  GetNumScannedBlocks(num_scanned_blocks, hm_);
  fprintf(fd, "{\n");
  sprintf(metric, "aligned DIB");
  PrintInfo(fd, metric);
  fprintf(fd, " \"datapoints\":\n");
  fprintf(fd, "    {");
  std::map<uint64_t, uint64_t>::iterator it;
  bool first_item = true;
  for (it = num_scanned_blocks.begin(); it != num_scanned_blocks.end(); ++it) {
    if (!first_item) fprintf(fd, ",");
    first_item = false;
    fprintf(fd, "\n");
    fprintf(fd, "      \"%" PRIu64 "\": %" PRIu64, it->first, it->second);
  }
  fprintf(fd, "\n");
  fprintf(fd, "    }\n");
  fprintf(fd, "}\n");

  if (filepath != "stdout") {
    fclose(fd); 
  }
}



void Monitoring::AddDFB(uint64_t distance) {
                                            
  std::map<uint64_t, uint64_t>::iterator it;
  it = dfb_.find(distance);
  if (it == dfb_.end()) {
      dfb_[distance] = 0;
  }
  dfb_[distance] += 1;
}


void Monitoring::ResetDFB() {
  dfb_.clear();
}


void Monitoring::PrintDFB(std::string filepath) {
  std::map<uint64_t, uint64_t>::iterator it;

  FILE* fd = NULL;
  if (filepath == "stdout") {
    fd = stdout;
  } else {
    fd = fopen(filepath.c_str(), "w");
  }

  fprintf(fd, "{\n");
  PrintInfo(fd, "DFB");
  fprintf(fd, " \"datapoints\":\n");
  fprintf(fd, "    {\n");

  bool first_item = true;
  for (it = dfb_.begin(); it != dfb_.end(); it++) {
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


void Monitoring::AddAlignedDFB(uint64_t index_init, uint64_t index_free_bucket) {
  std::map<uint64_t, uint64_t>::iterator it_find;

  if (index_init > index_free_bucket) {
    index_free_bucket += num_buckets_;
  }
  int index_selected = 64;
  uint64_t chunk_size = 16;
  for (int i = 4; i < 64; i++) {
    uint64_t offset_init = AlignOffsetToBlock(index_init * size_bucket_, chunk_size);
    uint64_t offset_free_bucket = AlignOffsetToBlock(index_free_bucket * size_bucket_, chunk_size);
    if (offset_init == offset_free_bucket) {
      index_selected = i;
      break;
    }

    chunk_size *= 2;
  }

  it_find = aligned_dfb_.find(index_selected);
  if (it_find == aligned_dfb_.end()) {
    aligned_dfb_[index_selected] = 0;
  }
  aligned_dfb_[index_selected] += 1;
}



void Monitoring::ResetAlignedDFB() {
  aligned_dfb_.clear();
}


void Monitoring::PrintAlignedDFB(std::string filepath) {
  std::map<uint64_t, uint64_t>::iterator it;

  FILE* fd = NULL;
  if (filepath == "stdout") {
    fd = stdout;
  } else {
    fd = fopen(filepath.c_str(), "w");
  }

  fprintf(fd, "{\n");
  PrintInfo(fd, "aligned DFB");
  fprintf(fd, " \"datapoints\":\n");
  fprintf(fd, "    {\n");

  bool first_item = true;
  for (it = aligned_dfb_.begin(); it != aligned_dfb_.end(); it++) {
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
  PrintInfo(fd, "DMB");
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




void Monitoring::AddAlignedDMB(uint64_t index_init, uint64_t index_missing_bucket) {
  std::map<uint64_t, uint64_t>::iterator it_find;
  if (index_init > index_missing_bucket) {
    index_missing_bucket += num_buckets_;
  }
  int index_selected = 64;
  uint64_t chunk_size = 16;
  for (int i = 4; i < 64; i++) {
    uint64_t offset_init = AlignOffsetToBlock(index_init * size_bucket_, chunk_size);
    uint64_t offset_missing_bucket = AlignOffsetToBlock(index_missing_bucket * size_bucket_, chunk_size);
    if (offset_init == offset_missing_bucket) {
      index_selected = i;
      break;
    }

    chunk_size *= 2;
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
  PrintInfo(fd, "aligned DMB");
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



void Monitoring::AddDSB(uint64_t distance) {
                                            
  std::map<uint64_t, uint64_t>::iterator it;
  it = dsb_.find(distance);
  if (it == dsb_.end()) {
      dsb_[distance] = 0;
  }
  dsb_[distance] += 1;
  //printf("Add DSB %" PRIu64 "\n", distance);
}


void Monitoring::ResetDSB() {
  dsb_.clear();
}


void Monitoring::PrintDSB(std::string filepath) {
  std::map<uint64_t, uint64_t>::iterator it;

  FILE* fd = NULL;
  if (filepath == "stdout") {
    fd = stdout;
  } else {
    fd = fopen(filepath.c_str(), "w");
  }

  fprintf(fd, "{\n");
  PrintInfo(fd, "DSB");
  fprintf(fd, " \"datapoints\":\n");
  fprintf(fd, "    {\n");

  bool first_item = true;
  for (it = dsb_.begin(); it != dsb_.end(); it++) {
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




void Monitoring::AddAlignedDSB(uint64_t index_stored, uint64_t index_shift_bucket) {
  std::map<uint64_t, uint64_t>::iterator it_find;
  if (index_stored > index_shift_bucket) {
    index_shift_bucket += num_buckets_;
  }
  int index_selected = 64;
  uint64_t chunk_size = 16;
  for (int i = 4; i < 64; i++) {
    uint64_t offset_stored = AlignOffsetToBlock(index_stored * size_bucket_, chunk_size);
    uint64_t offset_shift_bucket = AlignOffsetToBlock(index_shift_bucket * size_bucket_, chunk_size);
    if (offset_stored == offset_shift_bucket) {
      index_selected = i;
      break;
    }

    chunk_size *= 2;
  }

  it_find = aligned_dsb_.find(index_selected);
  if (it_find == aligned_dsb_.end()) {
    aligned_dsb_[index_selected] = 0;
  }
  aligned_dsb_[index_selected] += 1;

}




void Monitoring::ResetAlignedDSB() {
  aligned_dsb_.clear();
}


void Monitoring::PrintAlignedDSB(std::string filepath) {
  std::map<uint64_t, uint64_t>::iterator it;

  FILE* fd = NULL;
  if (filepath == "stdout") {
    fd = stdout;
  } else {
    fd = fopen(filepath.c_str(), "w");
  }

  fprintf(fd, "{\n");
  PrintInfo(fd, "aligned DSB");
  fprintf(fd, " \"datapoints\":\n");
  fprintf(fd, "    {\n");

  bool first_item = true;
  for (it = aligned_dsb_.begin(); it != aligned_dsb_.end(); it++) {
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

#include "monitoring.h"
#include "hashmap.h"

namespace hashmap {

uint64_t Monitoring::UpdateNumItemsInBucket(uint64_t index,
                                            int32_t  increment) {
  std::map<uint64_t, uint64_t>::iterator it;
  it = num_items_in_bucket_.find(index);
  if (it == num_items_in_bucket_.end()) {
    if(increment > 0) {
      num_items_in_bucket_[index] = 0;
    } else {
      return 0; 
    }
  }

  uint64_t num_items_new = num_items_in_bucket_[index] + increment;
  if (num_items_new < 0) {
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


const std::map<uint64_t, uint64_t>& Monitoring::GetDensity() {
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
  return density_;
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



void Monitoring::PrintDensity() {
  printf("Density (number of items colliding inside each bucket):\n");
  const std::map<uint64_t, uint64_t>& density = GetDensity();
  std::map<uint64_t, uint64_t>::const_iterator it;
  uint64_t count_total = 0;
  for (it = density.begin(); it != density.end(); ++it) {
    count_total += it->second;
  }
  fprintf(stdout, "%5d - %5llu\n", 0, num_buckets_ - count_total);
  for (it = density.begin(); it != density.end(); ++it) {
    fprintf(stdout, "%5llu - %5llu\n", it->first, it->second);
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
}


void Monitoring::PrintProbingSequenceLengthSearch() {
  std::map<uint64_t, uint64_t> counts;
  std::map<uint64_t, uint64_t>::iterator it_psl, it_count, it_find;

  for (it_psl = psl_search_.begin(); it_psl != psl_search_.end(); it_psl++) {
    it_find = counts.find(it_psl->second);
    if (it_find == counts.end()) {
      counts[it_psl->second] = 0;
    }
    counts[it_psl->second] += 1;
  }
 
  fprintf(stdout, "Probing sequence length for search (PSL-S)\n");
  for (it_count = counts.begin(); it_count != counts.end(); it_count++) {
    fprintf(stdout, "%5llu - %5llu\n", it_count->first, it_count->second);
  }
}



void Monitoring::GetNumScannedBlocks(std::vector< std::map<uint64_t, uint64_t> >& out_num_scanned_blocks, HashMap *hm) {
  int size_blocks[6] = { 1, 8, 16, 32, 64, 128 };

  std::map< uint64_t, uint64_t>::iterator it_find;
  for (uint64_t index_stored = 0; index_stored < num_buckets_; index_stored++) {
    uint64_t index_init;
    for (int i = 0; i < 6; i++) {
      if (hm->FillInitIndex(index_stored, &index_init) != 0) continue;
      fprintf(stdout, "%d\n", i);
      uint64_t offset_init = AlignOffsetToBlock(index_init * size_bucket_, size_blocks[i]);
      uint64_t offset_stored = AlignOffsetToBlock(index_stored * size_bucket_, size_blocks[i]);
      //if (offset_init < offset_stored)
      uint64_t num_blocks = offset_stored - offset_init;
      it_find = out_num_scanned_blocks[i].find(num_blocks);
      if (it_find == out_num_scanned_blocks[i].end()) {
        out_num_scanned_blocks[i][num_blocks] = 0;
      }
      out_num_scanned_blocks[i][num_blocks] += 1;
    }
  }
}


void Monitoring::PrintNumScannedBlocks(HashMap *hm) {
  fprintf(stdout, "PrintNumScannedBlocks\n");
  std::vector< std::map<uint64_t, uint64_t> > num_scanned_blocks(6);
  for (int i = 0; i < 6; i++) {
    fprintf(stdout, "blah %d\n", i);
    //num_scanned_blocks.insert( std::map<uint64_t, uint64_t>() );
  }
  GetNumScannedBlocks(num_scanned_blocks, hm);
  int size_blocks[6] = { 1, 8, 16, 32, 64, 128 };
  for (int i = 0; i < 6; i++) {
    fprintf(stdout, "Block size %d\n", size_blocks[i]);
    std::map<uint64_t, uint64_t>::iterator it;
    for (it = num_scanned_blocks[i].begin(); it != num_scanned_blocks[i].end(); ++it) {
      fprintf(stdout, "%6llu %6llu\n", it->first, it->second);
    }
  }
}


}; // end namespace hashmap

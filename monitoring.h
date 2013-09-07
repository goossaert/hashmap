#ifndef HASHMAP_MONITORING
#define HASHMAP_MONITORING

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
#include <inttypes.h>
#include <string.h>
#include <stdio.h>

#include <string>
#include <iostream>
#include <map>
#include <vector>



namespace hashmap
{
class HashMap;

class Monitoring
{
public:
  Monitoring(uint64_t num_buckets,
             uint64_t max_num_items_in_bucket,
             HashMap *hm) {
    num_buckets_ = num_buckets;
    max_num_items_in_bucket_ = max_num_items_in_bucket;
    size_bucket_ = 4;
    hm_ = hm;
  }

  virtual ~Monitoring() {
  }

  uint64_t UpdateNumItemsInBucket(uint64_t index,
                                  int32_t increment);
  uint64_t GetNumItemsInBucket(uint64_t index);
  const std::map<uint64_t, uint64_t>& GetDensity();
  void PrintDensity(std::string filepath);

  uint64_t** GetClustering(HashMap* hm);
  void PrintClustering(HashMap *hm);

  const std::map<uint64_t, uint64_t>& GetClustering();
  uint64_t GetProbingSequenceLengthSearch(uint64_t index);
  void SetProbingSequenceLengthSearch(uint64_t index, uint64_t psl);
  void PrintProbingSequenceLengthSearch();

  void GetNumScannedBlocks(std::vector< std::map<uint64_t, uint64_t> >& out_num_scanned_blocks, HashMap *hm);
  void PrintNumScannedBlocks(std::string filepath);
  void PrintInfo(FILE* fd, std::string metric);


private:
  std::map<uint64_t, uint64_t> num_items_in_bucket_;
  uint64_t num_buckets_;
  uint64_t max_num_items_in_bucket_;
  uint64_t size_bucket_;
  std::map<uint64_t, uint64_t> density_;
  std::map<uint64_t, uint64_t> psl_search_;
  HashMap *hm_;



  uint64_t AlignOffsetToBlock(uint64_t offset, uint64_t size_block) {
    return offset - offset % size_block;
  }

};


}; // end namespace hashmap

#endif // HASHMAP_MONITORING

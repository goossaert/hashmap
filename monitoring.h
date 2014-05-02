#ifndef HASHMAP_MONITORING
#define HASHMAP_MONITORING

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
#include <inttypes.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

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
    fprintf(stderr, "starting\n");
    //extra_parameters_ = "blah";
    //fprintf(stderr, "%s\n", extra_parameters_.c_str());
  }

  virtual ~Monitoring() {
  }

  uint64_t** GetClustering(HashMap* hm);
  void PrintClustering(HashMap *hm);
  const std::map<uint64_t, uint64_t>& GetClustering();

  uint64_t GetDIB(uint64_t index);
  void SetDIB(uint64_t index, uint64_t dib);
  void RemoveDIB(uint64_t index);
  void PrintDIB(std::string filepath);

  void AddDFB(uint64_t distance);
  void ResetDFB();
  void PrintDFB(std::string filepath);

  void AddAlignedDFB(uint64_t index_init, uint64_t index_free_bucket);
  void ResetAlignedDFB();
  void PrintAlignedDFB(std::string filepath);

  void AddDMB(uint64_t distance);
  void ResetDMB();
  void PrintDMB(std::string filepath);
  
  void AddAlignedDMB(uint64_t index_init, uint64_t index_free_bucket);
  void ResetAlignedDMB();
  void PrintAlignedDMB(std::string filepath);

  void AddNumberOfSwaps(uint64_t distance);
  void ResetNumberOfSwaps();
  void PrintNumberOfSwaps(std::string filepath);

  void GetNumScannedBlocks(std::map<uint64_t, uint64_t>& out_num_scanned_blocks, HashMap *hm);
  void PrintNumScannedBlocks(std::string filepath);

  void AddDSB(uint64_t distance);
  void ResetDSB();
  void PrintDSB(std::string filepath);
  
  void AddAlignedDSB(uint64_t index_stored, uint64_t index_shift_bucket);
  void ResetAlignedDSB();
  void PrintAlignedDSB(std::string filepath);

  void PrintInfo(FILE* fd, std::string metric);
  void SetCycle(uint64_t cycle) { cycle_ = cycle; }
  void SetInstance(uint64_t instance) { instance_ = instance; }

  void SetTestcase(std::string str) {
    testcase_ = str;
  }

  void SetParametersTestcaseString(std::string str) {
    parameters_testcase_string_ = str;
  }

  void SetParametersTestcaseJson(std::string str) {
    parameters_testcase_json_ = str;
  }


private:
  std::map<uint64_t, uint64_t> num_items_in_bucket_;
  uint64_t num_buckets_;
  uint64_t max_num_items_in_bucket_;
  uint64_t size_bucket_;
  std::map<uint64_t, uint64_t> dib_;
  std::map<uint64_t, uint64_t> dfb_;
  std::map<uint64_t, uint64_t> aligned_dfb_;
  std::map<uint64_t, uint64_t> dmb_;
  std::map<uint64_t, uint64_t> aligned_dmb_;
  std::map<uint64_t, uint64_t> dsb_;
  std::map<uint64_t, uint64_t> aligned_dsb_;
  std::map<uint64_t, uint64_t> swaps_;
  HashMap *hm_;
  uint64_t cycle_;
  uint64_t instance_;
  std::string parameters_testcase_string_;
  std::string parameters_testcase_json_;
  std::string testcase_;

  uint64_t AlignOffsetToBlock(uint64_t offset, uint64_t size_block) {
    return offset - offset % size_block;
  }

};


}; // end namespace hashmap

#endif // HASHMAP_MONITORING

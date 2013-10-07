#include "testcase.h"


// TODO: Factorize as much as possible accross the test cases.

namespace hashmap
{

int exists_or_mkdir(const char *path) {
  struct stat sb;

  if (stat(path, &sb) == 0) {
    if (!S_ISDIR(sb.st_mode)) {
      return 1;
    }
  } else if (mkdir(path, 0777) != 0) {
    return 1;
  }

  return 0;
}


void TestCase::InsertEntries(uint32_t num_items, std::set<std::string>& keys) {
  std::string key;
  int key_size = 16;
  char buffer[key_size + 1];
  buffer[key_size] = '\0';
  char alpha[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  std::set<std::string>::iterator it_find;

  for (uint32_t j = 0; j < num_items; j++) {
    bool is_valid = false;
    while (!is_valid) {
      for (int k = 0; k < key_size; k++) {
        buffer[k] = alpha[rand() % 62];
      }
      key = buffer;
      it_find = keys.find(key);
      if (it_find == keys.end()) {
        is_valid = true;
      } else {
        //fprintf(stdout, "%s\n", key.c_str());
        //fprintf(stdout, "%d\n", keys.size());
      }
    }
    keys.insert(key);
    int ret_put = hm_->Put(key, key);
    //fprintf(stderr, "Put() [%s]\n", key.c_str());
    if (ret_put != 0) {
      fprintf(stderr, "Put() error\n");
    }
  }
}


void TestCase::RemoveEntries(uint32_t num_items, std::set<std::string>& keys) {
  for (uint32_t index_del = 0; index_del < num_items; index_del++) {
    uint64_t r = rand();
    uint64_t offset = r % keys.size();
    //printf("delete index %d -- offset %llu -- rand %llu\n", index_del, offset, r);
    std::set<std::string>::iterator it(keys.begin());
    std::advance(it, offset);
    //fprintf(stdout, "str: %s\n", (*it).c_str());
    //key = buffer;
    int ret_remove = hm_->Remove(*it);
    //fprintf(stderr, "Remove() [%s]\n", it->c_str());
    if (ret_remove != 0) fprintf(stderr, "Error while removing\n");
    keys.erase(it);
  }
}




void BatchTestCase::run() {
  std::set<std::string> keys;
  std::string key;
  int key_size = 16;
  char buffer[key_size + 1];
  buffer[key_size] = '\0';
  char filename[1024];
  char alpha[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  uint32_t num_items;
  uint32_t num_items_big = (uint32_t)((double)num_buckets_ * load_factor_max_);
  uint32_t num_items_small = (uint32_t)((double)num_buckets_ * load_factor_remove_);
  fprintf(stdout, "num_items %u %u\n", num_items_big, num_items_small);

  std::string testcase = "batch";
  std::string directory = "results";
  if (exists_or_mkdir(directory.c_str()) != 0) {
    fprintf(stderr, "Could not create directory [%s]\n", directory.c_str());
    exit(1);
  }

  char pt_string[1024];
  sprintf(pt_string, "lfm%.2f-lfr%.2f", load_factor_max_, load_factor_remove_);

  char pt_json[1024];
  sprintf(pt_json, "{\"load_factor_max\": %.2f, \"load_factor_remove\": %.2f}", load_factor_max_, load_factor_remove_);


  std::set<std::string>::iterator it_find;
  for (int i = 0; i < 10; i++) {


    
    num_items = num_items_big;
    srand(i);
    keys.clear();
    hm_->Open();


    std::map<std::string, std::string> metadata;
    hm_->GetMetadata(metadata);

    char directory_sub_buffer[2048];
    sprintf(directory_sub_buffer, "%s/%s-%s-%s", directory.c_str(), testcase.c_str(), metadata["name"].c_str(), metadata["parameters_hashmap_string"].c_str());
    std::string directory_sub(directory_sub_buffer);
    if (exists_or_mkdir(directory_sub.c_str()) != 0) {
      fprintf(stderr, "Could not create directory [%s]\n", directory_sub.c_str());
      exit(1);
    }



    for (int cycle = 0; cycle < 50; cycle++) {
      fprintf(stderr, "instance %d cycle %d\n", i, cycle);
      for (uint32_t j = 0; j < num_items; j++) {
        bool is_valid = false;
        while (!is_valid) {
          for (int k = 0; k < key_size; k++) {
            buffer[k] = alpha[rand() % 62];
          }
          key = buffer;
          it_find = keys.find(key);
          if (it_find == keys.end()) {
            is_valid = true;
          } else {
            //fprintf(stdout, "%s\n", key.c_str());
            //fprintf(stdout, "%d\n", keys.size());
          }
        }
        keys.insert(key);
        int ret_put = hm_->Put(key, key);
        //fprintf(stderr, "Put() [%s]\n", key.c_str());
        if (ret_put != 0) {
          fprintf(stderr, "Put() error\n");
        }
      }
      printf("keys insert %zu\n", keys.size());

      hm_->monitoring_->SetTestcase(testcase);
      hm_->monitoring_->SetInstance(i);
      hm_->monitoring_->SetCycle(cycle);
      hm_->monitoring_->SetParametersTestcaseString(pt_string);
      hm_->monitoring_->SetParametersTestcaseJson(pt_json);


      sprintf(filename, "%s/%s-%s-%s--%s-density--instance%05d-cycle%04d.json", directory_sub.c_str(), testcase.c_str(), metadata["name"].c_str(), metadata["parameters_hashmap_string"].c_str(), pt_string, i, cycle);
      hm_->monitoring_->PrintDensity(filename);

      //fprintf(stderr, "PrintDensity() out\n");
      //sprintf(filename, "%s/%s-%s-num_scanned_blocks-%05d-%04d.json", testcase.c_str(), testcase.c_str(), metadata["name"].c_str(), i, cycle);
      //hm_->monitoring_->PrintNumScannedBlocks(filename);

      sprintf(filename, "%s/%s-%s-%s--%s-psl--instance%05d-cycle%04d.json", directory_sub.c_str(), testcase.c_str(), metadata["name"].c_str(), metadata["parameters_hashmap_string"].c_str(), pt_string, i, cycle);
      fprintf(stderr, "filename psl %s\n", filename);
      hm_->monitoring_->PrintProbingSequenceLengthSearch(filename);
      
      for (uint32_t index_del = 0; index_del < num_items_small; index_del++) {
        uint64_t r = rand();
        uint64_t offset = r % keys.size();
        //printf("delete index %d -- offset %llu -- rand %llu\n", index_del, offset, r);
        std::set<std::string>::iterator it(keys.begin());
        std::advance(it, offset);
        //fprintf(stdout, "str: %s\n", (*it).c_str());
        //key = buffer;
        int ret_remove = hm_->Remove(*it);
        //fprintf(stderr, "Remove() [%s]\n", it->c_str());
        if (ret_remove != 0) fprintf(stderr, "Error while removing\n");
        keys.erase(it);
      }
      printf("keys erase %zu\n", keys.size());
      num_items = num_items_small;
    }

    fprintf(stderr, "close\n");
    hm_->Close();
    fprintf(stderr, "ok\n");
  }

  // testcase-algo-metric-runnumber-step.json
  // batch50-shadow-density-00001-0001.json
  //hm_->monitoring_->PrintDensity("density.json");
  //hm_->monitoring_->PrintNumScannedBlocks("num_scanned_blocks.json");
}




void RippleTestCase::run() {
  std::set<std::string> keys;
  std::string key;
  int key_size = 16;
  char buffer[key_size + 1];
  buffer[key_size] = '\0';
  char filename[1024];
  char alpha[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  uint32_t num_items;
  uint32_t num_items_big = (uint32_t)((double)num_buckets_ * load_factor_max_);
  uint32_t num_items_small = (uint32_t)((double)num_buckets_ * load_factor_remove_);
  fprintf(stdout, "num_items %u %u\n", num_items_big, num_items_small);

  std::string testcase = "ripple";
  std::string directory = "results";
  if (exists_or_mkdir(directory.c_str()) != 0) {
    fprintf(stderr, "Could not create directory [%s]\n", testcase.c_str());
    exit(1);
  }

  char pt_string[1024];
  sprintf(pt_string, "lfm%.2f-lfr%.2f", load_factor_max_, load_factor_remove_);

  char pt_json[1024];
  sprintf(pt_json, "{\"load_factor_max\": %.2f, \"load_factor_remove\": %.2f}", load_factor_max_, load_factor_remove_);

  std::set<std::string>::iterator it_find;
  for (int i = 0; i < 10; i++) {
    num_items = num_items_big;
    srand(i);
    keys.clear();
    hm_->Open();
    for (int cycle = 0; cycle < 50; cycle++) {
      fprintf(stderr, "instance %d cycle %d\n", i, cycle);
      for (uint32_t j = 0; j < num_items; j++) {
        bool is_valid = false;
        while (!is_valid) {
          for (int k = 0; k < key_size; k++) {
            buffer[k] = alpha[rand() % 62];
          }
          key = buffer;
          it_find = keys.find(key);
          if (it_find == keys.end()) {
            is_valid = true;
          } else {
            //fprintf(stdout, "%s\n", key.c_str());
            //fprintf(stdout, "%d\n", keys.size());
          }
        }
        keys.insert(key);
        int ret_put = hm_->Put(key, key);
        //fprintf(stderr, "Put() [%s]\n", key.c_str());
        if (ret_put != 0) {
          fprintf(stderr, "Put() error\n");
        }

        if (cycle > 0) {
          uint64_t r = rand();
          uint64_t offset = r % keys.size();
          //printf("delete index %d -- offset %llu -- rand %llu\n", index_del, offset, r);
          std::set<std::string>::iterator it(keys.begin());
          std::advance(it, offset);
          //fprintf(stdout, "str: %s\n", (*it).c_str());
          //key = buffer;
          int ret_remove = hm_->Remove(*it);
          //fprintf(stderr, "Remove() [%s]\n", it->c_str());
          if (ret_remove != 0) fprintf(stderr, "Error while removing\n");
          keys.erase(it);
        }
      }
      printf("keys insert %zu\n", keys.size());

      hm_->monitoring_->SetTestcase(testcase);
      hm_->monitoring_->SetInstance(i);
      hm_->monitoring_->SetCycle(cycle);
      hm_->monitoring_->SetParametersTestcaseString(pt_string);
      hm_->monitoring_->SetParametersTestcaseJson(pt_json);

      std::map<std::string, std::string> metadata;
      hm_->GetMetadata(metadata);
      sprintf(filename, "%s/%s-%s-%s--%s-density--instance%05d-cycle%04d.json", directory.c_str(), testcase.c_str(), metadata["name"].c_str(), metadata["parameters_hashmap_string"].c_str(), pt_string, i, cycle);
      hm_->monitoring_->PrintDensity(filename);

      fprintf(stderr, "PrintDensity() out\n");
      //sprintf(filename, "%s/%s-%s-num_scanned_blocks-%05d-%04d.json", testcase.c_str(), testcase.c_str(), metadata["name"].c_str(), i, cycle);
      //hm->monitoring_->PrintNumScannedBlocks(filename);

      sprintf(filename, "%s/%s-%s-%s--%s-psl--instance%05d-cycle%04d.json", directory.c_str(), testcase.c_str(), metadata["name"].c_str(), metadata["parameters_hashmap_string"].c_str(), pt_string, i, cycle);
      fprintf(stderr, "filename psl %s\n", filename);
      hm_->monitoring_->PrintProbingSequenceLengthSearch(filename);

      num_items = num_items_small;
    }

    fprintf(stderr, "close\n");
    hm_->Close();
    fprintf(stderr, "ok\n");
  }



  // testcase-algo-metric-runnumber-step.json
  // batch50-shadow-density-00001-0001.json
  //hm_->monitoring_->PrintDensity("density.json");
  //hm_->monitoring_->PrintNumScannedBlocks("num_scanned_blocks.json");
}




void LoadingTestCase::run() {
  std::set<std::string> keys;
  std::string key;
  int key_size = 16;
  char buffer[key_size + 1];
  buffer[key_size] = '\0';
  char filename[1024];
  char alpha[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  uint32_t num_items;
  uint32_t num_items_big = num_buckets_;

  std::string testcase = "loading";
  std::string directory = "results";
  if (exists_or_mkdir(directory.c_str()) != 0) {
    fprintf(stderr, "Could not create directory [%s]\n", testcase.c_str());
    exit(1);
  }

  char pt_string[1024];
  sprintf(pt_string, "%s", "");

  char pt_json[1024];
  sprintf(pt_json, "{}");

  num_items = num_items_big / 50;
  std::set<std::string>::iterator it_find;
  for (int i = 0; i < 10; i++) {
    srand(i);
    keys.clear();
    hm_->Open();

    std::map<std::string, std::string> metadata;
    hm_->GetMetadata(metadata);

    char directory_sub_buffer[2048];
    sprintf(directory_sub_buffer, "%s/%s-%s-%s", directory.c_str(), testcase.c_str(), metadata["name"].c_str(), metadata["parameters_hashmap_string"].c_str());
    std::string directory_sub(directory_sub_buffer);
    if (exists_or_mkdir(directory_sub.c_str()) != 0) {
      fprintf(stderr, "Could not create directory [%s]\n", directory_sub.c_str());
      exit(1);
    }

    for (int cycle = 0; cycle < 50; cycle++) {
      fprintf(stderr, "instance %d cycle %d\n", i, cycle);
      for (uint32_t j = 0; j < num_items; j++) {
        bool is_valid = false;
        while (!is_valid) {
          for (int k = 0; k < key_size; k++) {
            buffer[k] = alpha[rand() % 62];
          }
          key = buffer;
          it_find = keys.find(key);
          if (it_find == keys.end()) {
            is_valid = true;
          } else {
            //fprintf(stdout, "%s\n", key.c_str());
            //fprintf(stdout, "%d\n", keys.size());
          }
        }
        keys.insert(key);
        int ret_put = hm_->Put(key, key);
        //fprintf(stderr, "Put() [%s]\n", key.c_str());
        if (ret_put != 0) {
          fprintf(stderr, "Put() error\n");
        }
      }
      printf("keys insert %zu\n", keys.size());

      hm_->monitoring_->SetTestcase(testcase);
      hm_->monitoring_->SetInstance(i);
      hm_->monitoring_->SetCycle(cycle);
      hm_->monitoring_->SetParametersTestcaseString(pt_string);
      hm_->monitoring_->SetParametersTestcaseJson(pt_json);

      sprintf(filename,
              "%s/%s-%s-%s--%s-density--instance%05d-cycle%04d.json",
              directory_sub.c_str(),
              testcase.c_str(),
              metadata["name"].c_str(),
              metadata["parameters_hashmap_string"].c_str(),
              pt_string,
              i,
              cycle);
      hm_->monitoring_->PrintDensity(filename);

      fprintf(stderr, "PrintDensity() out\n");
      //sprintf(filename, "%s/%s-%s-num_scanned_blocks-%05d-%04d.json", testcase.c_str(), testcase.c_str(), metadata["name"].c_str(), i, cycle);
      //hm->monitoring_->PrintNumScannedBlocks(filename);

      sprintf(filename,
              "%s/%s-%s-%s--%s-psl--instance%05d-cycle%04d.json",
              directory_sub.c_str(),
              testcase.c_str(),
              metadata["name"].c_str(),
              metadata["parameters_hashmap_string"].c_str(),
              pt_string,
              i,
              cycle);
      fprintf(stderr, "filename psl %s\n", filename);
      hm_->monitoring_->PrintProbingSequenceLengthSearch(filename);

      sprintf(filename,
              "%s/%s-%s-%s--%s-blocks--instance%05d-cycle%04d.json",
              directory_sub.c_str(),
              testcase.c_str(),
              metadata["name"].c_str(),
              metadata["parameters_hashmap_string"].c_str(),
              pt_string,
              i,
              cycle);
      hm_->monitoring_->PrintNumScannedBlocks(filename);

      sprintf(filename,
              "%s/%s-%s-%s--%s-secondary--instance%05d-cycle%04d.json",
              directory_sub.c_str(),
              testcase.c_str(),
              metadata["name"].c_str(),
              metadata["parameters_hashmap_string"].c_str(),
              pt_string,
              i,
              cycle);
      hm_->monitoring_->PrintNumSecondaryAccesses(filename);
    }

    fprintf(stderr, "close\n");
    hm_->Close();
    fprintf(stderr, "ok\n");
  }



  // testcase-algo-metric-runnumber-step.json
  // batch50-shadow-density-00001-0001.json
  //hm_->monitoring_->PrintDensity("density.json");
  //hm_->monitoring_->PrintNumScannedBlocks("num_scanned_blocks.json");
}





};

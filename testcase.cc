#include "testcase.h"


// TODO: Factorize as much as possible across the test cases.

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

std::string concatenate(std::string const& str, int i)
{
    std::stringstream s;
    s << str << i;
    return s.str();
}

void TestCase::InsertEntries(uint32_t num_items, std::set<std::string>& keys) {
  std::string key;
  std::set<std::string>::iterator it_find;
  std::string value_dummy;
  static uint64_t key_id_current = 0;
  // NOTE: If ever using this method, remember to reset key_id_current between
  //       instances.

  for (uint32_t j = 0; j < num_items; j++) {
    key_id_current += 1 + rand() % 32;
    key = concatenate( "key", key_id_current );
    it_find = keys.find(key);
    if (it_find != keys.end()) {
      fprintf(stderr, "Error: key already in the hash table, this should not happen\n");
    }

    keys.insert(key);
    int ret_get = hm_->Get(key, &value_dummy);
    if (ret_get != 1) {
      fprintf(stderr, "Get() error\n");
    }
    int ret_put = hm_->Put(key, key);
    if (ret_put != 0) {
      fprintf(stderr, "Put() error\n");
    }
  }
}


void TestCase::RemoveEntries(uint32_t num_items, std::set<std::string>& keys) {
  for (uint32_t index_del = 0; index_del < num_items; index_del++) {
    uint64_t r = rand();
    uint64_t offset = r % keys.size();
    //printf("delete index %d -- offset %" PRIu64 " -- rand %" PRIu64 "\n", index_del, offset, r);
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
  char filename[1024];
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
  for (int i = 0; i < 50; i++) {
    
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

    std::string value_dummy;
    uint64_t key_id_current = 0;

    for (int cycle = 0; cycle < 50; cycle++) {
      fprintf(stderr, "instance %d cycle %d\n", i, cycle);
      bool has_error_on_put = false;
      for (uint32_t j = 0; j < num_items; j++) {
        key_id_current += 1 + rand() % 32;
        key = concatenate( "key", key_id_current );
        it_find = keys.find(key);
        if (it_find != keys.end()) {
          fprintf(stderr, "Error: key already in the hash table, this should not happen\n");
        }

        keys.insert(key);
        int ret_get = hm_->Get(key, &value_dummy);
        if (ret_get != 1) {
          fprintf(stderr, "Get() error\n");
        }
        int ret_put = hm_->Put(key, key);
        //fprintf(stderr, "Put() [%s]\n", key.c_str());
        if (ret_put != 0) {
          fprintf(stderr, "Put() error\n");
          // break on error
          has_error_on_put = true;
          break;
        }
      }
      printf("keys insert %zu\n", keys.size());
      if (has_error_on_put) {
        hm_->monitoring_->ResetDFB();
        hm_->monitoring_->ResetAlignedDFB();
        hm_->monitoring_->ResetNumberOfSwaps();
        hm_->monitoring_->ResetDMB();
        hm_->monitoring_->ResetAlignedDMB();
        hm_->monitoring_->ResetDSB();
        hm_->monitoring_->ResetAlignedDSB();
        num_items = num_items_small;
        break;
      }

      hm_->monitoring_->SetTestcase(testcase);
      hm_->monitoring_->SetInstance(i);
      hm_->monitoring_->SetCycle(cycle);
      hm_->monitoring_->SetParametersTestcaseString(pt_string);
      hm_->monitoring_->SetParametersTestcaseJson(pt_json);

      sprintf(filename, "%s/%s-%s-%s--%s-dib--instance%05d-cycle%04d.json", directory_sub.c_str(), testcase.c_str(), metadata["name"].c_str(), metadata["parameters_hashmap_string"].c_str(), pt_string, i, cycle);
      fprintf(stderr, "filename dib %s\n", filename);
      hm_->monitoring_->PrintDIB(filename);


      sprintf(filename,
              "%s/%s-%s-%s--%s-adib--instance%05d-cycle%04d.json",
              directory_sub.c_str(),
              testcase.c_str(),
              metadata["name"].c_str(),
              metadata["parameters_hashmap_string"].c_str(),
              pt_string,
              i,
              cycle);
      hm_->monitoring_->PrintNumScannedBlocks(filename);

      sprintf(filename,
              "%s/%s-%s-%s--%s-dfb--instance%05d-cycle%04d.json",
              directory_sub.c_str(),
              testcase.c_str(),
              metadata["name"].c_str(),
              metadata["parameters_hashmap_string"].c_str(),
              pt_string,
              i,
              cycle);
      hm_->monitoring_->PrintDFB(filename);
      hm_->monitoring_->ResetDFB();

      sprintf(filename,
              "%s/%s-%s-%s--%s-adfb--instance%05d-cycle%04d.json",
              directory_sub.c_str(),
              testcase.c_str(),
              metadata["name"].c_str(),
              metadata["parameters_hashmap_string"].c_str(),
              pt_string,
              i,
              cycle);
      hm_->monitoring_->PrintAlignedDFB(filename);
      hm_->monitoring_->ResetAlignedDFB();

      sprintf(filename,
              "%s/%s-%s-%s--%s-swap--instance%05d-cycle%04d.json",
              directory_sub.c_str(),
              testcase.c_str(),
              metadata["name"].c_str(),
              metadata["parameters_hashmap_string"].c_str(),
              pt_string,
              i,
              cycle);
      hm_->monitoring_->PrintNumberOfSwaps(filename);
      hm_->monitoring_->ResetNumberOfSwaps();

      sprintf(filename,
              "%s/%s-%s-%s--%s-dmb--instance%05d-cycle%04d.json",
              directory_sub.c_str(),
              testcase.c_str(),
              metadata["name"].c_str(),
              metadata["parameters_hashmap_string"].c_str(),
              pt_string,
              i,
              cycle);
      hm_->monitoring_->PrintDMB(filename);
      hm_->monitoring_->ResetDMB();

      sprintf(filename,
              "%s/%s-%s-%s--%s-admb--instance%05d-cycle%04d.json",
              directory_sub.c_str(),
              testcase.c_str(),
              metadata["name"].c_str(),
              metadata["parameters_hashmap_string"].c_str(),
              pt_string,
              i,
              cycle);
      hm_->monitoring_->PrintAlignedDMB(filename);
      hm_->monitoring_->ResetAlignedDMB();

      sprintf(filename,
              "%s/%s-%s-%s--%s-dsb--instance%05d-cycle%04d.json",
              directory_sub.c_str(),
              testcase.c_str(),
              metadata["name"].c_str(),
              metadata["parameters_hashmap_string"].c_str(),
              pt_string,
              i,
              cycle);
      hm_->monitoring_->PrintDSB(filename);
      hm_->monitoring_->ResetDSB();

      sprintf(filename,
              "%s/%s-%s-%s--%s-adsb--instance%05d-cycle%04d.json",
              directory_sub.c_str(),
              testcase.c_str(),
              metadata["name"].c_str(),
              metadata["parameters_hashmap_string"].c_str(),
              pt_string,
              i,
              cycle);
      hm_->monitoring_->PrintAlignedDSB(filename);
      hm_->monitoring_->ResetAlignedDSB();



      for (uint32_t index_del = 0; index_del < num_items_small; index_del++) {
        uint64_t r = rand();
        uint64_t offset = r % keys.size();
        //printf("delete index %d -- offset %" PRIu64 " -- rand %" PRIu64 "\n", index_del, offset, r);
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
}




void RippleTestCase::run() {
  std::set<std::string> keys;
  std::string key;
  char filename[1024];
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
  for (int i = 0; i < 50; i++) {
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

    std::string value_dummy;
    uint64_t key_id_current = 0;

    for (int cycle = 0; cycle < 50; cycle++) {
      fprintf(stderr, "instance %d cycle %d\n", i, cycle);
      bool has_error_on_put = false;
      for (uint32_t j = 0; j < num_items; j++) {
        key_id_current += 1 + rand() % 32;
        key = concatenate( "key", key_id_current );
        it_find = keys.find(key);
        if (it_find != keys.end()) {
          fprintf(stderr, "Error: key already in the hash table, this should not happen\n");
        }

        keys.insert(key);
        int ret_get = hm_->Get(key, &value_dummy);
        if (ret_get != 1) {
          fprintf(stderr, "Get() error\n");
        }
        int ret_put = hm_->Put(key, key);
        if (ret_put != 0) {
          fprintf(stderr, "Put() error\n");
          has_error_on_put = true;
        }

        if (cycle > 0) {
          uint64_t r = rand();
          uint64_t offset = r % keys.size();
          //printf("delete index %d -- offset %" PRIu64 " -- rand %" PRIu64 "\n", index_del, offset, r);
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
      if (has_error_on_put) {
        hm_->monitoring_->ResetDFB();
        hm_->monitoring_->ResetAlignedDFB();
        hm_->monitoring_->ResetNumberOfSwaps();
        hm_->monitoring_->ResetDMB();
        hm_->monitoring_->ResetAlignedDMB();
        hm_->monitoring_->ResetDSB();
        hm_->monitoring_->ResetAlignedDSB();
        num_items = num_items_small;
        break;
      }

      hm_->monitoring_->SetTestcase(testcase);
      hm_->monitoring_->SetInstance(i);
      hm_->monitoring_->SetCycle(cycle);
      hm_->monitoring_->SetParametersTestcaseString(pt_string);
      hm_->monitoring_->SetParametersTestcaseJson(pt_json);

      sprintf(filename, "%s/%s-%s-%s--%s-dib--instance%05d-cycle%04d.json", directory_sub.c_str(), testcase.c_str(), metadata["name"].c_str(), metadata["parameters_hashmap_string"].c_str(), pt_string, i, cycle);
      fprintf(stderr, "filename dib %s\n", filename);
      hm_->monitoring_->PrintDIB(filename);

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
              "%s/%s-%s-%s--%s-dfb--instance%05d-cycle%04d.json",
              directory_sub.c_str(),
              testcase.c_str(),
              metadata["name"].c_str(),
              metadata["parameters_hashmap_string"].c_str(),
              pt_string,
              i,
              cycle);
      hm_->monitoring_->PrintDFB(filename);
      hm_->monitoring_->ResetDFB();

      sprintf(filename,
              "%s/%s-%s-%s--%s-adfb--instance%05d-cycle%04d.json",
              directory_sub.c_str(),
              testcase.c_str(),
              metadata["name"].c_str(),
              metadata["parameters_hashmap_string"].c_str(),
              pt_string,
              i,
              cycle);
      hm_->monitoring_->PrintAlignedDFB(filename);
      hm_->monitoring_->ResetAlignedDFB();

      sprintf(filename,
              "%s/%s-%s-%s--%s-swap--instance%05d-cycle%04d.json",
              directory_sub.c_str(),
              testcase.c_str(),
              metadata["name"].c_str(),
              metadata["parameters_hashmap_string"].c_str(),
              pt_string,
              i,
              cycle);
      hm_->monitoring_->PrintNumberOfSwaps(filename);
      hm_->monitoring_->ResetNumberOfSwaps();

      sprintf(filename,
              "%s/%s-%s-%s--%s-dmb--instance%05d-cycle%04d.json",
              directory_sub.c_str(),
              testcase.c_str(),
              metadata["name"].c_str(),
              metadata["parameters_hashmap_string"].c_str(),
              pt_string,
              i,
              cycle);
      hm_->monitoring_->PrintDMB(filename);
      hm_->monitoring_->ResetDMB();

      sprintf(filename,
              "%s/%s-%s-%s--%s-admb--instance%05d-cycle%04d.json",
              directory_sub.c_str(),
              testcase.c_str(),
              metadata["name"].c_str(),
              metadata["parameters_hashmap_string"].c_str(),
              pt_string,
              i,
              cycle);
      hm_->monitoring_->PrintAlignedDMB(filename);
      hm_->monitoring_->ResetAlignedDMB();

      sprintf(filename,
              "%s/%s-%s-%s--%s-dsb--instance%05d-cycle%04d.json",
              directory_sub.c_str(),
              testcase.c_str(),
              metadata["name"].c_str(),
              metadata["parameters_hashmap_string"].c_str(),
              pt_string,
              i,
              cycle);
      hm_->monitoring_->PrintDSB(filename);
      hm_->monitoring_->ResetDSB();

      sprintf(filename,
              "%s/%s-%s-%s--%s-adsb--instance%05d-cycle%04d.json",
              directory_sub.c_str(),
              testcase.c_str(),
              metadata["name"].c_str(),
              metadata["parameters_hashmap_string"].c_str(),
              pt_string,
              i,
              cycle);
      hm_->monitoring_->PrintAlignedDSB(filename);
      hm_->monitoring_->ResetAlignedDSB();





      

      num_items = num_items_small;
    }

    fprintf(stderr, "close\n");
    hm_->Close();
    fprintf(stderr, "ok\n");
  }
}




void LoadingTestCase::run() {
  std::set<std::string> keys;
  std::string key;
  char filename[1024];
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
  for (int i = 0; i < 50; i++) {
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

    std::string value_dummy;
    uint64_t key_id_current = 0;

    for (int cycle = 0; cycle < 50; cycle++) {
      fprintf(stderr, "instance %d cycle %d\n", i, cycle);
      bool has_error_on_put = false;
      for (uint32_t j = 0; j < num_items; j++) {
        key_id_current += 1 + rand() % 32;
        key = concatenate( "key", key_id_current );
        it_find = keys.find(key);
        if (it_find != keys.end()) {
          fprintf(stderr, "Error: key already in the hash table, this should not happen\n");
        }

        int ret_get = hm_->Get(key, &value_dummy);
        if (ret_get != 1) {
          fprintf(stderr, "Get() error\n");
        }
        int ret_put = hm_->Put(key, key);
        //fprintf(stderr, "Put() [%s]\n", key.c_str());
        if (ret_put != 0) {
          fprintf(stderr, "Put() error\n");
          // break on error
          has_error_on_put = true;
          break;
        }
        keys.insert(key);
      }
      printf("keys insert %zu\n", keys.size());
      if (has_error_on_put) {
        hm_->monitoring_->ResetDFB();
        hm_->monitoring_->ResetAlignedDFB();
        hm_->monitoring_->ResetNumberOfSwaps();
        hm_->monitoring_->ResetDMB();
        hm_->monitoring_->ResetAlignedDMB();
        hm_->monitoring_->ResetDSB();
        hm_->monitoring_->ResetAlignedDSB();
        break;
      }

      hm_->monitoring_->SetTestcase(testcase);
      hm_->monitoring_->SetInstance(i);
      hm_->monitoring_->SetCycle(cycle);
      hm_->monitoring_->SetParametersTestcaseString(pt_string);
      hm_->monitoring_->SetParametersTestcaseJson(pt_json);

      sprintf(filename,
              "%s/%s-%s-%s--%s-dib--instance%05d-cycle%04d.json",
              directory_sub.c_str(),
              testcase.c_str(),
              metadata["name"].c_str(),
              metadata["parameters_hashmap_string"].c_str(),
              pt_string,
              i,
              cycle);
      fprintf(stderr, "filename dib %s\n", filename);
      hm_->monitoring_->PrintDIB(filename);

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
              "%s/%s-%s-%s--%s-dfb--instance%05d-cycle%04d.json",
              directory_sub.c_str(),
              testcase.c_str(),
              metadata["name"].c_str(),
              metadata["parameters_hashmap_string"].c_str(),
              pt_string,
              i,
              cycle);
      hm_->monitoring_->PrintDFB(filename);
      hm_->monitoring_->ResetDFB();

      sprintf(filename,
              "%s/%s-%s-%s--%s-adfb--instance%05d-cycle%04d.json",
              directory_sub.c_str(),
              testcase.c_str(),
              metadata["name"].c_str(),
              metadata["parameters_hashmap_string"].c_str(),
              pt_string,
              i,
              cycle);
      hm_->monitoring_->PrintAlignedDFB(filename);
      hm_->monitoring_->ResetAlignedDFB();


      sprintf(filename,
              "%s/%s-%s-%s--%s-swap--instance%05d-cycle%04d.json",
              directory_sub.c_str(),
              testcase.c_str(),
              metadata["name"].c_str(),
              metadata["parameters_hashmap_string"].c_str(),
              pt_string,
              i,
              cycle);
      hm_->monitoring_->PrintNumberOfSwaps(filename);
      hm_->monitoring_->ResetNumberOfSwaps();

      sprintf(filename,
              "%s/%s-%s-%s--%s-dmb--instance%05d-cycle%04d.json",
              directory_sub.c_str(),
              testcase.c_str(),
              metadata["name"].c_str(),
              metadata["parameters_hashmap_string"].c_str(),
              pt_string,
              i,
              cycle);
      hm_->monitoring_->PrintDMB(filename);
      hm_->monitoring_->ResetDMB();

      sprintf(filename,
              "%s/%s-%s-%s--%s-admb--instance%05d-cycle%04d.json",
              directory_sub.c_str(),
              testcase.c_str(),
              metadata["name"].c_str(),
              metadata["parameters_hashmap_string"].c_str(),
              pt_string,
              i,
              cycle);
      hm_->monitoring_->PrintAlignedDMB(filename);
      hm_->monitoring_->ResetAlignedDMB();

      sprintf(filename,
              "%s/%s-%s-%s--%s-dsb--instance%05d-cycle%04d.json",
              directory_sub.c_str(),
              testcase.c_str(),
              metadata["name"].c_str(),
              metadata["parameters_hashmap_string"].c_str(),
              pt_string,
              i,
              cycle);
      hm_->monitoring_->PrintDSB(filename);
      hm_->monitoring_->ResetDSB();

      sprintf(filename,
              "%s/%s-%s-%s--%s-adsb--instance%05d-cycle%04d.json",
              directory_sub.c_str(),
              testcase.c_str(),
              metadata["name"].c_str(),
              metadata["parameters_hashmap_string"].c_str(),
              pt_string,
              i,
              cycle);
      hm_->monitoring_->PrintAlignedDSB(filename);
      hm_->monitoring_->ResetAlignedDSB();





    }

    fprintf(stderr, "close\n");
    hm_->Close();
    fprintf(stderr, "ok\n");
  }
}





};

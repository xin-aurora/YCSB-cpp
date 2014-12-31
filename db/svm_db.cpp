//
//  svm_db.cpp
//  YCSB-C
//
//  Created by Jinglei Ren on 12/27/14.
//  Copyright (c) 2014 Jinglei Ren <jinglei@ren.systems>.
//

#include "db/svm_db.h"

#include <vector>
#include "sitevm/sitevm.h"
#include "slib/mem_allocator.h"
#include "lib/trans_def.h"

namespace ycsbc {

SVMDB::SVMDB() : HashtableDB(NULL) {
  int err = sitevm_init();
  assert(!err);
  svm_ = sitevm_seg_create(NULL, SVM_SIZE);
  sitevm_malloc::init_sitevm_malloc(svm_);
  err = sitevm_enter();
  assert(!err);
  err = sitevm_open_and_update(svm_);
  assert(!err);
  key_table_ = NEW(vmp::SVMHashtable<HashtableDB::FieldHashtable *>);
  err = sitevm_commit_and_update(svm_);
  assert(!err);
}

void SVMDB::Init() {
  int err = sitevm_enter();
  assert(!err);
  err = sitevm_open_and_update(svm_);
  assert(!err);
}

void SVMDB::Close() {
  sitevm_exit();
}

SVMDB::~SVMDB() {
  std::vector<KeyHashtable::KVPair> key_pairs = key_table_->Entries();
  for (auto &key_pair : key_pairs) {
    DeleteFieldHashtable(key_pair.second);
  }
  DELETE(key_table_);
}

int SVMDB::Read(const std::string &table, const std::string &key,
    const std::vector<std::string> *fields, std::vector<KVPair> &result) {
  int ret;
  TRANS_BEGIN {
    ret = HashtableDB::Read(table, key, fields, result);
  } TRANS_END(svm_);
  return ret;
}

int SVMDB::Scan(const std::string &table, const std::string &key,
    int len, const std::vector<std::string> *fields,
    std::vector<std::vector<KVPair>> &result) {
  int ret;
  TRANS_BEGIN {
    ret = HashtableDB::Scan(table, key, len, fields, result);
  } TRANS_END(svm_);
  return ret;
}

int SVMDB::Update(const std::string &table, const std::string &key,
    std::vector<KVPair> &values) {
  int ret;
  TRANS_BEGIN {
    ret = HashtableDB::Update(table, key, values);
  } TRANS_END(svm_);
  return ret;
}

int SVMDB::Insert(const std::string &table, const std::string &key,
    std::vector<KVPair> &values) {
  int ret;
  TRANS_BEGIN {
    ret = HashtableDB::Insert(table, key, values);
  } TRANS_END(svm_);
  return ret;
}

int SVMDB::Delete(const std::string &table, const std::string &key) {
  int ret;
  TRANS_BEGIN {
    ret = HashtableDB::Delete(table, key);
  } TRANS_END(svm_);
  return ret;
}

HashtableDB::FieldHashtable *SVMDB::NewFieldHashtable() {
  return NEW(vmp::SVMHashtable<const char *>);
}

void SVMDB::DeleteFieldHashtable(HashtableDB::FieldHashtable *table) {
  std::vector<FieldHashtable::KVPair> pairs = table->Entries();
  for (auto &pair : pairs) {
    DeleteString(pair.second);
  }
  DELETE(table);
}

const char *SVMDB::CopyString(const std::string &str) {
  char *value = (char *)MALLOC(str.length() + 1);
  strcpy(value, str.c_str());
  return value;
}

void SVMDB::DeleteString(const char *str) {
  FREE(str, strlen(str) + 1);
}

} // ycsbc

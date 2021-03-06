/* -*- mode: c++; indent-tabs-mode: nil -*- */
#ifndef SQLITE_DB_H
#define SQLITE_DB_H

#include <mutex>
#include <string>

#include "base/macros.h"
#include "log/database.h"

struct sqlite3;

namespace cert_trans {


template <class Logged>
class SQLiteDB : public Database<Logged> {
 public:
  explicit SQLiteDB(const std::string& dbfile);

  ~SQLiteDB();

  typedef typename Database<Logged>::WriteResult WriteResult;
  typedef typename Database<Logged>::LookupResult LookupResult;

  WriteResult CreateSequencedEntry_(const Logged& logged) override;

  LookupResult LookupByHash(const std::string& hash,
                            Logged* result) const override;

  LookupResult LookupByIndex(int64_t sequence_number,
                             Logged* result) const override;

  std::unique_ptr<typename Database<Logged>::Iterator> ScanEntries(
      int64_t start_index) const override;

  WriteResult WriteTreeHead_(const ct::SignedTreeHead& sth) override;

  LookupResult LatestTreeHead(ct::SignedTreeHead* result) const override;

  int64_t TreeSize() const override;

  void AddNotifySTHCallback(
      const typename Database<Logged>::NotifySTHCallback* callback) override;

  void RemoveNotifySTHCallback(
      const typename Database<Logged>::NotifySTHCallback* callback) override;

  void InitializeNode(const std::string& node_id) override;
  LookupResult NodeId(std::string* node_id) override;

  // Force an STH notification. This is needed only for ct-dns-server,
  // which shares a SQLite database with ct-server, but needs to
  // refresh itself occasionally.
  void ForceNotifySTH();

 private:
  class Iterator;

  LookupResult LookupByIndex(const std::unique_lock<std::mutex>& lock,
                             int64_t sequence_number, Logged* result) const;
  // This finds the next entry with a sequence number equal or greater
  // to the one specified.
  LookupResult LookupNextIndex(const std::unique_lock<std::mutex>& lock,
                               int64_t sequence_number, Logged* result) const;
  LookupResult LatestTreeHeadNoLock(const std::unique_lock<std::mutex>& lock,
                                    ct::SignedTreeHead* result) const;
  LookupResult NodeId(const std::unique_lock<std::mutex>& lock,
                      std::string* node_id);

  void BeginTransaction(const std::unique_lock<std::mutex>& lock);

  void EndTransaction(const std::unique_lock<std::mutex>& lock);

  void MaybeStartNewTransaction(const std::unique_lock<std::mutex>& lock);

  mutable std::mutex lock_;
  sqlite3* const db_;
  // This is marked mutable, as it is a lazily updated cache updated
  // from some of the getters.
  mutable int64_t tree_size_;
  DatabaseNotifierHelper callbacks_;
  int64_t transaction_size_;
  bool in_transaction_;

  DISALLOW_COPY_AND_ASSIGN(SQLiteDB);
};


}  // namespace cert_trans

#endif

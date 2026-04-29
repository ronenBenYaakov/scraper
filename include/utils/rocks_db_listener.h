#pragma once

#include <rocksdb/db.h>
#include <rocksdb/options.h>
#include <string>
#include <functional>
#include <vector>
#include <mutex>

class RocksDBListener {
public:
    enum class EventType {
        PUT,
        DELETE
    };

    struct Handler {
        std::function<void(const std::string&, const std::string&)> onPut;
        std::function<void(const std::string&)> onDelete;
    };

public:
    explicit RocksDBListener(rocksdb::DB* db)
        : db_(db) {}

    void setHandler(const Handler& h) {
        handler_ = h;
    }

    void put(const std::string& key, const std::string& value) {
        std::lock_guard<std::mutex> lock(mu_);

        db_->Put(rocksdb::WriteOptions(), key, value);

        if (handler_.onPut)
            handler_.onPut(key, value);
    }

    void remove(const std::string& key) {
        std::lock_guard<std::mutex> lock(mu_);

        db_->Delete(rocksdb::WriteOptions(), key);

        if (handler_.onDelete)
            handler_.onDelete(key);
    }

    // Optional raw access if needed
    rocksdb::DB* raw() {
        return db_;
    }

private:
    rocksdb::DB* db_;
    Handler handler_;
    std::mutex mu_;
};
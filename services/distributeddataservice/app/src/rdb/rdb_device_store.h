/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef RDB_DEVICE_STORE_H
#define RDB_DEVICE_STORE_H

#include "rdb_store.h"
#include "rdb_store_stub.h"

namespace DistributedDB {
class RelationalStoreManager;
class RelationalStoreDelegate;
}

namespace OHOS::DistributedKv {
class RdbDeviceStore : public RdbStore {
public:
    explicit RdbDeviceStore(const RdbStoreParam& param);
    
    virtual ~RdbDeviceStore() override;
    
    static void Initialize();
    static RdbStore* CreateStore(const RdbStoreParam& param);
    
    /* IPC interface */
    int SetDistributedTables(const std::vector<std::string>& tables) override;
    
    /* RdbStore interface */
    int Init() override;
    
private:
    int CreateMetaData();
    
    DistributedDB::RelationalStoreManager* GetManager();
    
    DistributedDB::RelationalStoreDelegate* GetDelegate();
    
    std::mutex mutex_;
    DistributedDB::RelationalStoreManager* manager_;
    DistributedDB::RelationalStoreDelegate* delegate_;
};
}
#endif

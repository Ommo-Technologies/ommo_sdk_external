/*
 * Copyright 2025 Ommo Technologies, Inc. - All Rights Reserved
 *
 * Unless required by applicable law or agreed to in writing, software
 * is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS
 * OF ANY KIND, either express or implied.
*/
#include "client_context.h"
#include "client_context_impl.h"

#include <string>
#include <memory>
#include <unordered_map>
#include "client_manager.h"
#include "spdlog_logger.h"

namespace ommo::api
{
    ClientContext::ClientContext(const char* server_address) : p_impl_(new ClientContext::impl(server_address)) {}

    ClientContext::~ClientContext()
    {
        p_impl_->Shutdown();
        delete p_impl_;
    }

    void ClientContext::Start()
    {
        p_impl_->Start();
    }

    void ClientContext::Shutdown()
    {
        p_impl_->Shutdown();
    }

    void ClientContext::SetupLogging(const char* file_name)
    {
        SetLogger(std::make_unique<SpdLogLogger>());

        SpdLogConfig cfg;
        if (file_name && *file_name)
        {
            cfg.file_name = file_name;
        }
        ConfigureSpdLog(cfg);
    }

    api::TrackingDevices* ClientContext::GetTrackingDevices()
    {
        return p_impl_->GetTrackingDevices();
    }

    api::HardwareStates* ClientContext::GetHardwareStates()
    {
        return p_impl_->GetHardwareStates();
    }

    bool ClientContext::SetBaseStationMotorRunning(bool active)
    {
        return p_impl_->SetBaseStationMotorRunning(active);
    }

    void ClientContext::RegisterDeviceEventCallback(std::function<void(const api::TrackingDeviceEvent&)> callback_function)
    {
        p_impl_->RegisterDeviceEventCallback(callback_function);
    }

    void ClientContext::ResetDeviceEventCallback()
    {
        p_impl_->ResetDeviceEventCallback();
    }

    void ClientContext::RegisterChannelStateCallback(std::function<void(int)> callback_function)
    {
        p_impl_->RegisterChannelStateCallback(callback_function);
    }

    void ClientContext::ResetChannelStateCallback()
    {
        p_impl_->ResetChannelStateCallback();
    }

    uint32_t ClientContext::RequestDeviceData(api::DataRequest& request)
    {
        return p_impl_->RequestDeviceData(request);
    }

    uint32_t ClientContext::RequestDataFrame(api::DataRequest& request)
    {
        return p_impl_->RequestDataFrame(request);
    }

    uint32_t ClientContext::RequestBaseStationData()
    {
        return p_impl_->RequestBaseStationData();
    }
    void ClientContext::CloseBaseStationDataRequest(uint32_t request_tag)
    {
        return p_impl_->CloseBaseStationDataRequest(request_tag);
    }

    api::BaseStationDataResponse* ClientContext::GetLatestBaseStationData(uint32_t request_tag)
    {
        return p_impl_->GetLatestBaseStationData(request_tag);
    }

    api::BaseStationDataResponse* ClientContext::GetLatestBaseStationData(uint32_t request_tag, int32_t num_packets)
    {
        return p_impl_->GetLatestBaseStationData(request_tag, num_packets);
    }

    api::BaseStationDataResponse* ClientContext::GetBaseStationDataSinceIndex(uint32_t request_tag, int32_t start_index)
    {
        return p_impl_->GetBaseStationDataSinceIndex(request_tag, start_index);
    }

    void ClientContext::CloseRequest(uint32_t request_tag)
    {
        p_impl_->CloseRequest(request_tag);
    }

    api::DeviceIDList* ClientContext::GetAvailableDeviceList(uint32_t request_tag)
    {
        return p_impl_->GetAvailableDeviceList(request_tag);
    }

    api::DataResponse* ClientContext::GetLatestData(uint32_t request_tag, const api::DeviceID& device_id)
    {
        return p_impl_->GetLatestData(request_tag, device_id);
    }

    api::DataResponse* ClientContext::GetLatestData(uint32_t request_tag, const api::DeviceID& device_id, int32_t num_packets)
    {
        return p_impl_->GetLatestData(request_tag, device_id, num_packets);
    }

    api::DataResponse* ClientContext::GetDataSinceIndex(uint32_t request_tag, const api::DeviceID& device_id, int32_t start_index)
    {
        return p_impl_->GetDataSinceIndex(request_tag, device_id, start_index);
    }

    void ClientContext::RegisterTrackingDeviceDataCallback(uint32_t request_tag, std::function<void(const api::TrackingDeviceData&)> callback_function)
    {
        p_impl_->RegisterTrackingDeviceDataCallback(request_tag, callback_function);
    }

    void ClientContext::ResetTrackingDeviceDataCallback(uint32_t request_tag)
    {
        p_impl_->ResetTrackingDeviceDataCallback(request_tag);
    }

    void ClientContext::RegisterDataFrameCallback(uint32_t request_tag, std::function<void(const api::DataFrame&)> callback_function)
    {
        p_impl_->RegisterDataFrameCallback(request_tag, callback_function);
    }

    void ClientContext::ResetDataFrameCallback(uint32_t request_tag)
    {
        p_impl_->ResetDataFrameCallback(request_tag);
    }

    api::WirelessManager* ClientContext::CreateWirelessManager()
    {
        return p_impl_->CreateWirelessManager();
    }

    void ClientContext::DeleteWirelessManager(api::WirelessManager* wireless_manager)
    {
        p_impl_->DeleteWirelessManager(wireless_manager);
    }

    api::DataLogState ClientContext::EnableDataLogging(const char* directory, const char* file_name, bool overwrite)
    {
        std::string directory_str = std::string(directory);
        std::string file_name_str = std::string(file_name);
        return p_impl_->EnableDataLogging(directory_str, file_name_str, overwrite);
    }

    api::DataLogState ClientContext::DisableDataLogging()
    {
        return p_impl_->DisableDataLogging();
    }

    bool ClientContext::SelectReferenceDevice(bool enabled, uint32_t siu_uuid, uint32_t port_num)
    {
        return p_impl_->SelectReferenceDevice(enabled, siu_uuid, port_num);
    }

    ClientContext::impl::impl(const char* server_address)
    {
        std::string address = "localhost:50051";
        if (server_address != nullptr && server_address[0] != '\0')
        {
            address = std::string(server_address);
        }
        client_manager_ = std::make_unique<ommo::ClientManager>(address);
    }

    void ClientContext::impl::Start()
    {
        client_manager_->Start();
    }

    void ClientContext::impl::Shutdown()
    {
        client_manager_->Shutdown();
    }

    api::TrackingDevices* ClientContext::impl::GetTrackingDevices()
    {
        return client_manager_->GetTrackingDevices().release();
    }

    api::HardwareStates* ClientContext::impl::GetHardwareStates()
    {
        return client_manager_->GetHardwareStates().release();
    }

    bool ClientContext::impl::SetBaseStationMotorRunning(bool active)
    {
        return client_manager_->SetBaseStationMotorRunning(active);
    }

    void ClientContext::impl::RegisterDeviceEventCallback(std::function<void(const api::TrackingDeviceEvent&)> callback_function)
    {
        client_manager_->RegisterDeviceEventCallback(callback_function);
    }

    void ClientContext::impl::ResetDeviceEventCallback()
    {
        client_manager_->ResetDeviceEventCallback();
    }

    void ClientContext::impl::RegisterChannelStateCallback(std::function<void(int)> callback_function)
    {
        client_manager_->RegisterChannelStateCallback(callback_function);
    }

    void ClientContext::impl::ResetChannelStateCallback()
    {
        client_manager_->ResetChannelStateCallback();
    }

    uint32_t ClientContext::impl::RequestDeviceData(api::DataRequest& request)
    {
        std::shared_ptr<ommo::DataManager> manager = client_manager_->RequestDeviceData(request);
        std::unique_lock<std::shared_mutex> lock(data_manager_map_mutex_);
        uint32_t current_tag = data_managers_tag_source_++;
        data_managers_[current_tag] = manager;
        return current_tag;
    }

    uint32_t ClientContext::impl::RequestDataFrame(api::DataRequest& request)
    {
        std::shared_ptr<ommo::DataManager> manager = client_manager_->RequestDataFrame(request);
        std::unique_lock<std::shared_mutex> lock(data_manager_map_mutex_);
        uint32_t current_tag = data_managers_tag_source_++;
        data_managers_[current_tag] = manager;
        return current_tag;
    }

    void ClientContext::impl::CloseRequest(uint32_t request_tag)
    {
        std::unique_lock<std::shared_mutex> lock(data_manager_map_mutex_);
        auto item = data_managers_.find(request_tag);
        if (item != data_managers_.end())
        {
            client_manager_->CloseRequest(item->second);
            data_managers_.erase(item);
        }
    }

    api::DeviceIDList* ClientContext::impl::GetAvailableDeviceList(uint32_t request_tag)
    {
        std::shared_lock<std::shared_mutex> lock(data_manager_map_mutex_);
        auto item = data_managers_.find(request_tag);
        if (item != data_managers_.end())
        {
            return item->second->GetDeviceStorageList().release();
        }
        return new api::DeviceIDList{ nullptr, 0 };
    }

            // Get data functions
    api::DataResponse* ClientContext::impl::GetLatestData(uint32_t request_tag, const api::DeviceID& device_id)
    {
        std::shared_lock<std::shared_mutex> lock(data_manager_map_mutex_);
        auto item = data_managers_.find(request_tag);
        if (item != data_managers_.end())
        {
            return item->second->GetLatestData(device_id).release();
        }
        return new api::DataResponse{ api::DataResponseState::kNoData, nullptr, 0 };
    }

    api::DataResponse* ClientContext::impl::GetLatestData(uint32_t request_tag, const api::DeviceID& device_id, int32_t num_packets)
    {
        std::shared_lock<std::shared_mutex> lock(data_manager_map_mutex_);
        auto item = data_managers_.find(request_tag);
        if (item != data_managers_.end())
        {
            return item->second->GetLatestData(device_id, num_packets).release();
        }
        return new api::DataResponse{ api::DataResponseState::kNoData, nullptr, 0 };
    }

    api::DataResponse* ClientContext::impl::GetDataSinceIndex(uint32_t request_tag, const api::DeviceID& device_id, int32_t start_index)
    {
        std::shared_lock<std::shared_mutex> lock(data_manager_map_mutex_);
        auto item = data_managers_.find(request_tag);
        if (item != data_managers_.end())
        {
            return item->second->GetDataSinceIndex(device_id, start_index).release();
        }
        return new api::DataResponse{ api::DataResponseState::kNoData, nullptr, 0 };
    }

    uint32_t ClientContext::impl::RequestBaseStationData()
    {
        /*
         * When users request base station data through ClientContext, a single storage and backend stream are created.
         * Multiple requests share this storage and stream. Each request gets a unique tag for closing or retrieving data.
         */
        std::unique_lock<std::shared_mutex> lock(base_station_request_list_mutex_);
        if (nullptr == base_station_data_storage_)
        {
            base_station_data_storage_ = client_manager_->RequestBaseStationData();
        }
        uint32_t current_tag = data_managers_tag_source_++;
        base_station_request_list_.insert(current_tag);

        return current_tag;
    }

    void ClientContext::impl::CloseBaseStationDataRequest(uint32_t request_tag)
    {
        std::unique_lock<std::shared_mutex> lock(base_station_request_list_mutex_);
        auto it = base_station_request_list_.find(request_tag);
        if (it != base_station_request_list_.end())
        {
            base_station_request_list_.erase(it);
        }

        // Remove the stream and storage if all request is closed.
        if (base_station_request_list_.empty())
        {
            client_manager_->CloseBaseStationDataRequest(base_station_data_storage_);
        }
    }

    api::BaseStationDataResponse* ClientContext::impl::GetLatestBaseStationData(uint32_t request_tag)
    {
        std::unique_lock<std::shared_mutex> lock(base_station_request_list_mutex_);
        if (nullptr == base_station_data_storage_ || base_station_request_list_.find(request_tag) == base_station_request_list_.end())
        {
            return new api::BaseStationDataResponse{ api::DataResponseState::kNoData, nullptr, 0 };
        }
        lock.unlock();

        return base_station_data_storage_->GetLatestData().release();
    }

    api::BaseStationDataResponse* ClientContext::impl::GetLatestBaseStationData(uint32_t request_tag, int32_t num_packets)
    {
        std::unique_lock<std::shared_mutex> lock(base_station_request_list_mutex_);
        if (nullptr == base_station_data_storage_ || base_station_request_list_.find(request_tag) == base_station_request_list_.end())
        {
            return new api::BaseStationDataResponse{ api::DataResponseState::kNoData, nullptr, 0 };
        }
        lock.unlock();

        return base_station_data_storage_->GetLatestData(num_packets).release();
    }

    api::BaseStationDataResponse* ClientContext::impl::GetBaseStationDataSinceIndex(uint32_t request_tag, int32_t start_index)
    {
        std::unique_lock<std::shared_mutex> lock(base_station_request_list_mutex_);
        if (nullptr == base_station_data_storage_ || base_station_request_list_.find(request_tag) == base_station_request_list_.end())
        {
            return new api::BaseStationDataResponse{ api::DataResponseState::kNoData, nullptr, 0 };
        }
        lock.unlock();

        return base_station_data_storage_->GetDataSinceIndex(start_index).release();
    }

    void ClientContext::impl::RegisterTrackingDeviceDataCallback(uint32_t request_tag, std::function<void(const api::TrackingDeviceData&)> callback_function)
    {
        std::shared_lock<std::shared_mutex> lock(data_manager_map_mutex_);
        auto item = data_managers_.find(request_tag);
        if (item != data_managers_.end())
        {
            item->second->RegisterTrackingDeviceDataCallback(callback_function);
        }
    }

    void ClientContext::impl::ResetTrackingDeviceDataCallback(uint32_t request_tag)
    {
        std::shared_lock<std::shared_mutex> lock(data_manager_map_mutex_);
        auto item = data_managers_.find(request_tag);
        if (item != data_managers_.end())
        {
            item->second->ResetTrackingDeviceDataCallback();
        }
    }

    void ClientContext::impl::RegisterDataFrameCallback(uint32_t request_tag, std::function<void(const api::DataFrame&)> callback_function)
    {
        std::shared_lock<std::shared_mutex> lock(data_manager_map_mutex_);
        auto item = data_managers_.find(request_tag);
        if (item != data_managers_.end())
        {
            item->second->RegisterDataFrameCallback(callback_function);
        }
    }

    void ClientContext::impl::ResetDataFrameCallback(uint32_t request_tag)
    {
        std::shared_lock<std::shared_mutex> lock(data_manager_map_mutex_);
        auto item = data_managers_.find(request_tag);
        if (item != data_managers_.end())
        {
            item->second->ResetDataFrameCallback();
        }
    }

    api::WirelessManager* ClientContext::impl::CreateWirelessManager()
    {
        return client_manager_->CreateWirelessManager().get();
    }

    void ClientContext::impl::DeleteWirelessManager(api::WirelessManager* wireless_manager)
    {
        client_manager_->DeleteWirelessManager(wireless_manager);
    }

    api::DataLogState ClientContext::impl::EnableDataLogging(std::string directory, std::string file_name, bool overwrite)
    {
        return client_manager_->EnableDataLogging(directory, file_name, overwrite);
    }

    api::DataLogState ClientContext::impl::DisableDataLogging()
    {
        return client_manager_->DisableDataLogging();
    }

    bool ClientContext::impl::SelectReferenceDevice(bool enabled, uint32_t siu_uuid, uint32_t port_num)
    {
        return client_manager_->SelectReferenceDevice(enabled, siu_uuid, port_num);
    }
} // namespace ommo::api

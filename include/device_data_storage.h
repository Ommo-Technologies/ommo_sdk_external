/*
 * Copyright 2025 Ommo Technologies, Inc. - All Rights Reserved
 *
 * Unless required by applicable law or agreed to in writing, software
 * is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS
 * OF ANY KIND, either express or implied.
*/

#pragma once

#include <atomic>
#include <shared_mutex>
#include "sdk_types.h"
#include "ommo_service_api.pb.h"

namespace ommo
{
    class DeviceDataStorage
    {
    private:
        struct BufferInfo
        {
            std::atomic_int32_t data_num;
            api::DevicePacket* packet_buffer_ptr;
        };

        void SwitchBufferPointer();

        const api::DeviceDescriptorUPtr device_;
        uint32_t buffer_size_;

        // Record the packet index.
        uint32_t packet_received_num_;

        // Use DevicePacketUPtr so memory clean up on destruction happens properly
        api::DevicePacket* packet_buffer1_;
        api::DevicePacket* packet_buffer2_;

        BufferInfo read_buffer_;
        BufferInfo write_buffer_;
        std::shared_mutex switch_mutex_;

    public:
        DeviceDataStorage(const api::DeviceDescriptor& device, uint32_t buffer_size);
        ~DeviceDataStorage();

        uint32_t GetUUID() const;
        uint32_t GetPortId() const;

        bool PushData(const ommo::TrackingDeviceData& m);

        // Return the most recent packet.
        api::DataResponseUPtr GetLatestData();

        // Return the most recent <count> packets.
        api::DataResponseUPtr GetLatestData(uint32_t count);

        // TODO: This function currently does not handle when packet_idx wraps
        // TODO: Implement proper wrapping handling when packet_idx overflows uint32
        // Return all packets starting from start_idx;
        api::DataResponseUPtr GetDataSinceIndex(uint32_t start_idx);
    };
}  // namespace ommo

/*
 * Copyright 2025 Ommo Technologies, Inc. - All Rights Reserved
 *
 * Unless required by applicable law or agreed to in writing, software
 * is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS
 * OF ANY KIND, either express or implied.
*/

#include "device_data_storage.h"
#include "protobuf_converters.h"

#include <spdlog/spdlog.h>

namespace ommo
{

    uint32_t DeviceDataStorage::GetUUID() const
    {
        return device_->siu_uuid;
    }

    uint32_t DeviceDataStorage::GetPortId() const
    {
        return device_->port_id;
    }

    DeviceDataStorage::DeviceDataStorage(const api::DeviceDescriptor& device, uint32_t buffer_size) : buffer_size_(buffer_size), packet_received_num_(0),
         // Initialize device_ as DevicePacketUPtr for automatic deletion
         device_(api::CopyDeviceDescriptor(device))
    {
        // Allocate memory for data buffer and info buffer.
        packet_buffer1_ = new api::DevicePacket[buffer_size_];
        packet_buffer2_ = new api::DevicePacket[buffer_size_];

        // Initialize buffer memory so they are not random values
        for (int i = 0; i < buffer_size_; i++)
        {
            // Initialize the buffers to the default value to ensure there is no bad
            // data in the buffers. Otherwise, the can be pointing to random locations
            packet_buffer1_[i] = {};
            packet_buffer2_[i] = {};
        }

        // Initialize the read and write buffer info.
        read_buffer_.packet_buffer_ptr = packet_buffer1_;
        read_buffer_.data_num.store(0);
        write_buffer_.packet_buffer_ptr = packet_buffer2_;
        write_buffer_.data_num.store(0);
    }

    DeviceDataStorage::~DeviceDataStorage()
    {
        // Delete all dynamically allocated memory
        for (int i = 0; i < buffer_size_; i++)
        {
            api::DestroyDevicePacketMembers(packet_buffer1_[i]);
            api::DestroyDevicePacketMembers(packet_buffer2_[i]);
        }
        delete[] packet_buffer1_;
        delete[] packet_buffer2_;
    }

    bool DeviceDataStorage::PushData(const ommo::TrackingDeviceData& packet)
    {
        if (packet.siu_uuid() != device_->siu_uuid || packet.port_id() != device_->port_id)
        {
            return false;
        }

        // Save the data.
        const int32_t write_idx = write_buffer_.data_num;
        if (write_idx < buffer_size_)
        {
            // Important to delete previously allocated packets before copying new one
            api::DestroyDevicePacketMembers(write_buffer_.packet_buffer_ptr[write_idx]);
            // Save raw data.
            write_buffer_.packet_buffer_ptr[write_idx].packet_idx = packet_received_num_++;
            // Use helper function to ensure memory is properly taken over and deallocated
            api::MoveUniquePtr(write_buffer_.packet_buffer_ptr[write_idx].device_data, ommo::ProtoToTrackingDeviceData(packet));

            write_buffer_.data_num++;

            if (write_buffer_.data_num == buffer_size_)
            {
                // Switch write and read buffers when the write buffer is full.
                SwitchBufferPointer();
                // Reset the writer buffer (previous read buffer) to save new data.
                write_buffer_.data_num.store(0);
            }
        }
        else
        {
            SwitchBufferPointer();

            // Important to delete previously allocated packets before copying new one
            api::DestroyDevicePacketMembers(write_buffer_.packet_buffer_ptr[0]);
            // Save raw data.
            write_buffer_.packet_buffer_ptr[0].packet_idx = packet_received_num_++;
            // Use helper function to ensure memory is properly taken over and deallocated
            api::MoveUniquePtr(write_buffer_.packet_buffer_ptr[0].device_data, ommo::ProtoToTrackingDeviceData(packet));

            write_buffer_.data_num.store(1);
        }
        return true;
    }

    api::DataResponseUPtr DeviceDataStorage::GetLatestData()
    {
        api::DataResponseUPtr result(new api::DataResponse{ api::DataResponseState::kNoData, nullptr, 0 });
        // The write and read buffers cannot switch while reading data.
        std::shared_lock<std::shared_mutex> lock(switch_mutex_);

        // If write_buffer already has data, get the latest data from it.
        int32_t idx = write_buffer_.data_num - 1;
        if (idx >= 0)
        {
            result->packets = new api::DevicePacket[1];
            api::MoveAndDeletePtr(result->packets[0], api::CopyDevicePacket(write_buffer_.packet_buffer_ptr[idx]));
            result->packet_count = 1;
            result->state = api::DataResponseState::kSuccess;
        }
        else
        {
            idx = read_buffer_.data_num - 1;
            if (idx >= 0)
            {
                result->packets = new api::DevicePacket[1];
                api::MoveAndDeletePtr(result->packets[0], api::CopyDevicePacket(read_buffer_.packet_buffer_ptr[idx]));
                result->packet_count = 1;
                result->state = api::DataResponseState::kSuccess;
            }
        }
        // If the read and write buffers are both empty, return default value.
        return result;
    }

    api::DataResponseUPtr DeviceDataStorage::GetLatestData(uint32_t request_count)
    {
        api::DataResponseUPtr result(new api::DataResponse{ api::DataResponseState::kNoData, nullptr, 0 });

        // The write and read buffers cannot switch while reading data.
        std::shared_lock<std::shared_mutex> lock(switch_mutex_);

        const int32_t write_packet_num = write_buffer_.data_num;
        const int32_t read_packet_num = read_buffer_.data_num;

        // Only get data if request_count is not 0
        if (request_count > 0)
        {
            if (request_count <= write_packet_num)
            {
                // Read from write buffer.
                uint32_t start_idx = write_packet_num - request_count;
                result->packets = new api::DevicePacket[request_count];

                for (int32_t i = 0; i < request_count; i++)
                {
                    // Create new copy and take over memory owernship of copy
                    api::MoveAndDeletePtr(result->packets[i], api::CopyDevicePacket(write_buffer_.packet_buffer_ptr[start_idx + i]));
                }
                result->packet_count = request_count;
                result->state = api::DataResponseState::kSuccess;
            }
            else if (request_count <= read_packet_num + write_packet_num)
            {
                int32_t idx = read_packet_num + write_packet_num - request_count;
                result->packets = new api::DevicePacket[request_count];

                // First read from the read buffer.
                // Note: variable i is used when getting data from read buffer and write buffer.
                int32_t i = 0;
                for (; idx < read_packet_num; idx++)
                {
                    // Create new copy and take over memory owernship of copy
                    api::MoveAndDeletePtr(result->packets[i], api::CopyDevicePacket(read_buffer_.packet_buffer_ptr[idx]));
                    i++;
                }
                // Then read from write buffer.
                for (idx = 0; idx < write_packet_num; idx++)
                {
                    api::MoveAndDeletePtr(result->packets[i], api::CopyDevicePacket(write_buffer_.packet_buffer_ptr[idx]));
                    i++;
                }
                result->packet_count = request_count;
                result->state = api::DataResponseState::kSuccess;
            }
            else if (read_packet_num + write_packet_num > 0)
            {
                result->packets = new api::DevicePacket[read_packet_num + write_packet_num];
                // First read from read buffer.
                // Note: variable i is used when getting data from read buffer and write buffer.
                int32_t i = 0;
                for (int32_t idx = 0; idx < read_packet_num; idx++)
                {
                    api::MoveAndDeletePtr(result->packets[i], api::CopyDevicePacket(read_buffer_.packet_buffer_ptr[idx]));
                    i++;
                }
                // Then read from write buffer.
                for (int32_t idx = 0; idx < write_packet_num; idx++)
                {
                    api::MoveAndDeletePtr(result->packets[i], api::CopyDevicePacket(write_buffer_.packet_buffer_ptr[idx]));
                    i++;
                }
                result->packet_count = read_packet_num + write_packet_num;
                result->state = api::DataResponseState::kPartialData;
            }
        }
        return result;
    }

    api::DataResponseUPtr DeviceDataStorage::GetDataSinceIndex(uint32_t start_idx)
    {
        api::DataResponseUPtr result(new api::DataResponse{ api::DataResponseState::kNoData, nullptr, 0 });

        // The write and read buffers cannot switch while reading data.
        std::shared_lock<std::shared_mutex> lock(switch_mutex_);

        const int32_t write_packet_num = write_buffer_.data_num;
        const int32_t read_packet_num = read_buffer_.data_num;

        // First check the write buffer.
        if (write_packet_num > 0 && write_buffer_.packet_buffer_ptr[write_packet_num - 1].packet_idx < start_idx)
        {
            return result;
        }
        // If the first packet in write buffer has index <= start_idx, it contains all of the required packets.
        else if (write_packet_num > 0 && write_buffer_.packet_buffer_ptr[0].packet_idx <= start_idx)
        {
            int32_t idx = start_idx - write_buffer_.packet_buffer_ptr[0].packet_idx;
            uint32_t packet_count = write_packet_num - idx;
            result->packets = new api::DevicePacket[packet_count];

            for (int32_t i = 0; i < packet_count; i++)
            {
                api::MoveAndDeletePtr(result->packets[i], api::CopyDevicePacket(write_buffer_.packet_buffer_ptr[idx + i]));
            }
            result->state = api::DataResponseState::kSuccess;
            result->packet_count = packet_count;
        }
        // If write buffer is empty and the latest packet in read buffer has index < start_idx, requested data is not available.
        else if (write_packet_num == 0 && read_packet_num > 0 && read_buffer_.packet_buffer_ptr[read_packet_num - 1].packet_idx < start_idx)
        {
            return result;
        }
        // If the first packet in read buffer has index <= start_idx, packets will be required from both the read and write buffers
        else if (read_packet_num > 0 && read_buffer_.packet_buffer_ptr[0].packet_idx <= start_idx)
        {
            int32_t idx = start_idx - read_buffer_.packet_buffer_ptr[0].packet_idx;
            result->packets = new api::DevicePacket[read_packet_num - idx + write_packet_num];

            // Note: variable i is used when getting data from read buffer and write buffer.
            int32_t i = 0;
            for (; idx < read_packet_num; idx++)
            {
                api::MoveAndDeletePtr(result->packets[i], api::CopyDevicePacket(read_buffer_.packet_buffer_ptr[idx]));
                i++;
            }
            for (idx = 0; idx < write_packet_num; idx++)
            {
                api::MoveAndDeletePtr(result->packets[i], api::CopyDevicePacket(write_buffer_.packet_buffer_ptr[idx]));
                i++;
            }
            result->state = api::DataResponseState::kSuccess;
            result->packet_count = read_packet_num + write_packet_num + read_buffer_.packet_buffer_ptr[0].packet_idx - start_idx;
        }
        else if (read_packet_num > 0 && read_buffer_.packet_buffer_ptr[0].packet_idx > start_idx)
        {
            result->packets = new api::DevicePacket[read_packet_num + write_packet_num];
            int32_t i = 0;
            // First read from read buffer.
            for (int32_t idx = 0; idx < read_packet_num; idx++)
            {
                api::MoveAndDeletePtr(result->packets[i], api::CopyDevicePacket(read_buffer_.packet_buffer_ptr[idx]));
                i++;
            }
            // Then read from write buffer.
            for (int32_t idx = 0; idx < write_packet_num; idx++)
            {
                api::MoveAndDeletePtr(result->packets[i], api::CopyDevicePacket(write_buffer_.packet_buffer_ptr[idx]));
                i++;
            }
            result->state = api::DataResponseState::kPartialData;
            result->packet_count = write_packet_num + read_packet_num;
        }
        return result;
    }

    void DeviceDataStorage::SwitchBufferPointer()
    {
        std::unique_lock<std::shared_mutex> lock(switch_mutex_);

        api::DevicePacket* temp_ptr = write_buffer_.packet_buffer_ptr;
        write_buffer_.packet_buffer_ptr = read_buffer_.packet_buffer_ptr;
        read_buffer_.packet_buffer_ptr = temp_ptr;

        const int read_num = read_buffer_.data_num;
        const int write_num = write_buffer_.data_num.exchange(read_num);
        read_buffer_.data_num.store(write_num);
    }

}  // namespace ommo

/*
 * Copyright 2025 Ommo Technologies, Inc. - All Rights Reserved
 *
 * Unless required by applicable law or agreed to in writing, software
 * is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS
 * OF ANY KIND, either express or implied.
*/

#include "rpc_wireless_management_stream_client_call_data.h"

#include <grpc/support/log.h>
#include <cassert>

using grpc::ClientAsyncReader;

RpcWirelessManagementStreamClientCallData::RpcWirelessManagementStreamClientCallData(
    std::shared_ptr<Channel> channel, 
    CompletionQueue* cq, 
    const std::function<void(const ommo::WirelessManagementEvent&)> cb_handler, 
    std::weak_ptr<ommo::CallDataAssociation> association)
    : rpcClientCallData(channel, cq, ClientCallState::CONNECTING, association), cb_handler_(cb_handler)
{
    // Prepare call get bidirectional reader/writer
    stream_handler_ = stub->PrepareAsyncOpenWirelessManagementStream(&grpc_client_context, completion_queue);

    // StartCall initiates the RPC call
    stream_handler_->StartCall(&internal_read_info);

    if (cb_handler)
    {
        listener_active = true;
    }
}

bool RpcWirelessManagementStreamClientCallData::SendWirelessManagementRequest(const ommo::WirelessManagementRequest &request)
{
    rwlock_wrlockguard lock(statusLock);
    // TODO: check for cancels? or server failures?
    if (status == ClientCallState::PROCESSING)
    {
        // already doing a write, return false for now
        // TODO: consider using a queue/buffer?
        return false;
    }
    if (status == ClientCallState::CONNECTING || status == ClientCallState::WAITING)
    {
        status = ClientCallState::PROCESSING;
        stream_handler_->Write(request, &internal_write_info);
        return true;
    }
    return false;
}

bool RpcWirelessManagementStreamClientCallData::Proceed(OperationType op_type)
{
    rwlock_wrlockguard lock(statusLock);

    if (status == ClientCallState::CONNECTING)
    {
        // Start a read
        status = ClientCallState::WAITING;
        stream_handler_->Read(&response_, &internal_read_info);

        return true;
    }
    else if (status == ClientCallState::WAITING)
    {
        // Read finished send to callback_handler
        if (op_type == OperationType::READ)
        {
            if (cb_handler_ && listener_active)
            {
                cb_handler_(response_);
            }

            // Start another read
            stream_handler_->Read(&response_, &internal_read_info);
            // No need to change state since we are just waiting for more reads
        }

        return true;
    }
    else if (status == ClientCallState::PROCESSING)
    {
        // Read finished while waiting for write send to callback_handler
        if (op_type == OperationType::READ)
        {
            if (cb_handler_ && listener_active)
            {
                cb_handler_(response_);
            }

            // Start another read
            stream_handler_->Read(&response_, &internal_read_info);
            // No need to change state since we are still waiting for write to finish
        }
        else if (op_type == OperationType::WRITE)
        {
            // write finished
            // TODO: Handle queue to write more?
            status = ClientCallState::WAITING;
        }
        return true;
    }
    else
    {
        // Once in the FINISH state, deallocate ourselves (CallData).
        assert(status == ClientCallState::FINISH);
        // Delete this object
        return false;
    }
}

void RpcWirelessManagementStreamClientCallData::CloseStream()
{
    // TODO: can properly close stream with sending FINISH? instead of CancelAll on context?
}

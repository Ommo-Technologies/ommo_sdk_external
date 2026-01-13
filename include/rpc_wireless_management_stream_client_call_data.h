/*
 * Copyright 2025 Ommo Technologies, Inc. - All Rights Reserved
 *
 * Unless required by applicable law or agreed to in writing, software
 * is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS
 * OF ANY KIND, either express or implied.
*/

#pragma once

#include "grpcpp/grpcpp.h"
#include "ommo_service_api.grpc.pb.h"
#include "rpcClientCallData.h"

using grpc::ClientAsyncReaderWriter;

class RpcWirelessManagementStreamClientCallData : public rpcClientCallData
{
public:
    RpcWirelessManagementStreamClientCallData(
        std::shared_ptr<Channel> channel, 
        CompletionQueue* cq, 
        const std::function<void(const ommo::WirelessManagementEvent&)> cb_handler, 
        std::weak_ptr<ommo::CallDataAssociation> association = std::weak_ptr<ommo::CallDataAssociation>{}
    );

    bool SendWirelessManagementRequest(const ommo::WirelessManagementRequest &request);
    bool Proceed(OperationType op_type);
    void CloseStream();

private:
    const std::function<void(const ommo::WirelessManagementEvent&)> cb_handler_;
    ommo::WirelessManagementEvent response_;
    std::unique_ptr<ClientAsyncReaderWriter<ommo::WirelessManagementRequest, ommo::WirelessManagementEvent>> stream_handler_;
};

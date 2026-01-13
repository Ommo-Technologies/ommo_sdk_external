/*
 * Copyright 2025 Ommo Technologies, Inc. - All Rights Reserved
 *
 * Unless required by applicable law or agreed to in writing, software
 * is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS
 * OF ANY KIND, either express or implied.
*/

#pragma once

// Define DLL exports
#ifdef _WIN32
#ifdef OMMO_SDK_STATIC
#define OMMO_SDK_API
#elif defined(OMMO_SDK_EXPORTS)
#define OMMO_SDK_API __declspec(dllexport)
#else
#define OMMO_SDK_API __declspec(dllimport)
#endif
#else
#ifdef OMMO_SDK_STATIC
#define OMMO_SDK_API
#else
#define OMMO_SDK_API __attribute__((visibility("default")))
#endif
#endif

#include <functional>
#include "sdk_types.h"
#include "wireless_manager.h"

namespace ommo::api
{
    class OMMO_SDK_API ClientContext
    {
    public:
        /*
         * Creates a ClientContext object to interface with ommo service. If no server_address is provided
         * the default "localhost:50051" will be used.
         */
        explicit ClientContext(const char* server_address = nullptr);

        /*
         * The copying of ClientContext is disabled to prevent issues related to memory management and ABI compatibility.
         */
        ClientContext(const ClientContext& other) = delete;
        ClientContext& operator= (const ClientContext& other) = delete;

        /*
         * Destroys the client context and closes all connections to ommo service.
         */
        ~ClientContext();

        /*
         * Start the client context and attempt to establish connection to ommo service via a gRPC channel
         * on the provided server address.
         */
        void Start();

        /*
         * Shutdown the client context. This will close the connection to ommo service, cancel all data
         * requests, and delete all available data.
         */
        void Shutdown();

        /*
         * Enable logging for internal DLL output. If no filename is provided, only console logging will be used
         */
        void SetupLogging(const char* file_name = nullptr);

        /*
         * Get a list of tracking devices that are currently connected to and available from ommo service
         */
        api::TrackingDevices* GetTrackingDevices();

        /*
         * Get a list of basestation, siu, and wireless receiver states for all hardware that ommo service
         * has encountered since start up.
         */
        api::HardwareStates* GetHardwareStates();

        /*
         * Set the base station motor running to on/off on the server.
         * Returns true on success, false on RPC error or server-side failure.
         */
        bool SetBaseStationMotorRunning(bool active);

        /*
         * Register a call back to be called whenever a TrackingDeviceEvent is received from service
         * Only one call back can be registered at a time. Registering another callback will overwrite the existing one.
         */
        void RegisterDeviceEventCallback(std::function<void(const api::TrackingDeviceEvent&)> callback_function);

        /*
         * Reset the currently registered callback for TrackingDeviceEvent so it'll no longer be called
         */
        void ResetDeviceEventCallback();

        /*
         * Register a call back to be called whenever channel connection state to service changes
         * Only one call back can be registered at a time. Registering another callback will overwrite the existing one.
         */
        void RegisterChannelStateCallback(std::function<void(int)> callback_function);

        /*
         * Reset the currently registered callback for channel connection state so it'll no longer be called
         */
        void ResetChannelStateCallback();

        /*
         * Request real-time data from one or more devices. Data is returned individually for each device as
         * soon as it is ready. This request is suitable for scenarios where having the most recent data
         * possible is preferred or required.
         *
         * Individual devices will return data at an interval of ~1000/second.
         * @return the tag for the created request
         */
        uint32_t RequestDeviceData(api::DataRequest& request);

        /*
         * Request a grouping of data from one or more devices. Data for all specified devices is returned collectively
         * in a single data frame. This request is suitable for scenarios where having a more synchronized snapshot for
         * all devices is preferred.
         *
         * The minimum interval for a data frame is 20ms (~50 data frames/second).
         * @return the tag for the created request
         */
        uint32_t RequestDataFrame(api::DataRequest& request);

        /*
         * Terminate the open request stream associated with the request tag. This function is used for both DeviceData and
         * DataFrame requests. Once called, the tag can no longer be used with any functions requiring a request tag.
         */
        void CloseRequest(uint32_t request_tag);

        /* 
         * Request data from base station.
         *
         * Base station will return data packet at an interval of ~4/second.
         * @return the tag for the created request, which can be used to retrieve base station data.
         */
        uint32_t RequestBaseStationData();

        /*
        * Terminate the stream associated with the given request tag.
        * After this call, the tag cannot be used with any functions that require a request tag.
        */
        void CloseBaseStationDataRequest(uint32_t request_tag);

        /*
         *  Request a list of devices associated with the request.
         */
        api::DeviceIDList* GetAvailableDeviceList(uint32_t request_tag);

        /*
         * Request the most recent data received for the specified request and device.
         * 
         * The DataResponse state should be checked to ensure that data is available.
         */
        api::DataResponse* GetLatestData(uint32_t request_tag, const api::DeviceID& device_id);

        /*
         * Request the most recent <num_packets> data received for the specified request and device.
         * 
         * The DataResponse state should be checked to ensure that data is available.
         */
        api::DataResponse* GetLatestData(uint32_t request_tag, const api::DeviceID& device_id, int32_t num_packets);

        /*
         * Request all data received since <start_index> for the specified request and device.
         * 
         * The DataResponse state should be checked to ensure that data is available.
         */
        api::DataResponse* GetDataSinceIndex(uint32_t request_tag, const api::DeviceID& device_id, int32_t start_index);

        /*
         * Request the most recent data received for the base station.
         * 
         * The BaseStationDataResponse state should be checked to ensure that data is available.
         */
        api::BaseStationDataResponse* GetLatestBaseStationData(uint32_t request_tag);

        /*
         * Request the most recent <num_packets> data received for the base station.
         * 
         * The BaseStationDataResponse state should be checked to ensure that data is available.
         */
        api::BaseStationDataResponse* GetLatestBaseStationData(uint32_t request_tag, int32_t num_packets);

        /*
         * Request all data received since <start_index> for the base station.
         * 
         * The BaseStationDataResponse state should be checked to ensure that data is available.
         */
        api::BaseStationDataResponse* GetBaseStationDataSinceIndex(uint32_t request_tag, int32_t start_index);

        /* 
         * Register a call back to be called whenever a TrackingDeviceData is received for the Request identified by request_tag
         * 
         * Register function will do nothing unless the Request was created from RequestDeviceData
         * 
         * Only one call back can be registered at a time for a Request. Registering another callback will overwrite the existing one.
         */
        void RegisterTrackingDeviceDataCallback(uint32_t request_tag, std::function<void(const api::TrackingDeviceData&)> callback_function);

        /*
         * Reset the currently registered callback for TrackingDeviceData for the Request identified by request_tag
         * 
         * The callback associated with the Request will no longer be called after reset.
         */ 
        void ResetTrackingDeviceDataCallback(uint32_t request_tag);

        /*
         * Register a call back to be called whenever a DataFrame is received for the Request identified by request_tag
         *
         * Register function will do nothing unless the Request was created from RequestDataFrame
         *
         * Only one call back can be registered at a time for a Request. Registering another callback will overwrite the existing one.
         */
        void RegisterDataFrameCallback(uint32_t request_tag, std::function<void(const api::DataFrame&)> callback_function);

        /*
         * Reset the currently registered callback for DataFrame for the Request identified by request_tag
         *
         * The callback associated with the Request will no longer be called after reset.
         */
        void ResetDataFrameCallback(uint32_t request_tag);

        /*
         * Create a WirelessManager that can be used to manage wireless devices via the ommo service.
         * @return pointer of the created WirelessManager.
         */
        api::WirelessManager* CreateWirelessManager();

        /*
         * Delete the WirelessManager.
         * After this call, the WirelessManager will no longer be functional.
         */
        void DeleteWirelessManager(api::WirelessManager* wireless_manager);

        /*
         * Enable raw data logging. The data will be saved using the specified directory and file name.
         * If the overwrite flag is set to true, the existing file will be overwritten.
         * If the overwrite flag is set to false and file already exists, logging will not be enabled. 
         * @return api::DataLogState::kEnabled only if logging is successfully enabled.
         */
        api::DataLogState EnableDataLogging(const char* directory, const char* file_name, bool overwrite);

        /*
         * Disable raw data logging.
         * @return api::DataLogState::kDisabled only if logging is successfully disabled.
         */
        api::DataLogState DisableDataLogging();

        /*
         * Select a specific device as the reference.
         * @param enabled Whether to enable or disable the reference selection
         * @param siu_uuid The uuid of the device
         * @param port_num The port number of the device
         * @return true if the request was successful, false otherwise
         */
        bool SelectReferenceDevice(bool enabled, uint32_t siu_uuid, uint32_t port_num);

    private:
        class impl;
        ClientContext::impl* const p_impl_;
    };

    // TODO: C API bindings to be implemented

} // namespace ommo::api

//******************************************************************
//
// Copyright 2015 Samsung Electronics All Rights Reserved.
//
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#include "RemoteEnrollee.h"
#include "EnrolleeResource.h"
#include "CloudResource.h"
#include "OCPlatform.h"
#include "ESException.h"
#include "logger.h"
#include "OCResource.h"
#ifdef __WITH_DTLS__
#include "EnrolleeSecurity.h"
#endif //__WITH_DTLS

namespace OIC
{
    namespace Service
    {
        static const char ES_BASE_RES_URI[] = "/oic/res";
        #define ES_REMOTE_ENROLLEE_TAG "ES_REMOTE_ENROLLEE"
        #define DISCOVERY_TIMEOUT 5

        RemoteEnrollee::RemoteEnrollee(std::shared_ptr< OC::OCResource > resource)
        {
            m_ocResource = resource;
            m_enrolleeResource = std::make_shared<EnrolleeResource>(m_ocResource);
            m_securityProvStatusCb = nullptr;
            m_getConfigurationStatusCb = nullptr;
            m_securityPinCb = nullptr;
            m_secProvisioningDbPathCb = nullptr;
            m_devicePropProvStatusCb = nullptr;
            m_cloudPropProvStatusCb = nullptr;
        }

#ifdef __WITH_DTLS__
        ESResult RemoteEnrollee::registerSecurityCallbackHandler(SecurityPinCb securityPinCb,
                SecProvisioningDbPathCb secProvisioningDbPathCb)
        {
            // No need to check NULL for m_secProvisioningDbPathCB as this is not a mandatory
            // callback function. If m_secProvisioningDbPathCB is NULL, provisioning manager
            // in security layer will try to find the PDM.db file in the local path.
            // If PDM.db is found, the provisioning manager operations will succeed.
            // Otherwise all the provisioning manager operations will fail.
            m_secProvisioningDbPathCb = secProvisioningDbPathCb;
            m_securityPinCb = securityPinCb;
            return ES_OK;
        }
#endif //__WITH_DTLS__

        void RemoteEnrollee::securityStatusHandler(
                        std::shared_ptr< SecProvisioningStatus > status)
        {
            OIC_LOG_V(DEBUG, ES_REMOTE_ENROLLEE_TAG, "easySetupStatusCallback status is, UUID = %s, "
                    "Status = %d", status->getDeviceUUID().c_str(),
                    status->getESResult());

            if(status->getESResult() == ES_OK)
            {
                OIC_LOG(DEBUG, ES_REMOTE_ENROLLEE_TAG, "Ownership and ACL are successful. "
                        "Continue with Network information provisioning");

                OIC_LOG(DEBUG,ES_REMOTE_ENROLLEE_TAG,"Before ProvisionEnrollee");

                m_securityProvStatusCb(status);
            }
            else
            {
                OIC_LOG(DEBUG, ES_REMOTE_ENROLLEE_TAG, "Ownership and ACL are fail");

                m_securityProvStatusCb(status);
            }
        }

        void RemoteEnrollee::getConfigurationStatusHandler (
                std::shared_ptr< GetConfigurationStatus > status)
        {
            OIC_LOG(DEBUG,ES_REMOTE_ENROLLEE_TAG,"Entering getConfigurationStatusHandler");

            OIC_LOG_V(DEBUG,ES_REMOTE_ENROLLEE_TAG,"GetConfigurationStatus = %d", status->getESResult());

            m_getConfigurationStatusCb(status);
        }

        void RemoteEnrollee::devicePropProvisioningStatusHandler(
                std::shared_ptr< DevicePropProvisioningStatus > status)
        {
            OIC_LOG(DEBUG,ES_REMOTE_ENROLLEE_TAG,"Entering DevicePropProvisioningStatusHandler");

            OIC_LOG_V(DEBUG,ES_REMOTE_ENROLLEE_TAG,"ProvStatus = %d", status->getESResult());

            m_devicePropProvStatusCb(status);

            return;
        }

        void RemoteEnrollee::cloudPropProvisioningStatusHandler (
                std::shared_ptr< CloudPropProvisioningStatus > status)
        {
            OIC_LOG(DEBUG,ES_REMOTE_ENROLLEE_TAG,"Entering cloudPropProvisioningStatusHandler");

            OIC_LOG_V(DEBUG,ES_REMOTE_ENROLLEE_TAG,"CloudProvStatus = %d", status->getESCloudState());

            m_cloudPropProvStatusCb(status);
            return;
        }

        ESResult RemoteEnrollee::ESDiscoveryTimeout(unsigned short waittime)
        {
            struct timespec startTime;
            startTime.tv_sec=0;
            startTime.tv_sec=0;
            struct timespec currTime;
            currTime.tv_sec=0;
            currTime.tv_nsec=0;

            ESResult res = ES_OK;
            #ifdef _POSIX_MONOTONIC_CLOCK
                int clock_res = clock_gettime(CLOCK_MONOTONIC, &startTime);
            #else
                int clock_res = clock_gettime(CLOCK_REALTIME, &startTime);
            #endif

            if (0 != clock_res)
            {
                return ES_ERROR;
            }

            while (ES_OK == res || m_discoveryResponse == false)
            {
                #ifdef _POSIX_MONOTONIC_CLOCK
                        clock_res = clock_gettime(CLOCK_MONOTONIC, &currTime);
                #else
                        clock_res = clock_gettime(CLOCK_REALTIME, &currTime);
                #endif

                if (0 != clock_res)
                {
                    return ES_ERROR;
                }
                long elapsed = (currTime.tv_sec - startTime.tv_sec);
                if (elapsed > waittime)
                {
                    return ES_OK;
                }
                if (m_discoveryResponse)
                {
                    res = ES_OK;
                }
             }
             return res;
        }

        void RemoteEnrollee::onDeviceDiscovered(std::shared_ptr<OC::OCResource> resource)
        {
            OIC_LOG (DEBUG, ES_REMOTE_ENROLLEE_TAG, "onDeviceDiscovered");

            std::string resourceURI;
            std::string hostAddress;
            std::string hostDeviceID;

            try
            {
                if(resource)
                {
                    if(!(resource->connectivityType() & CT_ADAPTER_TCP))
                    {
                        // Get the resource URI
                        resourceURI = resource->uri();
                        OIC_LOG_V (DEBUG, ES_REMOTE_ENROLLEE_TAG,
                                "URI of the resource: %s", resourceURI.c_str());

                        // Get the resource host address
                        hostAddress = resource->host();
                        OIC_LOG_V (DEBUG, ES_REMOTE_ENROLLEE_TAG,
                                "Host address of the resource: %s", hostAddress.c_str());

                        hostDeviceID = resource->sid();
                        OIC_LOG_V (DEBUG, ES_REMOTE_ENROLLEE_TAG,
                                "Host DeviceID of the resource: %s", hostDeviceID.c_str());

                        if(!m_deviceId.empty() && m_deviceId == hostDeviceID)
                        {
                            OIC_LOG (DEBUG, ES_REMOTE_ENROLLEE_TAG, "Find matched CloudResource");
                            m_ocResource = resource;
                            m_discoveryResponse = true;
                        }
                    }
                }
            }
            catch(std::exception& e)
            {
                OIC_LOG_V (DEBUG, ES_REMOTE_ENROLLEE_TAG,
                        "Exception in foundResource: %s", e.what());
            }
        }

        ESResult RemoteEnrollee::discoverResource()
        {
            OIC_LOG(DEBUG, ES_REMOTE_ENROLLEE_TAG, "Enter discoverResource");

            std::string query("");
            query.append(ES_BASE_RES_URI);
            query.append("?rt=");
            query.append(OC_RSRVD_ES_RES_TYPE_PROV);

            OIC_LOG_V (DEBUG, ES_REMOTE_ENROLLEE_TAG, "query = %s", query.c_str());

            m_discoveryResponse = false;

            std::function< void (std::shared_ptr<OC::OCResource>) > onDeviceDiscoveredCb =
                    std::bind(&RemoteEnrollee::onDeviceDiscovered, this,
                                                    std::placeholders::_1);
            OCStackResult result = OC::OCPlatform::findResource("", query, CT_DEFAULT,
                    onDeviceDiscoveredCb);

            if (result != OCStackResult::OC_STACK_OK)
            {
                OIC_LOG(ERROR,ES_REMOTE_ENROLLEE_TAG,
                        "Failed discoverResource");
                return ES_ERROR;
            }

            ESResult foundResponse = ESDiscoveryTimeout (DISCOVERY_TIMEOUT);

            if (foundResponse == ES_ERROR || !m_discoveryResponse)
            {
                OIC_LOG(ERROR,ES_REMOTE_ENROLLEE_TAG,
                        "Failed discoverResource because timeout");
                return ES_ERROR;
            }
            return ES_OK;
        }

        void RemoteEnrollee::configureSecurity(SecurityProvStatusCb callback)
        {
#ifdef __WITH_DTLS__
            m_securityProvStatusCb = callback;

            SecurityProvStatusCb securityProvStatusCb = std::bind(
                    &RemoteEnrollee::securityStatusHandler,
                    this,
                    std::placeholders::_1);
            //TODO : DBPath is passed empty as of now. Need to take dbpath from application.
            m_enrolleeSecurity = std::make_shared <EnrolleeSecurity> (m_ocResource, "");

            m_enrolleeSecurity->registerCallbackHandler(securityProvStatusCb, m_securityPinCb, m_secProvisioningDbPathCb);

            try
            {
                m_enrolleeSecurity->performOwnershipTransfer();
            }
            catch (OCException & e)
            {
                OIC_LOG_V(ERROR, ES_REMOTE_ENROLLEE_TAG,
                        "Exception for performOwnershipTransfer : %s", e.reason().c_str());

                OIC_LOG(DEBUG,ES_REMOTE_ENROLLEE_TAG,"Fail performOwnershipTransfer");
                    std::shared_ptr< SecProvisioningStatus > securityProvisioningStatus =
                            std::make_shared< SecProvisioningStatus >(nullptr, ES_ERROR);
                    m_securityProvStatusCb(securityProvisioningStatus);
                return ;
            }
#else
            OIC_LOG(DEBUG,ES_REMOTE_ENROLLEE_TAG,"Mediator is unsecured.");

            std::shared_ptr< SecProvisioningStatus > securityProvisioningStatus =
                     std::make_shared< SecProvisioningStatus >(nullptr, ES_ERROR);
            m_securityProvStatusCb(securityProvisioningStatus);
#endif
        }

        void RemoteEnrollee::getConfiguration(GetConfigurationStatusCb callback)
        {
            if(!callback)
            {
                throw ESInvalidParameterException("Callback is empty");
            }

            m_getConfigurationStatusCb = callback;

            if (m_enrolleeResource == nullptr)
            {
                throw ESBadRequestException ("Device not created");
            }

            GetConfigurationStatusCb getConfigurationStatusCb = std::bind(
                    &RemoteEnrollee::getConfigurationStatusHandler, this, std::placeholders::_1);
            m_enrolleeResource->registerGetConfigurationStatusCallback(getConfigurationStatusCb);
            m_enrolleeResource->getConfiguration();
        }

        void RemoteEnrollee::provisionDeviceProperties(const DeviceProp& devProp, DevicePropProvStatusCb callback)
        {
            OIC_LOG(DEBUG,ES_REMOTE_ENROLLEE_TAG,"Enter provisionDeviceProperties");

            if(!callback)
            {
                throw ESInvalidParameterException("Callback is empty");
            }

            m_devicePropProvStatusCb = callback;

            if (m_enrolleeResource == nullptr)
            {
                throw ESBadRequestException ("Device not created");
            }

            if(devProp.WIFI.ssid.empty())
            {
                throw ESBadRequestException ("Invalid Provisiong Data.");
            }

            DevicePropProvStatusCb devicePropProvStatusCb = std::bind(
                    &RemoteEnrollee::devicePropProvisioningStatusHandler, this, std::placeholders::_1);

            m_enrolleeResource->registerDevicePropProvStatusCallback(devicePropProvStatusCb);
            m_enrolleeResource->provisionEnrollee(devProp);
        }

        void RemoteEnrollee::initCloudResource()
        {
            ESResult result = ES_ERROR;

            if (m_cloudResource != nullptr)
            {
                throw ESBadRequestException ("Already created");
            }

            result = discoverResource();

            if (result == ES_ERROR)
            {
                OIC_LOG(ERROR,ES_REMOTE_ENROLLEE_TAG,
                                    "Failed to create resource object using discoverResource");
                throw ESBadRequestException ("Resource object not created");
            }

            else
            {
                if(m_ocResource != nullptr)
                {
                    m_cloudResource = std::make_shared<CloudResource>(std::move(m_ocResource));

                    std::shared_ptr< CloudPropProvisioningStatus > provStatus = std::make_shared<
                        CloudPropProvisioningStatus >(ESResult::ES_OK, ESCloudProvState::ES_CLOUD_ENROLLEE_FOUND);

                    m_cloudPropProvStatusCb(provStatus);
                }
                else
                {
                    throw ESBadGetException ("Resource handle is invalid");
                }
            }
        }


        void RemoteEnrollee::provisionCloudProperties(const CloudProp& cloudProp, CloudPropProvStatusCb callback)
        {
            OIC_LOG(DEBUG,ES_REMOTE_ENROLLEE_TAG,"Enter provisionCloudProperties");

            ESResult result = ES_ERROR;

            if(!callback)
            {
                throw ESInvalidParameterException("Callback is empty");
            }

            m_cloudPropProvStatusCb = callback;

            if(cloudProp.authCode.empty()
                || cloudProp.authProvider.empty()
                || cloudProp.ciServer.empty())
            {
                throw ESBadRequestException ("Invalid Cloud Provisiong Info.");
            }

            try
            {
                initCloudResource();
            }

            catch (const std::exception& e)
            {
                OIC_LOG_V(ERROR, ES_REMOTE_ENROLLEE_TAG,
                    "Exception caught in provisionCloudProperties = %s", e.what());

                std::shared_ptr< CloudPropProvisioningStatus > provStatus = std::make_shared<
                        CloudPropProvisioningStatus >(ESResult::ES_ERROR, ESCloudProvState::ES_CLOUD_ENROLLEE_NOT_FOUND);
                m_cloudPropProvStatusCb(provStatus);
            }

            if (m_cloudResource == nullptr)
            {
                throw ESBadRequestException ("Cloud Resource not created");
            }

            CloudPropProvStatusCb cloudPropProvStatusCb = std::bind(
                    &RemoteEnrollee::cloudPropProvisioningStatusHandler, this, std::placeholders::_1);

            m_cloudResource->registerCloudPropProvisioningStatusCallback(cloudPropProvStatusCb);
            m_cloudResource->provisionEnrollee(cloudProp);
        }

        void RemoteEnrollee::setDevID(const std::string devId)
        {
            m_deviceId = devId;
        }
    }
}

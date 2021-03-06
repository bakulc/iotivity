/******************************************************************
 *
 * Copyright 2015 Samsung Electronics All Rights Reserved.
 *
 *
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ******************************************************************/

#ifndef SIMULATOR_POST_REQUEST_GEN_H_
#define SIMULATOR_POST_REQUEST_GEN_H_

#include "request_generation.h"
#include "query_param_generator.h"
#include "attribute_generator.h"
#include "request_sender.h"

/**
 * @class RequestModel
 */
class RequestModel;
class POSTRequestGenerator : public RequestGeneration
{
    public:
        /**
         * constructor
         * @param[in] id            identity
         * @param[in] ocResource    oc resource
         * @param[in] requestSchema request schema
         * @param[in] callback      callback function
         */
        POSTRequestGenerator(int id, const std::shared_ptr<OC::OCResource> &ocResource,
                             const std::shared_ptr<RequestModel> &requestSchema,
                             ProgressStateCallback callback);

        /** start sending requests */
        void startSending();
        /** stop sending requests */
        void stopSending();

    private:
        /** send all requests */
        void SendAllRequests();
        void onResponseReceived(SimulatorResult result,
                                const SimulatorResourceModel &repModel, const RequestInfo &reqInfo);
        /** completed event */
        void completed();

        bool m_stopRequested;
        std::unique_ptr<std::thread> m_thread;
        std::shared_ptr<RequestModel> m_requestSchema;
        POSTRequestSender m_requestSender;
};

typedef std::shared_ptr<POSTRequestGenerator> POSTRequestGeneratorSP;

#endif



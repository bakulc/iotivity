/******************************************************************
 *
 * Copyright 2016 Samsung Electronics All Rights Reserved.
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

#ifndef JNI_MAP_H_
#define JNI_MAP_H_

#include <jni.h>

/**
 * @class JniMap
 */
class JniMap
{
    public:
        /** constructor */
        JniMap(JNIEnv *env);
        /** This method to add the key and value */
        void put(jobject jKey, jobject jValue);
        /** This method is to get the java object */
        jobject get();

    private:
        JNIEnv *m_env;
        jobject m_hashMap;
};

#endif

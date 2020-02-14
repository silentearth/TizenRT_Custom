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

#ifndef SIMULATOR_RESOURCE_MODEL_SCHEMA_JNI_H_
#define SIMULATOR_RESOURCE_MODEL_SCHEMA_JNI_H_

#include <jni.h>

#include "simulator_resource_model_schema.h"

jobject AttributePropertyToJava(JNIEnv *env,
                                const std::shared_ptr<AttributeProperty> &property);

std::shared_ptr<AttributeProperty> AttributePropertyToCpp(JNIEnv *env,
        jobject &jProperty);

#endif

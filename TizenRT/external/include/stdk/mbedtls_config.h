/****************************************************************************
 *
 * Copyright 2019 Samsung Electronics All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the License.
 *
 ****************************************************************************/
/**
 * \file config.h
 *
 * \brief Customized mbedtls configuration options (set of defines) for STDK
 *
 */
#ifndef _STDK_MBEDTLS_CONFIG_H
#define _STDK_MBEDTLS_CONFIG_H


#define MBEDTLS_CIPHER_PADDING_PKCS7

#define MBEDTLS_ECP_DP_CURVE25519_ENABLED

#define MBEDTLS_SSL_EXTENDED_MASTER_SECRET

#define MBEDTLS_X509_CSR_PARSE_C

#define MBEDTLS_XTEA_C

#endif /* _STDK_MBEDTLS_CONFIG_H */

/*
 * Copyright (c) 2025 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PROJMGRIPCSERVER_H
#define PROJMGRIPCSERVER_H

#include <map>
#include <string>

/**
  * Forward declarations
*/
class ProjMgr;
class ResponseEnvelope;

/**
  * @brief projmgr ipc server
*/
class ProjMgrIpcServer {
public:
  /**
   * @brief class constructor
  */
  ProjMgrIpcServer(void);

  /**
   * @brief class destructor
  */
  ~ProjMgrIpcServer(void);

  /**
   * @brief run ipc server
   * @param project manager instance
   * @return true if terminated successfully, otherwise false
  */
  bool Run(ProjMgr* manager);

protected:
  ProjMgr* m_manager = nullptr;
  std::string ProcessRequest(const std::string& requestBytes, bool& shutdown);
  ResponseEnvelope GetVersion(const std::string& payload);
  ResponseEnvelope Shutdown(const std::string& payload);
  ResponseEnvelope LoadPacks(const std::string& payload);
  ResponseEnvelope LoadSolution(const std::string& payload);
  ResponseEnvelope ListComponents(const std::string& payload);
};

#endif  // PROJMGRIPCSERVER_H

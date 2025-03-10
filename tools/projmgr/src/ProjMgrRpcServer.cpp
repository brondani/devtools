/*
 * Copyright (c) 2025 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ProjMgrRpcServer.h"
#include "ProjMgr.h"
#include "ProductInfo.h"
#include "ProductInfo.h"

#include <jsonrpccxx/server.hpp>
#include <iostream>

using namespace std;
using namespace jsonrpccxx;

static constexpr const char* CONTENT_LENGTH_HEADER = "Content-Length:";

ProjMgrRpcServer::ProjMgrRpcServer(ProjMgr* manager) :
  m_manager(manager) {
}

ProjMgrRpcServer::~ProjMgrRpcServer(void) {
  // Reserved
}

const string ProjMgrRpcServer::GetRequestFromStdinWithLength(void) {
  string line;
  int contentLength = 0;
  const string& header = CONTENT_LENGTH_HEADER;
  while (getline(cin, line) && !line.empty() && !cin.fail()) {
    if (line.find(header) == 0) {
      contentLength = RteUtils::StringToInt(line.substr(header.length()), 0);
    }
  }
  string request(contentLength, '\0');
  cin.read(&request[0], contentLength);
  return request;
}

const string ProjMgrRpcServer::GetRequestFromStdin(void) {
  string jsonData;
  int braces = 0;
  bool inJson = false;
  char c;
  while (cin.get(c)) {
    if (c == '{') {
      braces++;
      inJson = true;
    }
    if (c == '}') {
      braces--;
    }
    if (inJson) {
      jsonData += c;
    }
    if (inJson && braces == 0) {
      break;
    }
  }
  return jsonData;
}

class RpcHandler {
public:
  RpcHandler(ProjMgrRpcServer* server) : m_server(server) {}
  string GetVersion(void);
  bool Shutdown(void);
  bool LoadPacks(void);
  bool LoadSolution(string solution);
protected:
  ProjMgrRpcServer* m_server;
};

bool ProjMgrRpcServer::Run(void) {
  JsonRpc2Server jsonServer;
  RpcHandler handler(this);

  // Register supported RPC functions
  jsonServer.Add("GetVersion"  , GetHandle(&RpcHandler::GetVersion  , handler));
  jsonServer.Add("Shutdown"    , GetHandle(&RpcHandler::Shutdown    , handler));
  jsonServer.Add("LoadPacks"   , GetHandle(&RpcHandler::LoadPacks   , handler));
  jsonServer.Add("LoadSolution", GetHandle(&RpcHandler::LoadSolution, handler));

  while (!m_shutdown) {
    // Get request
    const auto request = m_contextLength ?
      GetRequestFromStdinWithLength() :
      GetRequestFromStdin();

    // Handle request
    const auto response = jsonServer.HandleRequest(request);

    // Send response
    if (m_contextLength) {
      cout << CONTENT_LENGTH_HEADER << response.size() << std::endl << std::endl;
    }
    cout << response << std::flush;
  }

  return true;
}

string RpcHandler::GetVersion(void) {
  return VERSION_STRING;
}

bool RpcHandler::Shutdown(void) {
  m_server->SetShutdown(true);
  return true;
}

bool RpcHandler::LoadPacks(void) {
  m_server->GetManager()->GetWorker().LoadAllRelevantPacks();
  return true;
}

bool RpcHandler::LoadSolution(string solution) {
  //m_server->GetManager()->GetWorker()
  return true;
}


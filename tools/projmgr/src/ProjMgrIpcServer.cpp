/*
 * Copyright (c) 2025 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ProjMgrIpcServer.h"
#include "ProjMgr.h"
#include "ProductInfo.h"

#include "cproto.pb.h"

#include <iostream>
#include <string>

using namespace std;

ContextItem m_globalContext;

ProjMgrIpcServer::ProjMgrIpcServer(void) {
  // Reserved
}

ProjMgrIpcServer::~ProjMgrIpcServer(void) {
  // Reserved
}

ResponseEnvelope ProjMgrIpcServer::GetVersion(const std::string& payload) {
  ResponseEnvelope envelope;
  GetVersionResponse response;
  response.set_version(VERSION_STRING);
  envelope.set_status(true);
  envelope.set_payload(response.SerializeAsString());
  return envelope;
}

ResponseEnvelope ProjMgrIpcServer::Shutdown(const std::string& payload) {
  ResponseEnvelope envelope;
  envelope.set_status(true);
  return envelope;
}

ResponseEnvelope ProjMgrIpcServer::LoadPacks(const std::string& payload) {
  ResponseEnvelope envelope;
  m_manager->GetWorker().LoadPacks(m_globalContext);
  envelope.set_status(true);
  return envelope;
}

ResponseEnvelope ProjMgrIpcServer::LoadSolution(const std::string& payload) {
  LoadSolutionRequest request;
  request.ParseFromString(payload);
  ResponseEnvelope envelope;
  envelope.set_status(true);
  return envelope;
}

ResponseEnvelope ProjMgrIpcServer::ListComponents(const std::string& payload) {
  RteComponentMap installedComponents = m_globalContext.rteActiveTarget->GetFilteredComponents();
  ListComponentsResponse response;
  for (const auto& component : installedComponents) {
    response.add_component(component.first);
  }
  ResponseEnvelope envelope;
  envelope.set_payload(response.SerializeAsString());
  envelope.set_status(true);
  return envelope;
}

std::string ProjMgrIpcServer::ProcessRequest(const std::string& requestBytes, bool& shutdown) {
  // Registry of requests identifiers mapped to handlers
  const unordered_map<int, function<ResponseEnvelope(const string&)>> m_requests = {
    { ID::SHUTDOWN        , [&](const string& payload) { return Shutdown(payload);       } },
    { ID::GET_VERSION     , [&](const string& payload) { return GetVersion(payload);     } },
    { ID::LOAD_PACKS      , [&](const string& payload) { return LoadPacks(payload);      } },
    { ID::LOAD_SOLUTION   , [&](const string& payload) { return LoadSolution(payload);   } },
    { ID::LIST_COMPONENTS , [&](const string& payload) { return ListComponents(payload); } },
  };

  // Parse request
  RequestEnvelope request;
  if (!request.ParseFromString(requestBytes)) {
    ResponseEnvelope errorResponse;
    errorResponse.set_id(ID::NONE);
    errorResponse.set_status(false);
    errorResponse.set_error("Failed to parse request");
    return errorResponse.SerializeAsString();
  }

  // Find the correct request handler
  auto id = m_requests.find(request.id());
  if (id == m_requests.end()) {
    ResponseEnvelope errorResponse;
    errorResponse.set_id(ID::NONE);
    errorResponse.set_status(false);
    errorResponse.set_error("Unknown id: " + request.id());
    return errorResponse.SerializeAsString();
  }

  // Call the request handler
  auto response = id->second(request.payload());
  response.set_id(request.id());

  // Set shutdown flag
  shutdown = (request.id() == ID::SHUTDOWN);

  // Serialize response envelope
  return response.SerializeAsString();
}

#ifdef _WIN32
  #include <fcntl.h>
  #include <io.h>
#endif

bool ProjMgrIpcServer::Run(ProjMgr* manager) {

#ifdef _WIN32
  // Force binary mode (prevent \n to \r\n conversion)
  _setmode(_fileno(stdout), _O_BINARY);
  _setmode(_fileno(stdin), _O_BINARY);
#endif

  GOOGLE_PROTOBUF_VERIFY_VERSION;

  m_manager = manager;
  std::string serializedData;
  uint32_t size;
  bool shutdown = false;

  while (!shutdown) {
    // Read size (4 bytes)
    std::cin.read(reinterpret_cast<char*>(&size), sizeof(size));

    // Read protobuf message
    serializedData.resize(size);
    std::cin.read(&serializedData[0], size);

    // Process request and get response
    auto responseData = ProcessRequest(serializedData, shutdown);

    // Send size
    uint32_t responseSize = responseData.size();
    std::cout.write(reinterpret_cast<char*>(&responseSize), sizeof(responseSize));

    // Send protobuf message
    std::cout.write(responseData.data(), responseSize);
    std::cout.flush();
  }
  return true;
}

/*
 * Copyright (c) 2025 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ProjMgrRpcServer.h"
#include "ProjMgr.h"
#include "ProductInfo.h"
#include <RteFsUtils.h>

#include <jsonrpccxx/server.hpp>
#include <iostream>
#include <optional>
#include <regex>

using namespace std;
using namespace jsonrpccxx;

static constexpr const char* CONTENT_LENGTH_HEADER = "Content-Length:";

ProjMgrRpcServer::ProjMgrRpcServer(ProjMgr& manager) :
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

namespace Args {
  struct Component {
    string id;
    string fromPack;
    optional<string> description;
    optional<string> doc;
    optional<string> implements;
    optional<bool> selected;
    optional<int> instances;
    optional<int> maxInstances;
  };
  struct Api {
    string id;
    optional<string> description;
    optional<string> doc;
  };
  struct Taxonomy {
    string id;
    optional<string> description;
    optional<string> doc;
  };
  struct ComponentsInfo {
    vector<Api> apis;
    vector<Component> components;
    vector<Taxonomy> taxonomy;
  };
  template<class T> void to_json(nlohmann::json& j, const string& key, const T& value) {
    j[key] = value;
  }
  template<class T> void to_json(nlohmann::json& j, const string& key, const optional<T>& opt) {
    if (opt.has_value()) {
      j[key] = opt.value();
    }
  }
  void to_json(nlohmann::json& j, const Component& c) {
    to_json(j, "id"          , c.id           );
    to_json(j, "description" , c.description  );
    to_json(j, "doc"         , c.doc          );
    to_json(j, "from-pack"   , c.fromPack     );
    to_json(j, "implements"  , c.implements   );
    to_json(j, "selected"    , c.selected     );
    to_json(j, "instances"   , c.instances    );
    to_json(j, "maxInstances", c.maxInstances );
  }
  void to_json(nlohmann::json& j, const Api& a) {
    to_json(j, "id"          , a.id);
    to_json(j, "description" , a.description);
    to_json(j, "doc"         , a.doc);
  }
  void to_json(nlohmann::json& j, const Taxonomy& t) {
    to_json(j, "id"          , t.id);
    to_json(j, "description" , t.description);
    to_json(j, "doc"         , t.doc);
  }
  void to_json(nlohmann::json& j, const ComponentsInfo& info) {
    to_json(j, "apis"        , info.apis);
    to_json(j, "components"  , info.components);
    to_json(j, "taxonomy"    , info.taxonomy);
  }
}

class RpcHandler {
public:
  RpcHandler(ProjMgrRpcServer& server) :
    m_server(server),
    m_manager(server.GetManager()),
    m_worker(server.GetManager().GetWorker()) {}

  const string GetVersion(void);
  bool Shutdown(void);
  bool LoadPacks(void);
  bool LoadSolution(const string& solution);
  const Args::ComponentsInfo GetComponentsInfo(const string& context);

protected:
  ProjMgrRpcServer& m_server;
  ProjMgr& m_manager;
  ProjMgrWorker& m_worker;
  ContextItem m_globalContext;
  ContextItem& getContext(const string& context);
};

#include <fstream>

bool ProjMgrRpcServer::Run(void) {
  JsonRpc2Server jsonServer;
  RpcHandler handler(*this);

  // Register supported RPC functions
  jsonServer.Add("GetVersion"           , GetHandle(&RpcHandler::GetVersion           , handler)                  );
  jsonServer.Add("Shutdown"             , GetHandle(&RpcHandler::Shutdown             , handler)                  );
  jsonServer.Add("LoadPacks"            , GetHandle(&RpcHandler::LoadPacks            , handler)                  );
  jsonServer.Add("LoadSolution"         , GetHandle(&RpcHandler::LoadSolution         , handler), { "solution"   });
  jsonServer.Add("GetComponentsInfo"    , GetHandle(&RpcHandler::GetComponentsInfo    , handler), { "context"    });

  while (!m_shutdown && !cin.fail()) {
    // Get request
    const auto request = m_contextLength ?
      GetRequestFromStdinWithLength() :
      GetRequestFromStdin();

    if (request.empty()) {
      continue;
    }

    //ofstream log("d:/work/csolution-rpc-inspector/example/log.txt", fstream::app);
    //log << request << std::endl;

    // Handle request
    const auto response = jsonServer.HandleRequest(request);

    // Send response
    if (m_contextLength) {
      cout << CONTENT_LENGTH_HEADER << response.size() << std::endl << std::endl;
    }
    cout << response << std::flush;

    //log << response << std::endl;
    //log.close();

  }
  return true;
}

ContextItem& RpcHandler::getContext(const string& context) {
  map<string, ContextItem>* contexts = nullptr;
  m_worker.GetContexts(contexts);
  if (context.empty() && contexts->size() > 0) {
    return (*contexts).begin()->second;
  }
  if (contexts->find(context) == contexts->end()) {
    throw JsonRpcException(-4, context + " was not found among loaded contexts");
  }
  return (*contexts)[context];
}

const string RpcHandler::GetVersion(void) {
  return VERSION_STRING;
}

bool RpcHandler::Shutdown(void) {
  m_server.SetShutdown(true);
  return true;
}

bool RpcHandler::LoadPacks(void) {
  return m_worker.LoadPacks(m_globalContext);
}

bool RpcHandler::LoadSolution(const string& solution) {
  const auto csolutionFile = RteFsUtils::MakePathCanonical(solution);
  if (!regex_match(csolutionFile, regex(".*\\.csolution\\.(yml|yaml)"))) {
    throw JsonRpcException(-2, solution + " is not a valid *.csolution.yml file");
  }
  return m_manager.LoadSolution(csolutionFile);
}

const Args::ComponentsInfo RpcHandler::GetComponentsInfo(const string& context) {
  Args::ComponentsInfo componentsInfo;
  const auto contextItem = getContext(context);

  const auto& attributes = contextItem.rteActiveTarget->GetAttributes();
  m_globalContext.rteActiveTarget->SetAttributes(attributes);
  m_globalContext.rteActiveTarget->UpdateFilterModel();

  for (auto& [component, componentItem] : m_globalContext.rteActiveTarget->GetFilteredComponents()) {
    Args::Component c;
    c.id = component;
    c.fromPack = componentItem->GetPackageID(true);
    const auto& description = componentItem->GetDescription();
    if (!description.empty()) {
      c.description = description;
    }
    const auto& doc = componentItem->GetDocFile();
    if (!doc.empty()) {
      c.doc = doc;
    }
    const auto& api = componentItem->GetApi(m_globalContext.rteActiveTarget, true);
    if (api) {
      c.implements = api->ConstructComponentID(true);
    }
    if (contextItem.components.find(component) != contextItem.components.end()) {
      c.selected = true;
      const auto selected = contextItem.components.at(component);
      c.instances = selected.item->instances;
    }
    if (componentItem->HasMaxInstances()) {
      c.maxInstances = componentItem->GetMaxInstances();
    }
    componentsInfo.components.push_back(c);
  }
  for (auto& [_, apiItem] : m_globalContext.rteActiveTarget->GetFilteredApis()) {
    Args::Api a;
    const auto& api = apiItem->GetApi(m_globalContext.rteActiveTarget, true);
    if (api) {
      a.id = api->ConstructComponentID(true);
    }
    const auto& description = apiItem->GetDescription();
    if (!description.empty()) {
      a.description = description;
    }
    const auto& doc = apiItem->GetDocFile();
    if (!doc.empty()) {
      a.doc = doc;
    }
    componentsInfo.apis.push_back(a);
  }
  for (auto& [taxonomy, taxonomyItem] : m_globalContext.rteActiveTarget->GetFilteredModel()->GetTaxonomy()) {
    Args::Taxonomy t;
    t.id = taxonomy;
    const auto& description = taxonomyItem->GetDescription();
    if (!description.empty()) {
      t.description = description;
    }
    const auto& doc = taxonomyItem->GetDocFile();
    if (!doc.empty()) {
      t.doc = doc;
    }
    componentsInfo.taxonomy.push_back(t);
  }
  return componentsInfo;
}

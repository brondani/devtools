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
#include <fstream>
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
  while (cin.get(c) && !cin.fail()) {
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
  template<class T> void to_json(nlohmann::json& j, const string& key, const T& value) {
    j[key] = value;
  }
  template<class T> void to_json(nlohmann::json& j, const string& key, const optional<T>& opt) {
    if (opt.has_value()) {
      j[key] = opt.value();
    }
  }

  // GetPacksInfo
  struct Pack {
    string id;
    optional<string> description;
    optional<string> overview;
    optional<bool> used;
    optional<vector<string>> references;
  };
  struct PacksInfo {
    vector<Pack> packs;
  };
  void to_json(nlohmann::json& j, const Pack& p) {
    to_json(j, "id", p.id);
    to_json(j, "description", p.description);
    to_json(j, "overview", p.overview);
    to_json(j, "used", p.used);
    to_json(j, "references", p.references);
  }
  void to_json(nlohmann::json& j, const PacksInfo& info) {
    to_json(j, "packs", info.packs);
  }

  // GetCoponentsInfo
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

  // ValidateComponents
  struct Condition {
    string expression;
    optional<vector<string>> aggregates;
  };
  struct Result {
    string result;
    string id;
    optional<vector<string>> aggregates;
    optional<vector<Condition>> conditions;    
  };
  struct Results {
    optional<vector<Result>> validation;
  };
  void to_json(nlohmann::json& j, const Condition& c) {
    to_json(j, "expression" , c.expression);
    to_json(j, "aggregates" , c.aggregates);
  }
  void to_json(nlohmann::json& j, const Result& r) {
    to_json(j, "result"     , r.result);
    to_json(j, "id"         , r.id);
    to_json(j, "aggregates" , r.aggregates);
    to_json(j, "conditions" , r.conditions);
  }
  void to_json(nlohmann::json& j, const Results& r) {
    to_json(j, "validation" , r.validation);
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
  const Args::PacksInfo GetPacksInfo(const string& context);
  const Args::ComponentsInfo GetComponentsInfo(const string& context);
  const Args::Results ValidateComponents(const string& context, const StrVec& components);

protected:
  enum Exception
  {
    SOLUTION_NOT_FOUND     = -1,
    SOLUTION_NOT_VALID     = -2,
    SOLUTION_NOT_LOADED    = -3,
    CONTEXT_NOT_FOUND      = -4,
    CONTEXT_NOT_VALID      = -5,
    COMPONENT_NOT_FOUND    = -6,
    COMPONENT_NOT_RESOLVED = -7,
    PACKS_NOT_LOADED       = -8,
    PACKS_LOADING_FAIL     = -9,
    RTE_MODEL_ERROR        = -10,
  };

  ProjMgrRpcServer& m_server;
  ProjMgr& m_manager;
  ProjMgrWorker& m_worker;
  ContextItem m_globalContext;
  bool m_packsLoaded = false;
  bool m_solutionLoaded = false;
  string m_activeContext;
  ContextItem& SelectContext(const string& context);
  void UpdateGlobalContext(const ContextItem& contextItem);
};

bool ProjMgrRpcServer::Run(void) {
  JsonRpc2Server jsonServer;
  RpcHandler handler(*this);

  // Register supported RPC functions
  jsonServer.Add("GetVersion"           , GetHandle(&RpcHandler::GetVersion           , handler)                             );
  jsonServer.Add("Shutdown"             , GetHandle(&RpcHandler::Shutdown             , handler)                             );
  jsonServer.Add("LoadPacks"            , GetHandle(&RpcHandler::LoadPacks            , handler)                             );
  jsonServer.Add("LoadSolution"         , GetHandle(&RpcHandler::LoadSolution         , handler), { "solution"              });
  jsonServer.Add("GetPacksInfo"         , GetHandle(&RpcHandler::GetPacksInfo         , handler), { "context"               });
  jsonServer.Add("GetComponentsInfo"    , GetHandle(&RpcHandler::GetComponentsInfo    , handler), { "context"               });
  jsonServer.Add("ValidateComponents"   , GetHandle(&RpcHandler::ValidateComponents   , handler), { "context", "components" });

  while (!m_shutdown && !cin.fail()) {
    // Get request
    const auto request = m_contextLength ?
      GetRequestFromStdinWithLength() :
      GetRequestFromStdin();

    if (request.empty()) {
      continue;
    }

    ofstream log;
    if (m_debug) {
      log.open(RteFsUtils::GetCurrentFolder(true) + "csolution-rpc-log.txt", fstream::app);
      log << request << std::endl;
    }

    // Handle request
    const auto response = jsonServer.HandleRequest(request);

    // Send response
    if (m_contextLength) {
      cout << CONTENT_LENGTH_HEADER << response.size() << std::endl << std::endl;
    }
    cout << response << std::flush;

    if (m_debug) {
      log << response << std::endl;
      log.close();
    }

  }
  return true;
}

ContextItem& RpcHandler::SelectContext(const string& context) {
  if (!m_solutionLoaded) {
    throw JsonRpcException(SOLUTION_NOT_LOADED, "a valid solution must be loaded before proceeding");
  }
  if (context.empty()) {
    throw JsonRpcException(CONTEXT_NOT_VALID, "'context' argument cannot be empty");
  }
  const auto selected = m_worker.GetSelectedContexts();
  if (find(selected.begin(), selected.end(), context) == selected.end()) {
    throw JsonRpcException(CONTEXT_NOT_FOUND, context + " was not found among selected contexts");
  }
  map<string, ContextItem>* contexts = nullptr;
  m_worker.GetContexts(contexts);
  return (*contexts)[context];
}

void RpcHandler::UpdateGlobalContext(const ContextItem& contextItem) {
  if (m_activeContext != contextItem.name) {
    m_activeContext = contextItem.name;
    // set global context attributes according to selected context
    const auto& attributes = contextItem.rteActiveTarget->GetAttributes();
    m_globalContext.rteActiveTarget->SetAttributes(attributes);
    m_globalContext.rteActiveTarget->UpdateFilterModel();
  }
}

const string RpcHandler::GetVersion(void) {
  return VERSION_STRING;
}

bool RpcHandler::Shutdown(void) {
  m_server.SetShutdown(true);
  return true;
}

bool RpcHandler::LoadPacks(void) {
  m_packsLoaded = m_worker.LoadPacks(m_globalContext);
  if (!m_packsLoaded) {
    throw JsonRpcException(PACKS_LOADING_FAIL, "packs failed to load");
  }
  return true;
}

bool RpcHandler::LoadSolution(const string& solution) {
  if (!m_packsLoaded) {
    throw JsonRpcException(PACKS_NOT_LOADED, "packs must be loaded before proceeding");
  }
  const auto csolutionFile = RteFsUtils::MakePathCanonical(solution);
  if (!regex_match(csolutionFile, regex(".*\\.csolution\\.(yml|yaml)"))) {
    throw JsonRpcException(SOLUTION_NOT_FOUND, solution + " is not a *.csolution.yml file");
  }
  m_solutionLoaded = m_manager.LoadSolution(csolutionFile);
  m_activeContext.clear();
  if (!m_solutionLoaded) {
    throw JsonRpcException(SOLUTION_NOT_VALID, "failed to load and process solution " + csolutionFile);
  }
  return true;
}

const Args::PacksInfo RpcHandler::GetPacksInfo(const string& context) {
  const auto contextItem = SelectContext(context);
  UpdateGlobalContext(contextItem);

  map<string, vector<string>> packRefs;
  for (const auto& packItem : contextItem.packRequirements) {
    if (!packItem.origin.empty()) {
      const auto packId = RtePackage::ComposePackageID(packItem.pack.vendor, packItem.pack.name, packItem.pack.version);
      CollectionUtils::PushBackUniquely(packRefs[packId], packItem.origin);
    }
  }

  Args::PacksInfo packsInfo;
  for (auto& [pack, packItem] : m_globalContext.rteActiveTarget->GetFilteredModel()->GetPackages()) {
    Args::Pack p;
    p.id = packItem->GetPackageID(true);
    const auto& description = packItem->GetDescription();
    if (!description.empty()) {
      p.description = description;
    }
    string overview = packItem->GetChildAttribute("description", "overview");
    if (!overview.empty()) {
      RteFsUtils::NormalizePath(overview, packItem->GetAbsolutePackagePath());
      p.overview = overview;
    }
    if (contextItem.packages.find(p.id) != contextItem.packages.end()) {
      p.used = true;
      if (packRefs.find(p.id) != packRefs.end()) {
        p.references = packRefs.at(p.id);
      }
    }
    packsInfo.packs.push_back(p);
  }

  return packsInfo;
}

const Args::ComponentsInfo RpcHandler::GetComponentsInfo(const string& context) {
  const auto contextItem = SelectContext(context);
  UpdateGlobalContext(contextItem);

  Args::ComponentsInfo componentsInfo;
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
      if (componentItem->HasMaxInstances()) {
        const auto selected = contextItem.components.at(component);
        c.instances = selected.item->instances;
      }
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

const Args::Results RpcHandler::ValidateComponents(const string& context, const StrVec& components) {
  const auto contextItem = SelectContext(context);
  UpdateGlobalContext(contextItem);

  m_globalContext.rteActiveTarget->ClearSelectedComponents();
  m_globalContext.components.clear();

  for (const auto& componentId : components) {
    const auto component = m_globalContext.rteActiveTarget->GetComponent(componentId);
    if (component == NULL) {
      throw JsonRpcException(COMPONENT_NOT_FOUND, componentId + " was not found among filtered components");
    }
    RteComponentInstance* instance = new RteComponentInstance(component);
    instance->InitInstance(component);
    m_globalContext.components.insert({ componentId, { instance, nullptr , ""} });
  }
  if (!m_worker.AddRequiredComponents(m_globalContext)) {
    throw JsonRpcException(COMPONENT_NOT_RESOLVED, "failed to resolve components");
  }
  if (!m_worker.CheckRteErrors()) {
    throw JsonRpcException(RTE_MODEL_ERROR, "rte model reported error");
  }

  Args::Results results;
  if (!m_worker.ValidateContext(m_globalContext)) {
    results.validation = vector<Args::Result>{};
    for (const auto& validation : m_globalContext.validationResults) {
      Args::Result r;
      r.result = RteItem::ConditionResultToString(validation.result);
      r.id = validation.id;
      if (!validation.aggregates.empty()) {
        r.aggregates = vector<string>(validation.aggregates.begin(), validation.aggregates.end());
      }
      if (!validation.conditions.empty()) {
        Args::Condition c;
        r.conditions = vector<Args::Condition>{};
        for (const auto& condition : validation.conditions) {
          c.expression = condition.expression;
          if (!condition.aggregates.empty()) {
            c.aggregates = vector<string>(condition.aggregates.begin(), condition.aggregates.end());
          }
          r.conditions->push_back(c);
        }
      }
      results.validation->push_back(r);
    }
  }
  return results;
}

/*
 * Copyright (c) 2020-2021 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PROJMGR_H
#define PROJMGR_H

#include "ProjMgr.h"
#include "ProjMgrKernel.h"

#include "XMLTreeSlim.h"
#include "yaml-cpp/yaml.h"
#include <string>

struct FileInfo {
  std::string name;
  std::string category;
};

struct GroupInfo {
  std::string name;
  std::set<FileInfo> files;
  std::set<GroupInfo> groups;
};

inline bool operator<(const FileInfo& lhs, const FileInfo& rhs)
{
  return lhs.name < rhs.name;
}

inline bool operator<(const GroupInfo& lhs, const GroupInfo& rhs)
{
  return lhs.name < rhs.name;
}

struct ComponentInfo {
  std::string filter;
  std::map<std::string, std::string> attributes;
};

inline bool operator<(const ComponentInfo& lhs, const ComponentInfo& rhs)
{
  return lhs.filter < rhs.filter;
}

class ProjMgr {
public:
  ProjMgr(void);
  ~ProjMgr(void);
  static int RunProjMgr(int argc, char **argv);
  void PrintUsage();
  bool LoadPacks();
  bool CheckRteErrors();
  bool ListPacks(std::set<std::string>& packs);
  bool ListDevices(std::set<std::string>& devices);
  bool ListComponents(std::set<std::string>& components);
  bool ListDependencies(std::set<std::string>& dependencies);
  bool ParseInput();
  bool ProcessProject();
  bool GenerateCprj();
  bool ProcessDevice();
  bool ProcessComponents();
  bool ProcessDependencies();

protected:
  ProjMgrKernel* m_kernel = nullptr;
  RteGlobalModel* m_model = nullptr;
  RteProject* m_project = nullptr;
  RteTarget* m_target = nullptr;
  std::list<RtePackage*> m_installedPacks;
  std::string m_input;
  std::string m_filter;
  std::string m_command;
  std::string m_args;

  std::string m_projectName;
  std::string m_rootFolder;
  std::string m_infoDescription;
  std::string m_device;
  std::map<std::string, std::string> m_deviceAttributes;
  std::string m_outputFolder;
  std::string m_outputType;
  std::set<RtePackage*> m_packages;
  std::string m_toolchain;
  std::string m_toolchainVersion;
  std::set<ComponentInfo> m_componentDescriptions;
  std::set<RteComponent*> m_components;
  std::set<RteComponentContainer*> m_dependencies;
  GroupInfo m_files;

  void ParseFiles(YAML::Node files, GroupInfo& group);
  void GenerateCprjGroups(XMLTreeElement* element, const GroupInfo& group);
  std::set<std::string> SplitArgs(const std::string& args);
  static void SetAttribute(XMLTreeElement* element, const std::string& name, const std::string& value);
  bool SetTargetAttributes(std::map<std::string, std::string>& attributes);
  void ApplyFilter(const std::set<std::string>& origin, const std::set<std::string>& filter, std::set<std::string>& result);
};

#endif  // PROJMGR_H

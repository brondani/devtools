/*
 * Copyright (c) 2020-2021 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ProjMgr.h"

#include "ProductInfo.h"

#include "CbuildLayer.h"
#include "CbuildUtils.h"

#include "RteFsUtils.h"
#include "XmlFormatter.h"
#include "CrossPlatformUtils.h"

#include <algorithm>
#include <cxxopts.hpp>
#include <iostream>
#include <fstream>
#include <functional>

using namespace std;

static constexpr const char* SCHEMA_FILE = "PACK.xsd";    // XML schema file name
static constexpr const char* SCHEMA_VERSION = "1.7.2";    // XML schema version

static constexpr const char* USAGE = "\
Usage:\n\
  projmgr <command> [<args>] [OPTIONS...]\n\n\
Commands:\n\
  list packs        Print list of installed packs\n\
       devices      Print list of available device names\n\
       components   Print list of available components\n\
       dependencies Print list of unresolved project dependencies\n\
  convert           Convert cproject.yml in cprj file\n\
  help              Print usage\n\n\
Options:\n\
  -i, --input arg   Input YAML file\n\
  -f, --filter arg  Filter words\n\
  -h, --help        Print usage\n\
";

ProjMgr::ProjMgr(void) {
  // Reserved
}

ProjMgr::~ProjMgr(void) {
  // Reserved
}

int ProjMgr::RunProjMgr(int argc, char **argv) {

  ProjMgr manager;

  // Command line options
  cxxopts::Options options(ORIGINAL_FILENAME);
  cxxopts::ParseResult parseResult;

  try {
    options.add_options()
      ("command", "", cxxopts::value<string>())
      ("args", "", cxxopts::value<string>())
      ("i,input", "", cxxopts::value<string>())
      ("f,filter", "", cxxopts::value<string>())
      ("h,help", "")
      ;
    options.parse_positional({ "command", "args"});

    parseResult = options.parse(argc, argv);

    if (parseResult.count("command")) {
      manager.m_command = parseResult["command"].as<string>();
    } else {
      // No command was given, print usage and return success
      manager.PrintUsage();
      return 0;
    }
    if (parseResult.count("args")) {
      manager.m_args = parseResult["args"].as<string>();
    }
    if (parseResult.count("input")) {
      manager.m_input = parseResult["input"].as<string>();
      error_code ec;
      if (!fs::exists(manager.m_input, ec)) {
        cerr << "projmgr error: input file " << manager.m_input << " was not found" << endl;
        return 1;
      }
      manager.m_projectName = fs::path(manager.m_input).stem().stem().generic_string();
      manager.m_rootFolder = fs::path(fs::canonical(manager.m_input, ec)).parent_path().generic_string();
    }
    if (parseResult.count("filter")) {
      manager.m_filter = parseResult["filter"].as<string>();
    }
  } catch (cxxopts::OptionException& e) {
    cerr << "projmgr error: parsing command line failed!" << endl << e.what() << endl;
    return 1;
  }

  // Unmatched items in the parse result
  if (!parseResult.unmatched().empty()) {
    cerr << "projmgr error: too many command line arguments!" << endl;
    return 1;
  }

  // Parse commands
  if ((manager.m_command == "help") || parseResult.count("help")) {
    // Print usage
    manager.PrintUsage();
    return 0;
  } else if (manager.m_command == "list") {
    // Process 'list' command
    if (manager.m_args.empty()) {
      cerr << "projmgr error: list <args> was not specified!" << endl;
      return 1;
    }
    // Load installed packs
    if ((!manager.LoadPacks()) || (!manager.CheckRteErrors()) ){
      return 1;
    }
    // Parse input if present
    if (!manager.m_input.empty() && !manager.ParseInput()) {
      return 1;
    }
    // Process argument
    if (manager.m_args == "packs") {
      set<string>packs;
      if (manager.ListPacks(packs)) {
        for (const auto& pack : packs) {
          cout << pack << endl;
        }
      }
    } else if (manager.m_args == "devices") {
      set<string>devices;
      if (manager.ListDevices(devices)) {
        for (const auto& device : devices) {
          cout << device << endl;
        }
      }
    } else if (manager.m_args == "components") {
      if (manager.ProcessDevice()) {
        set<string>components;
        if (manager.ListComponents(components)) {
          for (const auto& component : components) {
            cout << component << endl;
          }
        }
      }
    } else if (manager.m_args == "dependencies") {
      if (manager.m_input.empty()) {
        cerr << "projmgr error: input YAML file was not specified!" << endl;
        return 1;
      }
      if ((manager.ProcessDevice()) && (manager.ProcessComponents()) && (manager.ProcessDependencies())) {
        set<string>dependencies;
        if (manager.ListDependencies(dependencies)) {
          for (const auto& dependency : dependencies) {
            cout << dependency << endl;
          }
        }
      }
    } else {
      cerr << "projmgr error: list <args> was not found!" << endl;
      return 1;
    }
  } else if (manager.m_command == "convert") {
    // Process 'convert' command
    if (manager.m_input.empty()) {
      cerr << "projmgr error: input YAML file was not specified!" << endl;
      return 1;
    }
    if ((!manager.ParseInput()) || !manager.LoadPacks() ||
       !manager.ProcessProject() || !manager.CheckRteErrors() || !manager.GenerateCprj()) {
      return 1;
    }
  } else {
    cerr << "projmgr error: <command> was not found!" << endl;
    return 1;
  }
  return 0;
}

void ProjMgr::PrintUsage(void) {
  cout << PRODUCT_NAME << " " << VERSION_STRING << " " << COPYRIGHT_NOTICE << " " << endl;
  cout << USAGE << endl;
}

bool ProjMgr::LoadPacks() {
  string packRoot = CrossPlatformUtils::GetEnv("CMSIS_PACK_ROOT");
  if (packRoot.empty()) {
    packRoot = CrossPlatformUtils::GetDefaultCMSISPackRootDir();
  }
  m_kernel = ProjMgrKernel::Get();
  m_kernel->SetCmsisPackRoot(packRoot);
  // Get installed packs
  if (!m_kernel->GetInstalledPacks(m_installedPacks)) {
    cerr << "projmgr error: parsing installed packs failed!" << endl;
  }
  return true;
}

bool ProjMgr::CheckRteErrors() {
  const list<string>& rteErrorMessages = m_kernel->GetCallback()->GetErrorMessages();
  if (!rteErrorMessages.empty()) {
    for (const auto& rteErrorMessage : rteErrorMessages) {
      cerr << rteErrorMessage << endl;
    }
    return false;
  }
  return true;
}

bool ProjMgr::SetTargetAttributes(map<string, string>& attributes) {
  if (m_project == nullptr) {
    m_model = m_kernel->GetGlobalModel();
    m_project = new RteProject();
    m_model->AddProject(0, m_project);
    m_model->SetActiveProjectId(m_project->GetProjectId());
    m_project = m_model->GetActiveProject();
    m_project->AddTarget("CMSIS", attributes, true, true);
    m_project->SetActiveTarget("CMSIS");
    m_target = m_project->GetActiveTarget();
  }
  else {
    m_target->SetAttributes(attributes);
    m_target->UpdateFilterModel();
  }
  return true;
}

void ProjMgr::ApplyFilter(const set<string>& origin, const set<string>& filter, set<string>& result) {
  result.clear();
  for (const auto& word : filter) {
    if (result.empty()) {
      for (const auto& item : origin) {
        if (item.find(word) != string::npos) {
          result.insert(item);
        }
      }
    } else {
      const auto temp = result;
      for (const auto& item : temp) {
        if (item.find(word) == string::npos) {
          result.erase(result.find(item));
        }
        if (result.empty()) {
          return;
        }
      }
    }
  }
}

bool ProjMgr::ListPacks(set<string>& packs) {
  if (m_installedPacks.empty()) {
    cerr << "projmgr error: no installed pack was found" << endl;
    return false;
  }
  for (const auto& pack : m_installedPacks) {
    packs.insert(pack->GetPackageID());
  }
  if (!m_filter.empty()) {
    set<string> filteredPacks;
    ApplyFilter(packs, SplitArgs(m_filter), filteredPacks);
    if (filteredPacks.empty()) {
      cerr << "projmgr error: no pack was found with filter '" << m_filter << "'" << endl;
      return false;
    }
    packs = filteredPacks;
  }
  return true;
}

bool ProjMgr::ListDevices(set<string>& devices) {
  for (const auto& pack : m_installedPacks) {
    list<RteDeviceItem*> deviceItems;
    pack->GetEffectiveDeviceItems(deviceItems);
    for (const auto& deviceItem : deviceItems) {
      devices.insert(deviceItem->GetFullDeviceName());
    }
  }
  if (devices.empty()) {
    cerr << "projmgr error: no installed device was found" << endl;
    return false;
  }  
  if (!m_filter.empty()) {
    set<string> filteredDevices;
    ApplyFilter(devices, SplitArgs(m_filter), filteredDevices);
    if (filteredDevices.empty()) {
      cerr << "projmgr error: no device was found with filter '" << m_filter << "'" << endl;
      return false;
    }
    devices = filteredDevices;
  }
  return true;
}

bool ProjMgr::ListComponents(set<string>& components) {
  if (m_device.empty()) {
    for (const auto& pack : m_installedPacks) {
      if (pack->GetComponents()) {
        const auto& packComponents = pack->GetComponents()->GetChildren();
        for (const auto& packComponent : packComponents) {
          components.insert(packComponent->GetComponentUniqueID(true));
        }
      }
    }
    if (components.empty()) {
      cerr << "projmgr error: no installed component was found" << endl;
      return false;
    }
  } else {
    RteComponentMap componentMap = m_target->GetFilteredComponents();
    for (const auto& component : componentMap) {
      components.insert(component.second->GetComponentUniqueID(true));
    }
    if (components.empty()) {
      cerr << "projmgr error: no filtered component was found for device '" << m_device << "'" << endl;
      return false;
    }
  }
  if (!m_filter.empty()) {
    set<string> filteredComponents;
    ApplyFilter(components, SplitArgs(m_filter), filteredComponents);
    if (filteredComponents.empty()) {
      cerr << "projmgr error: no component was found with filter '" << m_filter << "'" << endl;
      return false;
    }
    components = filteredComponents;
  }
  return true;
}

bool ProjMgr::ListDependencies(set<string>& dependencies) {
  for (const auto& dependency : m_dependencies) {
    dependencies.insert(dependency->GetComponentAggregateID());
  }
  if (!dependencies.empty() && !m_filter.empty()) {
    set<string> filteredDependencies;
    ApplyFilter(dependencies, SplitArgs(m_filter), filteredDependencies);
    if (filteredDependencies.empty()) {
      cerr << "projmgr error: no unresolved dependency was found with filter '" << m_filter << "'" << endl;
      return false;
    }
    dependencies = filteredDependencies;
  }
  return true;
}

bool ProjMgr::ParseInput() {
  try {
    YAML::Node input = YAML::LoadFile(m_input);

    YAML::Node project = input["project"];

    m_device = project["device"].IsDefined() ? project["device"].as<string>() : "";
    if (project["attributes"].IsDefined()) {
      // Attributes
      map<string, string> deviceAttributes;
      YAML::Node attributes = project["attributes"];
      for (const auto& attr : attributes) {
        m_deviceAttributes.insert({ attr.first.as<string>(), attr.second.as<string>() });
      }
    }
    m_outputType = project["type"].IsDefined() ? project["type"].as<string>() : "exe";
    m_outputFolder = project["output"].IsDefined() ? project["output"].as<string>() : "output";

    YAML::Node compiler = input["compiler"];
    m_toolchain = compiler["name"].as<string>();
    m_toolchainVersion = compiler["version"].as<string>();

    YAML::Node components = input["components"];
    for (const auto& item : components) {
      ComponentInfo description;
      if (item["filter"].IsDefined()) {
        description.filter = item["filter"].as<string>();
      }
      if (item["attributes"].IsDefined()) {
        // Attributes
        map<string, string> deviceAttributes;
        YAML::Node attributes = item["attributes"];
        for (const auto& attr : attributes) {
          description.attributes.insert({ attr.first.as<string>(), attr.second.as<string>() });
        }
      }
      m_componentDescriptions.insert(description);
    }

    YAML::Node files = input["files"];
    ParseFiles(files, m_files);

  } catch (YAML::Exception& e) {
    cerr << "projmgr error: check YAML file!" << endl << e.what() << endl;
    return false;
  }

  return true;
}

void ProjMgr::ParseFiles(YAML::Node node, GroupInfo& group) {
  for (const auto& item : node) {
    // File node
    if (item["file"].IsDefined()) {
      FileInfo file = { item["file"].as<string>() };
      if (item["category"].IsDefined()) {
        file.category = item["category"].as<string>();
      }
      group.files.insert(file);
    }

    // Group node
    if (item["group"].IsDefined()) {
      GroupInfo subgroup = { item["group"].as<string>() };
      GroupInfo& reference = const_cast<GroupInfo&>(*(group.groups.insert(subgroup).first));

      // Parse children recursively
      if (item["files"].IsDefined()) {
        YAML::Node files = item["files"];
        ParseFiles(files, reference);
      }
    }
  }
}

bool ProjMgr::ProcessDevice() {
  list<RteDeviceItem*> devices;
  for (const auto& pack : m_installedPacks) {
    list<RteDeviceItem*> deviceItems;
    pack->GetEffectiveDeviceItems(deviceItems);
    devices.insert(devices.end(), deviceItems.begin(), deviceItems.end());
  }
  list<RteDeviceItem*> filteredDevices;
  for (const auto& device : devices) {
    if (device->GetDeviceName() == m_device) {
      filteredDevices.push_back(device);
    }
  }
  RteDeviceItem* filteredDevice = nullptr;
  for (const auto& item : filteredDevices) {
    if ((!filteredDevice) || (VersionCmp::Compare(filteredDevice->GetPackage()->GetVersionString(), item->GetPackage()->GetVersionString()) < 0)) {
      filteredDevice = item;
    }
  }
  if (!filteredDevice) {
    cerr << "projmgr error: no device was found" << endl;
    return false;
  }

  for (const auto& processor : filteredDevice->GetProcessors()) {
    const auto& attributes = processor.second->GetAttributes();
    m_deviceAttributes.insert(attributes.begin(), attributes.end());
    // TODO: handle multiple processors
    break;
  }
  m_deviceAttributes["Dvendor"] = filteredDevice->GetEffectiveAttribute("Dvendor");
  m_deviceAttributes["Dname"] = m_device;

  map<string, string> attributes;
  attributes.insert(m_deviceAttributes.begin(), m_deviceAttributes.end());
  if (m_toolchain == "AC6" || m_toolchain == "AC5") {
    attributes["Tcompiler"] = "ARMCC";
    attributes["Toptions"] = m_toolchain;
  } else {
    attributes["Tcompiler"] = m_toolchain;
  }
  if (!SetTargetAttributes(attributes)) {
    return false;
  }

  m_packages.insert(filteredDevice->GetPackage());

  return true;
}

bool ProjMgr::ProcessComponents() {
  RteComponentMap componentMap = m_target->GetFilteredComponents();
  set<string> componentIds, filteredIds;
  for (const auto& component : componentMap) {
    componentIds.insert(component.first);
  }

  for (const auto& description : m_componentDescriptions) {
    RteComponentMap filteredComponents, filteredComponentsByAttribute;

    // Filter components by filter words
    if (!description.filter.empty()) {
      set<string> filteredIds;
      ApplyFilter(componentIds, SplitArgs(description.filter), filteredIds);
      filteredComponents.clear();
      for (const auto& filteredId : filteredIds) {
        filteredComponents[filteredId] = componentMap[filteredId];
      }
    }

    // Filter components by attributes
    if (!description.attributes.empty()) {
      for (const auto& component : (filteredComponents.empty() ? componentMap : filteredComponents)) {
        bool match = true;
        for (const auto& attribute : description.attributes) {
          auto  attr = component.second->GetAttribute(attribute.first);
          if (component.second->GetAttribute(attribute.first) != attribute.second) {
            match = false;
            break;
          }
        }
        if (match) {
          filteredComponentsByAttribute.insert(component);
        }
      }
    }

    // Evaluate filtered components
    RteComponentMap& componentsRef = filteredComponentsByAttribute.empty() ? filteredComponents.empty() ?
                                     componentMap : filteredComponents : filteredComponentsByAttribute;
    if (componentsRef.size() == 1) {
      // Single match
      m_components.insert(componentsRef.begin()->second);
      m_packages.insert(componentsRef.begin()->second->GetPackage());
    } else if (componentsRef.empty()) {
      // No match
      cerr << "projmgr error: no component was found with";
      if (!description.filter.empty()) {
        cerr << " filter '" << description.filter << "'";
      }
      if (!description.attributes.empty()) {
        cerr << " attributes";
        for (const auto& attribute : description.attributes) {
          cerr << " '" << attribute.first << ": " << attribute.second << "'";
        }
      }
      cerr << endl;
      return false;
    } else {
      // Multiple matches
      cerr << "projmgr error: multiple components were found:" << endl;
      for (const auto& component : componentsRef) {
        cerr << component.first << endl;
      }
      return false;
    }
  }

  // Add components into RTE
  set<RteComponentInstance*> unresolvedComponents;
  const list<RteItem*> selItems(begin(m_components), end(m_components));
  m_project->AddCprjComponents(selItems, m_target, unresolvedComponents);
  if (!unresolvedComponents.empty()) {
    cerr << "projmgr error: unresolved components:" << endl;
    for (const auto& component : unresolvedComponents) {
      cerr << component->GetComponentUniqueID(true) << endl;
    }
    return false;
  }
  return true;
}

bool ProjMgr::ProcessDependencies() {
  m_project->ResolveDependencies(m_target);
  const auto& selected = m_target->GetSelectedComponentAggregates();
  for (const auto& component : selected) {
    RteComponentContainer* c = component.first;
    string componentAggregate = c->GetComponentAggregateID();
    const auto& match = find_if(m_components.begin(), m_components.end(),
      [componentAggregate](auto component) {
        return component->GetComponentAggregateID() == componentAggregate;
      });
    if (match == m_components.end()) {
      m_dependencies.insert(c);
    }
  }
  if (selected.size() != (m_components.size() + m_dependencies.size())) {
    cerr << "projmgr error: resolving dependencies failed" << endl;
    return false;
  }
  return true;
}

bool ProjMgr::ProcessProject() {
  if ((!ProcessDevice()) || (!ProcessComponents()) || (!ProcessDependencies())) {
    return false;
  }
  if (!m_dependencies.empty()) {
    cerr << "projmgr error: missing dependencies:" << endl;
      for (const auto& dependency : m_dependencies) {
        cerr << dependency->GetComponentAggregateID() << endl;
      }
    return false;
  }
  return true;
}

bool ProjMgr::GenerateCprj() {
  // Root
  XMLTree* cprjTree = new XMLTreeSlim();
  XMLTreeElement* rootElement;
  rootElement = cprjTree->CreateElement("cprj");

  // Created
  const string& tool = string(ORIGINAL_FILENAME) + " " + string(VERSION_STRING);
  const string& timestamp = CbuildUtils::GetLocalTimestamp();
  XMLTreeElement* createdElement = rootElement->CreateElement("created");
  createdElement->AddAttribute("tool", tool);
  createdElement->AddAttribute("timestamp", string(timestamp));

  // Info
  XMLTreeElement* infoElement = rootElement->CreateElement("info");
  infoElement->AddAttribute("isLayer", "false");
  const string& infoDescription = m_infoDescription.empty() ? "Automatically generated project" : m_infoDescription;
  XMLTreeElement* infoDescriptionElement = infoElement->CreateElement("description");
  infoDescriptionElement->SetText(infoDescription);

  // Create first level elements
  XMLTreeElement* packagesElement = rootElement->CreateElement("packages");
  XMLTreeElement* compilersElement = rootElement->CreateElement("compilers");
  XMLTreeElement* targetElement = rootElement->CreateElement("target");
  XMLTreeElement* componentsElement = rootElement->CreateElement("components");
  XMLTreeElement* filesElement = rootElement->CreateElement("files");

  // Packages
  for (const auto& package : m_packages) {
    XMLTreeElement* packageElement = packagesElement->CreateElement("package");
    packageElement->AddAttribute("name", package->GetName());
    packageElement->AddAttribute("vendor", package->GetVendorName());
    packageElement->AddAttribute("version", package->GetVersionString());
  }

  // Compilers
  XMLTreeElement* compilerElement = compilersElement->CreateElement("compiler");
  compilerElement->AddAttribute("name", m_toolchain);
  if (!m_toolchainVersion.empty()) {
    compilerElement->AddAttribute("version", m_toolchainVersion);
  }

  // Target
  static constexpr const char* DEVICE_ATTRIBUTES[] = { "Ddsp", "Dfpu", "Dmve", "Dsecure", "Dtz", "Dvendor" };
  for (const auto& name : DEVICE_ATTRIBUTES) {
    const string& value = m_deviceAttributes[name];
    SetAttribute(targetElement, name, value);
  }
 
  targetElement->AddAttribute("Dname",   m_device);
  targetElement->AddAttribute("Dendian", "Little-endian");
  XMLTreeElement* targetOutputElement = targetElement->CreateElement("output");
  targetOutputElement->AddAttribute("name", m_projectName);
  targetOutputElement->AddAttribute("type", m_outputType);
  targetOutputElement->AddAttribute("outdir", m_outputFolder);
  targetOutputElement->AddAttribute("intdir", m_outputFolder);

  // Components
  static constexpr const char* COMPONENT_ATTRIBUTES[] = { "Cbundle", "Cclass", "Cgroup", "Csub", "Cvariant", "Cvendor", "Cversion" };
  for (const auto& component : m_components) {
    XMLTreeElement* componentElement = componentsElement->CreateElement("component");
    for (const auto& name : COMPONENT_ATTRIBUTES) {
      const string& value = component->GetAttribute(name);
      SetAttribute(componentElement, name, value);
    }
  }

  // Files
  GenerateCprjGroups(filesElement, m_files);

  // Save CPRJ
  const string& filename = m_rootFolder + "/" + m_projectName + ".cprj";
  if (!CbuildLayer::WriteXmlFile(filename, cprjTree)) {
    cerr << "projmgr error: " << filename << " file cannot be written!" << endl;
    return false;
  }
  return true;
}

void ProjMgr::GenerateCprjGroups(XMLTreeElement* element, const GroupInfo& group) {
  if (!group.name.empty()) {
    element->AddAttribute("name", group.name);
  }
  for (const auto& file : group.files) {
    XMLTreeElement* fileElement = element->CreateElement("file");
    fileElement->AddAttribute("name", file.name);
    fileElement->AddAttribute("category", file.category);
  }
  for (const auto& group : group.groups) {
    XMLTreeElement* groupElement = element->CreateElement("group");
    GenerateCprjGroups(groupElement, group);
  }
}

void ProjMgr::SetAttribute(XMLTreeElement* element, const string& name, const string& value) {
  if (!value.empty()) {
    element->AddAttribute(name, value);
  }
}

set<string> ProjMgr::SplitArgs(const string& args) {
  set<string> s;
  size_t end = 0, start = 0, len = args.length();
  while (end < len) {
    if ((end = args.find(" ", start)) == string::npos) end = len;
    s.insert(args.substr(start, end - start));
    start = end + 1;
  }
  return s;
}

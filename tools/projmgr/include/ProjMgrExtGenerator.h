/*
 * Copyright (c) 2020-2023 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PROJMGREXTGENERATOR_H
#define PROJMGREXTGENERATOR_H

#include "ProjMgrParser.h"
#include "ProjMgrUtils.h"

 /**
  * @brief external generator item containing
  *        component identifier
  *        directory for generated files
  *        project type
 */
struct ExtGeneratorItem {
  std::string componentId;
  std::string genDir;
  std::string projectType;
};


struct CbuildGenItem {
  StrVec forContext;
  std::string device;
  std::string board;
  std::string projectPart;
};

/**
 * @brief map of used generators, directories and contexts
*/
typedef std::map<std::string, StrVecMap> GeneratorContextVecMap;

/**
 * @brief vector of external generator items
*/
typedef std::vector<ExtGeneratorItem> ExtGeneratorVec;

/**
 * @brief map of vector of external generator items
*/
typedef std::map<std::string, ExtGeneratorVec> ExtGeneratorVecMap;

/**
 * @brief solution/project types
*/
static constexpr const char* TYPE_SINGLE_CORE = "single-core";
static constexpr const char* TYPE_MULTI_CORE = "multi-core";
static constexpr const char* TYPE_TRUSTZONE = "trustzone";
static constexpr const char* PROCESSOR_CORE = "processor-core";

/**
 * @brief projmgr external generator class responsible for handling global generators
*/
class ProjMgrExtGenerator {
public:
  /**
   * @brief class constructor
  */
  ProjMgrExtGenerator(ProjMgrParser* parser);


  /**
   * @brief class destructor
  */
  ~ProjMgrExtGenerator(void);


  bool RetrieveGlobalGenerators(void);

  bool IsGlobalGenerator(const std::string& generatorId);
  bool CheckGeneratorId(const std::string& generatorId, const std::string& componentId);
  const std::string& GetGlobalGenDir(const std::string& generatorId);
  const std::string& GetGlobalGenRunCmd(const std::string& generatorId);
  void AddUsedGenerator(const std::string& generatorId, const std::string& genDir, const std::string& contextId);
  const GeneratorContextVecMap& GetUsedGenerators(void);
  bool GetCgen(const std::string& contextId, ClayerItem& cgen);

protected:
  ProjMgrParser* m_parser = nullptr;
  StrVec m_globalGeneratorFiles;
  std::map<std::string, GlobalGeneratorItem> m_globalGenerators;
  GeneratorContextVecMap m_usedGenerators;
  
  bool m_checkSchema;
  std::string m_compilerRoot;



  
};

#endif  // PROJMGREXTGENERATOR_H

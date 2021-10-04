/*
 * Copyright (c) 2020-2021 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ProjMgrKernel.h"
#include "ProjMgrCallback.h"

#include "RteFsUtils.h"
#include "RteKernel.h"
#include "RteValueAdjuster.h"
#include "XMLTreeSlim.h"

#include <iostream>

using namespace std;

// Singleton kernel object
static ProjMgrKernel *theProjMgrKernel = 0;

ProjMgrKernel::ProjMgrKernel(RteCallback* callback) : RteKernel(callback) {
  m_callback = dynamic_cast<ProjMgrCallback*>(callback);
}

ProjMgrKernel::~ProjMgrKernel() {
  if (m_callback)
    delete m_callback;
  m_callback = 0;
}

ProjMgrKernel *ProjMgrKernel::Get() {
  if (!theProjMgrKernel) {
    ProjMgrCallback* cb = new ProjMgrCallback();
    theProjMgrKernel = new ProjMgrKernel(cb);
  }
  return theProjMgrKernel;
}

void ProjMgrKernel::Destroy() {
  if (theProjMgrKernel)
    delete theProjMgrKernel;
  theProjMgrKernel = 0;
}

class ProjMgrXmlParser : public XMLTreeSlim
{
public:
  ProjMgrXmlParser() :XMLTreeSlim(0, true) { m_XmlValueAdjuster = new RteValueAdjuster(); }
  ~ProjMgrXmlParser() { delete m_XmlValueAdjuster; }
};

XMLTree* ProjMgrKernel::CreateXmlTree() const
{
  return new ProjMgrXmlParser();
}

bool ProjMgrKernel::GetInstalledPacks(list<RtePackage*>& packs) {
  list<string> pdscFiles;
  RteFsUtils::GetPackageDescriptionFiles(pdscFiles, theProjMgrKernel->GetCmsisPackRoot(), 3);
  for (const auto& pdscFile : pdscFiles) {
    RtePackage* pack = theProjMgrKernel->LoadPack(pdscFile);
    if (!pack) {
      return false;
    }
    packs.push_back(pack);
  }
  theProjMgrKernel->GetGlobalModel()->InsertPacks(packs);
  return true;
}

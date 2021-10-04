/*
 * Copyright (c) 2020-2021 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PROJMGRKERNEL_H
#define PROJMGRKERNEL_H

#include "ProjMgrCallback.h"
#include "RteKernel.h"

class ProjMgrKernel : public RteKernel
{
public:
  ProjMgrKernel(RteCallback* callback);
  ~ProjMgrKernel();

  /**
   * @brief get kernel
   * @return pointer to singleton kernel instance
  */
  static ProjMgrKernel *Get();

  /**
   * @brief destroy kernel
  */
  static void Destroy();


  bool GetInstalledPacks(std::list<RtePackage*>& packs);

  /**
   * @brief get callback
   * @return pointer to callback
  */
  const ProjMgrCallback* GetCallback() {
    return m_callback;
  }

protected:
  XMLTree* CreateXmlTree() const;

private:
  ProjMgrCallback *m_callback;
};

#endif /* PROJMGRKERNEL_H */

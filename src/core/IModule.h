/*
 * IModule.h
 *
 *  Base Interface for Drivers and Modules
 */

#ifndef CORE_IMODULE_H_
#define CORE_IMODULE_H_

class IModule {
public:
  virtual ~IModule() {}
  virtual void init() = 0;   // 初期化処理
  virtual void update() = 0; // 周期処理
};

#endif /* CORE_IMODULE_H_ */

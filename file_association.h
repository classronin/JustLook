#ifndef FILE_ASSOCIATION_H
#define FILE_ASSOCIATION_H

#include <windows.h>
#include <vector>
#include <string>

// 注册文件关联
bool RegisterFileAssociations();

// 取消文件关联
bool UnregisterFileAssociations();

// 检查是否已注册
bool IsFileAssociationsRegistered();

#endif // FILE_ASSOCIATION_H

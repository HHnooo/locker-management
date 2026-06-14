#pragma once
#include "NormalLocker.h"
#include "VIPLocker.h"
#include <mutex>

// 储物柜管理器：管理12个柜子（8普通+4VIP），提供存取查统操作，每个柜子有独立互斥锁
class LockerManager {
private:
    NormalLocker normalLockers[2][4];          // 普通柜数组：2行×4列
    VIPLocker vipLockers[1][4];                // VIP柜数组：1行×4列（第3行）
    mutable std::mutex normalMutexes_[2][4];   // 普通柜的互斥锁，与柜子一一对应
    mutable std::mutex vipMutexes_[1][4];      // VIP柜的互斥锁，与柜子一一对应

public:
    // 初始化12个柜子，打印完成信息
    LockerManager();

    // 显示所有柜子的占用/空闲状态
    void showAllStatus();

    // 存物：row行col列，VIP柜需传6位数字密码
    bool storeItem(int row, int col, string userName, string pwd = "");

    // 取物：验证身份后释放柜子，VIP需额外验证密码
    bool takeItem(int row, int col, string userName, string pwd = "");

    // 统计并打印普通柜和VIP柜的空闲数量
    void countFree();

    // 打印功能选择菜单
    void showMenu();
};
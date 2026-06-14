#pragma once
#include "Locker.h"

// 普通储物柜：继承Locker，通过姓名验证存取权限
class NormalLocker : public Locker {
public:
    // 构造：调用父类Locker初始化行号列号
    NormalLocker(int r = 0, int c = 0);

    // 存物：柜子空闲且姓名非空才能存，返回true成功/false失败
    bool store(string userName);

    // 取物：柜子占用且姓名匹配才能取，返回true成功/false失败
    bool take(string userName);
};
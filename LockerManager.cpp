#include "LockerManager.h"
#include <iostream>
#include <mutex>
#include <cstdlib>
#include <thread>
#include <chrono>
using namespace std;

// 测试用：若设置了环境变量LOCKER_TEST_DELAY则每次操作延迟100ms
static void testDelay() {
    if (getenv("LOCKER_TEST_DELAY"))
        this_thread::sleep_for(chrono::milliseconds(100));
}

// 初始化普通柜(1-1到2-4)和VIP柜(3-1到3-4)，全部空闲
LockerManager::LockerManager() {
    for (int r = 0; r < 2; r++) {
        for (int c = 0; c < 4; c++) {
            normalLockers[r][c] = NormalLocker(r + 1, c + 1);
        }
    }
    for (int c = 0; c < 4; c++) {
        vipLockers[0][c] = VIPLocker(3, c + 1);
    }
    cout << "校园储物柜管理系统初始化完成！共12个柜子（普通柜8个、VIP柜4个），所有柜子处于空闲状态。" << endl;
}

// 遍历普通柜和VIP柜，加锁后打印每个柜子的状态
void LockerManager::showAllStatus() {
    cout << "\n【普通储物柜】" << endl;
    for (int r = 0; r < 2; r++) {
        for (int c = 0; c < 4; c++) {
            std::unique_lock lock(normalMutexes_[r][c]);  // 加锁
            normalLockers[r][c].showStatus();
        }
    }
    cout << "\n【VIP储物柜】" << endl;
    for (int c = 0; c < 4; c++) {
        std::unique_lock lock(vipMutexes_[0][c]);  // 加锁
        vipLockers[0][c].showStatus();
    }
}

// 存物入口：校验范围→校验姓名→加锁→检查占用→执行存物
bool LockerManager::storeItem(int row, int col, string userName, string pwd) {
    // 校验行号(1~3)和列号(1~4)
    if (row < 1 || row > 3 || col < 1 || col > 4) {
        cout << "错误：行号或列号超出范围！" << endl;
        return false;
    }
    // 校验姓名非空
    if (userName.empty()) {
        cout << "错误：姓名不能为空！" << endl;
        return false;
    }

    // 普通柜（行1或2）
    if (row == 1 || row == 2) {
        std::unique_lock lock(normalMutexes_[row - 1][col - 1]);
        if (normalLockers[row - 1][col - 1].getIsUsed()) {
            cout << "错误：该储物柜已被占用，无法存物！" << endl;
            return false;
        }
        normalLockers[row - 1][col - 1].store(userName);
        cout << "存物成功！" << endl;
        testDelay();
        return true;
    }

    // VIP柜（行3）
    if (row == 3) {
        // 校验密码为6位纯数字
        if (pwd.size() != 6) {
            cout << "错误：密码必须为6位纯数字！" << endl;
            return false;
        }
        for (int i = 0; i < 6; i++) {
            if (pwd[i] < '0' || pwd[i] > '9') {
                cout << "错误：密码必须为6位纯数字！" << endl;
                return false;
            }
        }
        std::unique_lock lock(vipMutexes_[0][col - 1]);
        if (vipLockers[0][col - 1].getIsUsed()) {
            cout << "错误：该VIP储物柜已被占用，无法存物！" << endl;
            return false;
        }
        vipLockers[0][col - 1].storeVIP(userName, pwd);
        cout << "VIP存物成功！" << endl;
        testDelay();
        return true;
    }

    return false;
}

// 取物入口：校验范围→校验姓名→加锁→检查占用→验证身份→取物
bool LockerManager::takeItem(int row, int col, string userName, string pwd) {
    // 校验行号列号
    if (row < 1 || row > 3 || col < 1 || col > 4) {
        cout << "错误：行号或列号超出范围！" << endl;
        return false;
    }
    // 校验姓名非空
    if (userName.empty()) {
        cout << "错误：姓名不能为空！" << endl;
        return false;
    }

    // 普通柜（行1或2）
    if (row == 1 || row == 2) {
        std::unique_lock lock(normalMutexes_[row - 1][col - 1]);
        if (!normalLockers[row - 1][col - 1].getIsUsed()) {
            cout << "错误：该储物柜为空闲状态，无法取物！" << endl;
            return false;
        }
        if (normalLockers[row - 1][col - 1].getUser() != userName) {
            cout << "错误：姓名不匹配，取物失败！" << endl;
            return false;
        }
        normalLockers[row - 1][col - 1].take(userName);
        cout << "取物成功！" << endl;
        testDelay();
        return true;
    }

    // VIP柜（行3）
    if (row == 3) {
        std::unique_lock lock(vipMutexes_[0][col - 1]);
        if (!vipLockers[0][col - 1].getIsUsed()) {
            cout << "错误：该VIP储物柜为空闲状态，无法取物！" << endl;
            return false;
        }
        if (vipLockers[0][col - 1].getUser() != userName) {
            cout << "错误：姓名不匹配，取物失败！" << endl;
            return false;
        }
        if (!vipLockers[0][col - 1].verifyPassword(pwd)) {
            cout << "错误：密码错误，取物失败！" << endl;
            return false;
        }
        vipLockers[0][col - 1].takeVIP(userName, pwd);
        cout << "VIP取物成功！" << endl;
        testDelay();
        return true;
    }

    return false;
}

// 统计空闲：遍历所有柜子（加锁），分别计数普通柜和VIP柜的空闲数
void LockerManager::countFree() {
    int normalFree = 0;
    int vipFree = 0;

    for (int r = 0; r < 2; r++) {
        for (int c = 0; c < 4; c++) {
            std::unique_lock lock(normalMutexes_[r][c]);
            if (!normalLockers[r][c].getIsUsed()) normalFree++;
        }
    }
    for (int c = 0; c < 4; c++) {
        std::unique_lock lock(vipMutexes_[0][c]);
        if (!vipLockers[0][c].getIsUsed()) vipFree++;
    }

    cout << "普通柜空闲数量：" << normalFree << endl;
    cout << "VIP柜空闲数量：" << vipFree << endl;
    cout << "总计空闲数量：" << normalFree + vipFree << endl;
}

// 打印5个功能选项
void LockerManager::showMenu() {
    cout << "\n1. 查看所有储物柜状态" << endl;
    cout << "2. 存物品" << endl;
    cout << "3. 取物品" << endl;
    cout << "4. 统计空闲储物柜数量" << endl;
    cout << "5. 退出系统" << endl;
    cout << "请输入操作编号（1-5）：";
}
#pragma once
#include <string>
using namespace std;

// 储物柜基类：记录柜子位置、占用状态和使用人
class Locker {
private:
    int row;        // 行号（1~3）
    int col;        // 列号（1~4）
    bool isUsed;    // 是否被占用（true占用/false空闲）
    string user;    // 使用人姓名，空闲时为""

public:
    // 构造：传入行号列号，默认空闲、使用人为空
    Locker(int r = 0, int c = 0);

    // 打印柜子编号和占用/空闲状态
    void showStatus();

    // 返回是否被占用
    bool getIsUsed();

    // 返回使用人姓名
    string getUser();

    // 设置使用人并标记为占用
    void setUser(string u);

    // 清空使用人并标记为空闲
    void clearUser();
};
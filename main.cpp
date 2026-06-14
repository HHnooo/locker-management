#include "LockerManager.h"
#include "ThreadPool.h"
#include <iostream>
#include <string>
using namespace std;

// 程序入口：初始化管理器→创建线程池→循环接收用户输入→线程池异步执行
int main() {
    LockerManager mgr;     // 初始化12个储物柜
    ThreadPool pool(4);    // 4个工作线程
    int choice, row, col;
    string name, pwd;

    while (1) {
        cout << "1. 查看状态  2. 存物  3. 取物  4. 统计空闲  5. 退出" << endl;
        cout << "请选择：";
        cin >> choice;

        if (choice == 5) break;       // 退出

        if (choice == 1) {
            // 查看状态：提交到线程池异步执行
            pool.dispatch([&mgr] { mgr.showAllStatus(); });
        } else if (choice == 2) {
            // 存物：读取坐标和姓名，VIP额外读密码
            cout << "行号(1-3)："; cin >> row;
            cout << "列号(1-4)："; cin >> col;
            cout << "姓名："; cin >> name;
            if (row == 3) { cout << "6位密码："; cin >> pwd; }
            pool.dispatch([&mgr, row, col, name, pwd] {
                mgr.storeItem(row, col, name, pwd);
            });
        } else if (choice == 3) {
            // 取物：读取坐标和姓名，VIP额外读密码
            cout << "行号(1-3)："; cin >> row;
            cout << "列号(1-4)："; cin >> col;
            cout << "姓名："; cin >> name;
            if (row == 3) { cout << "密码："; cin >> pwd; }
            pool.dispatch([&mgr, row, col, name, pwd] {
                mgr.takeItem(row, col, name, pwd);
            });
        } else if (choice == 4) {
            // 统计空闲
            pool.dispatch([&mgr] { mgr.countFree(); });
        } else {
            cout << "输入无效！" << endl;
        }
    }

    cout << "再见！" << endl;
    return 0;
}
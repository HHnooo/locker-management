#include <string>
#include <iostream>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <queue>
#include <functional>
#include <future>
#include <vector>
#include <atomic>
#include <stdexcept>
using namespace std;

//Locker 类（基类）
class Locker {
private:
    int row;        // 行号（1~3）
    int col;        // 列号（1~4）
    bool isUsed;    // 是否被占用（true占用/false空闲）
    string user;    // 使用人姓名，空闲时为""

public:
    //构造函数：传入行号列号，默认空闲、使用人为空
    Locker(int r = 0, int c = 0);

    //打印柜子编号和占用or空闲状态
    void showStatus();

    //返回是否被占用
    bool getIsUsed();

    //返回使用人姓名
    string getUser();

    //设置使用人并标记为占用
    void setUser(string u);

    //清空使用人并标记为空闲
    void clearUser();
};

//构造函数：初始化行号列号，默认空闲，使用人为空
Locker::Locker(int r, int c) {
    row = r;
    col = c;
    isUsed = false;
    user = "";
}

//打印 模板为："编号：行-列，状态：占用or空闲"
void Locker::showStatus() {
    cout << "编号：" << row << "-" << col << "，状态：";
    if (isUsed) {
        cout << "占用";
    } else {
        cout << "空闲";
    }
    cout << endl;
}

//返回是否占用
bool Locker::getIsUsed() {
    return isUsed;
}

//返回使用人姓名
string Locker::getUser() {
    return user;
}

//保存姓名，标记占用
void Locker::setUser(string u) {
    user = u;
    isUsed = true;
}

//清除姓名，标记空闲
void Locker::clearUser() {
    user = "";
    isUsed = false;
}

//NormalLocker类（普通储物柜）
class NormalLocker : public Locker {
public:
    //构造函数 调用父类Locker初始化行号列号
    NormalLocker(int r = 0, int c = 0);

    //存物 柜子空闲且姓名非空才能存，成功返回true 失败返回false
    bool store(string userName);

    //取物 柜子占用且姓名匹配才能取，返回true成功/false失败
    bool take(string userName);
};

//构造函数：调用Locker初始化
NormalLocker::NormalLocker(int r, int c)
    : Locker(r, c) {
}

// 存物 先检查空闲 再检查姓名非空，通过后登记使用人
bool NormalLocker::store(string userName) {
    if (getIsUsed()) return false;       // 检查空闲
    if (userName.empty()) return false;  // 检查姓名非空
    setUser(userName);                   // 登记使用人
    return true;
}

// 取物 先检查占用 再检查姓名匹配，通过后释放柜子
bool NormalLocker::take(string userName) {
    if (!getIsUsed()) return false;            // 检查占用
    if (userName != getUser()) return false;   // 检查姓名匹配
    clearUser();                               // 释放柜子
    return true;
}

//VIPLocker 类（VIP储物柜）
class VIPLocker : public Locker {
private:
    string password;   // 6位纯数字密码

public:
    //构造函数 调用父类初始化，密码初始为空
    VIPLocker(int r = 0, int c = 0);

    //设置密码 仅6位纯数字才有效
    void setPassword(string pwd);

    //验证密码 与存储密码一致则返回true
    bool verifyPassword(string pwd);

    //VIP存物 空闲时存入，同时设置使用人和密码
    bool storeVIP(string userName, string pwd);

    //VIP取物：姓名和密码都匹配才能取物
    bool takeVIP(string userName, string pwd);
};

//构造函数
VIPLocker::VIPLocker(int r, int c)
    //调用父类初始化，密码为空
    : Locker(r, c) {
    password = "";
}

//设置密码
void VIPLocker::setPassword(string pwd) {
    //长度6且每位都是数字才保存
    if (pwd.size() != 6) return;
    for (int i = 0; i < 6; i++) {
        if (pwd[i] < '0' || pwd[i] > '9') return;
    }
    password = pwd;
}

// 验证密码
bool VIPLocker::verifyPassword(string pwd) {
    //非空且匹配
    if (password.empty()) return false;
    if (password != pwd) return false;
    return true;
}

//VIP存物
bool VIPLocker::storeVIP(string userName, string pwd) {
    //空闲且姓名密码都合法才能存
    if (getIsUsed()) return false;
    if (userName.empty()) return false;
    if (pwd.size() != 6) return false;
    for (int i = 0; i < 6; i++) {
        if (pwd[i] < '0' || pwd[i] > '9') return false;
    }
    setUser(userName);
    password = pwd;
    return true;
}

// VIP取物
bool VIPLocker::takeVIP(string userName, string pwd) {
    //占用、姓名、密码三项都匹配才释放柜子
    if (!getIsUsed()) return false;
    if (userName != getUser()) return false;
    if (pwd != password) return false;
    clearUser();
    password = "";     // 清空密码
    return true;
}

//LockerManager类

// 储物柜管理
class LockerManager {
private:
    NormalLocker normalLockers[2][4];       //8普通
    VIPLocker vipLockers[1][4];             //4VIP
    //每个柜子有独立互斥锁
    mutable std::mutex normalMutexes_[2][4];
    mutable std::mutex vipMutexes_[1][4];

public:
    LockerManager();
    void showAllStatus();
    bool storeItem(int row, int col, string userName, string pwd = "");
    bool takeItem(int row, int col, string userName, string pwd = "");
    void countFree();
    void showMenu();
};

//初始化柜子
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

// 遍历普通柜和VIP柜，多线程加锁后打印每个柜子的状态
void LockerManager::showAllStatus() {
    cout << "\n【普通储物柜】" << endl;
    //遍历普通柜和VIP柜
    for (int r = 0; r < 2; r++) {
        for (int c = 0; c < 4; c++) {
            unique_lock lock(normalMutexes_[r][c]);
            normalLockers[r][c].showStatus();
        }
    }
    //打印
    cout << "\n【VIP储物柜】" << endl;
    for (int c = 0; c < 4; c++) {
        //多线程加锁
        unique_lock lock(vipMutexes_[0][c]);
        vipLockers[0][c].showStatus();
    }
}

// 存物入口 校验范围，校验姓名，多线程加锁，检查占用，执行存物
bool LockerManager::storeItem(int row, int col, string userName, string pwd) {
    //校验范围
    if (row < 1 || row > 3 || col < 1 || col > 4) {
        cout << "错误：行号或列号超出范围！" << endl;
        return false;
    }
    //校验姓名
    if (userName.empty()) {
        cout << "错误：姓名不能为空！" << endl;
        return false;
    }

    // 普通柜（行：1or2）
    if (row == 1 || row == 2) {
        //多线程加锁
        unique_lock lock(normalMutexes_[row - 1][col - 1]);
        //检查占用
        if (normalLockers[row - 1][col - 1].getIsUsed()) {
            cout << "错误：该储物柜已被占用，无法存物！" << endl;
            return false;
        }
        normalLockers[row - 1][col - 1].store(userName);
        cout << "存物成功！" << endl;
        return true;
    }

    // VIP柜（行：3）
    if (row == 3) {
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
        //多线程加锁
        unique_lock lock(vipMutexes_[0][col - 1]);
        //检查占用
        if (vipLockers[0][col - 1].getIsUsed()) {
            cout << "错误：该VIP储物柜已被占用，无法存物！" << endl;
            return false;
        }
        vipLockers[0][col - 1].storeVIP(userName, pwd);
        cout << "VIP存物成功！" << endl;
        return true;
    }

    return false;
}

// 取物：校验范围，校验姓名，多线程加锁，检查占用，验证身份，取物
bool LockerManager::takeItem(int row, int col, string userName, string pwd) {
    // 校验范围
    if (row < 1 || row > 3 || col < 1 || col > 4) {
        cout << "错误：行号或列号超出范围！" << endl;
        return false;
    }
    // 校验姓名
    if (userName.empty()) {
        cout << "错误：姓名不能为空！" << endl;
        return false;
    }

    // 普通柜取物
    if (row == 1 || row == 2) {
        //多线程加锁
        unique_lock lock(normalMutexes_[row - 1][col - 1]);
        //检查占用
        if (!normalLockers[row - 1][col - 1].getIsUsed()) {
            cout << "错误：该储物柜为空闲状态，无法取物！" << endl;
            return false;
        }
        //验证身份
        if (normalLockers[row - 1][col - 1].getUser() != userName) {
            cout << "错误：姓名不匹配，取物失败！" << endl;
            return false;
        }
        //取物
        normalLockers[row - 1][col - 1].take(userName);
        cout << "取物成功！" << endl;
        return true;
    }

    //VIP柜取物（行3）
    if (row == 3) {
        // 多线程加锁
        unique_lock lock(vipMutexes_[0][col - 1]);
        // 检查占用
        if (!vipLockers[0][col - 1].getIsUsed()) {
            cout << "错误：该VIP储物柜为空闲状态，无法取物！" << endl;
            return false;
        }
        // 验证身份
        if (vipLockers[0][col - 1].getUser() != userName) {
            cout << "错误：姓名不匹配，取物失败！" << endl;
            return false;
        }
        // 验证身份
        if (!vipLockers[0][col - 1].verifyPassword(pwd)) {
            cout << "错误：密码错误，取物失败！" << endl;
            return false;
        }
        // 取物
        vipLockers[0][col - 1].takeVIP(userName, pwd);
        cout << "VIP取物成功！" << endl;
        return true;
    }

    return false;
}

//统计空闲：遍历所有柜子（多线程加锁），分别计数普通柜和VIP柜的空闲数
void LockerManager::countFree() {
    int normalFree = 0;
    int vipFree = 0;
    //遍历+统计
    for (int r = 0; r < 2; r++) {
        for (int c = 0; c < 4; c++) {
            //多线程加锁
            unique_lock lock(normalMutexes_[r][c]);
            if (!normalLockers[r][c].getIsUsed()) normalFree++;
        }
    }
    for (int c = 0; c < 4; c++) {
        //多线程加锁
        unique_lock lock(vipMutexes_[0][c]);
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

//ThreadPool类
class ThreadPool {
public:
    // 构造函数：创建num_threads个工作线程，每个线程进入worker_loop等待任务
    explicit ThreadPool(size_t num_threads);

    // 禁止线程池对象拷贝和赋值
    ThreadPool(const ThreadPool &) = delete;
    ThreadPool &operator=(const ThreadPool &) = delete;

    // 提交任务：将无返回值任务放入队列，唤醒一个空闲工作线程执行
    // 若线程池已关闭（stop_为true），抛出runtime_error异常
    void enqueue(std::function<void()> task);

    //分发任务：打包带返回值的任务，入队后返回future
    //可通过future.get()等待并获取任务执行结果
    template<typename F, typename... Args>
    auto dispatch(F&& f, Args&&... args)
        -> std::future<std::invoke_result_t<F, Args...>> {
        //推导返回值类型
        using R = std::invoke_result_t<F, Args...>;          
        //packaged_task包装任务，绑定函数和参数
        auto task = std::make_shared<std::packaged_task<R()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
        auto future = task->get_future();                     //获取关联的future
        enqueue([task] { (*task)(); });                       //包装后入队
        return future;
    }

    //关闭线程池
    void shutdown();

    //析构自动调用shutdown，确保线程被正确回收
    ~ThreadPool();

    //返回工作线程数量
    size_t size() const noexcept { return workers_.size(); }

private:
    //工作线程主循环：反复从任务队列取任务并执行，直到stop且队列空
    void worker_loop();

    std::vector<std::thread> workers_;           // 工作线程数组
    std::queue<std::function<void()>> tasks_;    // 任务队列，存储待执行任务
    std::mutex mutex_;                           // 互斥锁，保护任务队列的并发访问
    std::condition_variable cv_;                 // 条件变量，用于线程的等待与唤醒
    std::atomic<bool> stop_{false};              // 停止标记
};

//创建num_threads个工作线程，启动worker_loop
ThreadPool::ThreadPool(size_t num_threads) {
    for (size_t i = 0; i < num_threads; ++i) {
        workers_.emplace_back(&ThreadPool::worker_loop, this);
    }
}

//将任务加入队列，唤醒一个等待线程；若已关闭则抛出异常
void ThreadPool::enqueue(std::function<void()> task) {
    {
        std::lock_guard lock(mutex_);
        if (stop_) throw std::runtime_error("线程池已关闭，无法接受新任务");
        tasks_.push(std::move(task));
    }
    cv_.notify_one();
}

//worker主循环：等待任务，取出，执行，直到stop且队列空
void ThreadPool::worker_loop() {
    while (true) {
        std::function<void()> task;
        {
            std::unique_lock lock(mutex_);
            cv_.wait(lock, [this] { return stop_ || !tasks_.empty(); });
            if (stop_ && tasks_.empty()) return;
            task = std::move(tasks_.front());
            tasks_.pop();
        }
        task();
    }
}

//设置stop标记，唤醒所有worker，等待全部结束
void ThreadPool::shutdown() {
    {
        std::lock_guard lock(mutex_);
        stop_ = true;
    }
    cv_.notify_all();
    for (auto &worker : workers_) {
        if (worker.joinable()) worker.join();
    }
}

//关闭线程池
ThreadPool::~ThreadPool() {
    shutdown();
}

//主函数
int main() {
    LockerManager mgr;     // 初始化12个储物柜
    ThreadPool pool(4);    // 4个工作线程
    int choice, row, col;  // 用户选择、储物柜坐标
    string name, pwd;

    while (1) {
        cout << "1. 查看状态  2. 存物  3. 取物  4. 统计空闲  5. 退出" << endl;
        cout << "请选择：";
        cin >> choice;

        if (choice == 5) break;

        if (choice == 1) {
            pool.dispatch([&mgr] { mgr.showAllStatus(); });
        } else if (choice == 2) {
            cout << "行号(1-3)："; cin >> row;
            cout << "列号(1-4)："; cin >> col;
            cout << "姓名："; cin >> name;
            if (row == 3) { cout << "6位密码："; cin >> pwd; }
            pool.dispatch([&mgr, row, col, name, pwd] {
                mgr.storeItem(row, col, name, pwd);
            });
        } else if (choice == 3) {
            cout << "行号(1-3)："; cin >> row;
            cout << "列号(1-4)："; cin >> col;
            cout << "姓名："; cin >> name;
            if (row == 3) { cout << "密码："; cin >> pwd; }
            pool.dispatch([&mgr, row, col, name, pwd] {
                mgr.takeItem(row, col, name, pwd);
            });
        } else if (choice == 4) {
            pool.dispatch([&mgr] { mgr.countFree(); });
        } else {
            cout << "输入无效！" << endl;
        }
    }

    cout << "再见！" << endl;
    return 0;
}

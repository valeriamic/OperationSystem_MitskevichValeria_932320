#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <sstream>

using namespace std;

struct Event { // создание события, которое будет передаваться потребителю
    int id;
    string message;
};

mutex mtx;
condition_variable cv;
bool ready = false;
Event* sharedEvent = nullptr;

void provider(int count) {
    for (int i = 0; i < count; ++i) {
        this_thread::sleep_for(chrono::seconds(1)); // установка задержки на 1 секунду
        unique_lock<mutex> lock(mtx);
        if (ready) continue;
        stringstream ss;
        ss << i + 1;
        sharedEvent = new Event{ i + 1, "Event " + ss.str() };
        ready = true; // флаг, что событие готово
        cout << "provided: " << sharedEvent->message << " отправлено" << endl;
        cv.notify_one(); // сообщение о готовности события
    }
}

void consumer(int count) {
    for (int i = 0; i < count; ++i) {
        unique_lock<mutex> lock(mtx);
        cv.wait(lock, [] { return ready; });  // ожидание сигнала от producer, пока событие не станет готовым
        Event* e = sharedEvent;
        sharedEvent = nullptr;
        ready = false;
        cout << "consumed: " << e->message << " получено" << endl; // вывод события на экран

        delete e;
    }
}

int main() {
    const int N = 5; // условие остановки, чтобы не зацикливалось до бесконечности

    thread t1(provider, N);
    thread t2(consumer, N);

    t1.join();
    t2.join();

    return 0;
}

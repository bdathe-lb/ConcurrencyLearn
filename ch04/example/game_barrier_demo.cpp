#include <barrier>
#include <chrono>
#include <iostream>
#include <string>
#include <thread>

auto OnStageComplete = []() {
  static int stage = 1;

  std::cout << "\n============================================\n";
  std::cout << " [系统广播] 所有玩家已准备就绪！Stage " << stage << " 圆满结束！\n";
  std::cout << "============================================\n\n";

  ++ stage;
};

std::barrier<decltype(OnStageComplete)> game_barrier(3, OnStageComplete);

void PlayerThread(std::string player_name, int  load_time) {
  // -- Phase 1: Load map data --
  std::cout << "玩家 [" << player_name << "] 开始加载地图...\n";
  std::this_thread::sleep_for(std::chrono::milliseconds(load_time));
  std::cout << "玩家 [" << player_name << "] 地图加载完毕，在路障 A 处等待队友...\n";

  game_barrier.arrive_and_wait();

  // -- Phase 2: Generate game characters --
  std::cout << "玩家 [" << player_name << "] 开始初始化角色和装备...\n";
  std::this_thread::sleep_for(std::chrono::milliseconds(load_time / 2)); 
  std::cout << "玩家 [" << player_name << "] 角色初始化完毕，在路障 B 处等待队友...\n";

  game_barrier.arrive_and_wait();

  // ---- Successfully entered the game ----
  std::cout << "玩家 [" << player_name << "] 成功进入游戏世界！\n";
}

int main (int argc, char *argv[]) {
  std::cout << "游戏大厅开启，正在等待玩家准备...\n\n";

  std::thread p1(PlayerThread, "张三", 300);
  std::thread p2(PlayerThread, "李四", 800);
  std::thread p3(PlayerThread, "王五", 150);

  p1.join();
  p2.join();
  p3.join();

  std::cout << "\n游戏顺利运行中，祝您游戏愉快！\n";
  
  return 0;
}

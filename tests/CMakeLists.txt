cmake_minimum_required(VERSION 3.14)
project(mcr_motor_tests CXX)

set(CMAKE_CXX_STANDARD 17)

# ============================================================
# Google Test (FetchContent 自動ダウンロード)
# ============================================================
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.14.0
)
# Windows: ランタイムライブラリの強制設定 (MSVC用)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# ============================================================
# インクルードパス
#   1. tests/mock/  — モックレジスタ (最優先)
#   2. src/         — IModule.h, drivers/*.h
# ============================================================
include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/mock
  ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

# ============================================================
# テスト対象のドライバソース
# ============================================================
set(DRIVER_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/../src/drivers/Motor.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../src/drivers/Servo.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../src/drivers/Encoder.cpp
)

# ============================================================
# モックレジスタ定義
# ============================================================
set(MOCK_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/mock/mock_registers.cpp
)

# ============================================================
# テスト実行ファイル
#   test_globals.cpp は main.cpp をリンクしない代わりに
#   g_sys / g_motor / g_servo / g_encoder / g_timer_1ms を定義する
# ============================================================
add_executable(run_tests
  ${DRIVER_SOURCES}
  ${MOCK_SOURCES}
  test_globals.cpp
  test_motor.cpp
  test_servo.cpp
  test_encoder.cpp
)

target_link_libraries(run_tests
  GTest::gtest_main
)

# ============================================================
# CTest との統合
# ============================================================
include(GoogleTest)
gtest_discover_tests(run_tests)

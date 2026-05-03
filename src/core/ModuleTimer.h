/*
 * ModuleTimer.h
 *
 *  ノンブロッキング時間計測クラス。
 *  Embedded-Module-Architecture (EMA) の ModuleTimer に相当する実装。
 *
 *  ベアメタル GR-PEACH では Arduino の millis() が使えないため、
 *  main.cpp 側の OSTM0 1ms 割り込みコールバックがインクリメントする
 *  グローバルカウンタ g_timer_1ms をタイムソースとして利用する。
 */

#ifndef CORE_MODULE_TIMER_H_
#define CORE_MODULE_TIMER_H_

// main.cpp 側で定義される 1ms 累積カウンタ。
// OSTM0 割り込みコールバック内で ++ される。
extern volatile unsigned long g_timer_1ms;

class ModuleTimer
{
public:
    ModuleTimer() : start_(0), running_(false) {}

    // 現在時刻を起点としてタイマーを開始する。
    void setTime()
    {
        start_   = g_timer_1ms;
        running_ = true;
    }

    // setTime() からの経過時間 [ms]。未起動の場合は 0。
    unsigned long getNowTime() const
    {
        return running_ ? (g_timer_1ms - start_) : 0UL;
    }

    // 指定時間 [ms] 以上経過したか。未起動の場合は false。
    bool isExpired(unsigned long ms) const
    {
        return running_ && ((g_timer_1ms - start_) >= ms);
    }

    // タイマーを停止状態に戻す。
    void reset()
    {
        running_ = false;
        start_   = 0;
    }

    // タイマーが起動中か。
    bool isRunning() const { return running_; }

private:
    unsigned long start_;
    bool          running_;
};

#endif /* CORE_MODULE_TIMER_H_ */

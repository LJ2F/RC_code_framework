/**
 * Derived from ROBOCON26_R2 UsbPort, author Keten (2863861004@qq.com), 2026.
 * Framework adaptation: task-owned transmitter, bounded queues and connection epochs.
 */
#pragma once
#include "stm32h7xx_hal.h"
#include "lockfree_queue.hpp"
#include <atomic>
#include <cstddef>
#include <cstdint>
class UsbPort {
public:
  static constexpr size_t kRxPayload = 256, kTxPayload = 512, kQueueDepth = 8;
  struct Packet { uint16_t len{}; uint8_t data[kRxPayload]{}; uint32_t generation{}; };
  struct Stats {
    uint32_t rx_packets, rx_bytes, rx_drop_packets, tx_packets, tx_bytes;
    uint32_t tx_drop_packets, tx_busy_hits, tx_errors, connection_changes;
  };
  using RxCallback = void(*)(const uint8_t*, size_t, void*);
  static UsbPort& Instance();
  HAL_StatusTypeDef writeAsync(const uint8_t*, size_t);
  bool Read(Packet&); // One consumer; callback executes in this receiving task.
  bool connected() const;
  bool txBusy() const;
  Stats getStats() const;
  void SetRxCallback(RxCallback cb, void* user); // Configure in consumer task.
  bool StartService(); // Once, after osKernelInitialize.
  void ServiceOnce(); // USB service task only; also used in backend simulation.
  void OnRxFromIsr(const uint8_t*, size_t);
  void OnTxCpltFromIsr();
  void OnStateFromIsr(bool ready, bool aborted);
private:
  UsbPort() = default;
  struct TxPacket { uint16_t len{}; uint8_t data[kTxPayload]{}; uint32_t generation{}; };
  static_assert(std::atomic<uint32_t>::is_always_lock_free);
  void wake();
  Algorithm::MpscQueue<Packet,kQueueDepth> rx_;
  Algorithm::MpscQueue<TxPacket,kQueueDepth> tx_;
  TxPacket staging_{};
  bool staging_valid_{false};
  std::atomic<bool> staging_pending_{false};
  std::atomic<bool> inflight_{false};
  std::atomic<uint32_t> epoch_{0}, completed_{0}, aborted_{0};
  uint32_t active_completed_{0}, active_aborted_{0};
  std::atomic<void*> service_{nullptr};
  RxCallback callback_{nullptr};
  void* user_{nullptr};
  enum Counter { RxPackets, RxBytes, RxDrops, TxPackets, TxBytes, TxDrops,
                 TxBusy, TxErrors, Changes, Count };
  std::atomic<uint32_t> counters_[Count]{};
};

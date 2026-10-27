/* USB C callback bridge. Derived from Keten's R2 UsbPort (2026). */
#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
void UsbPort_Prepare(void);
void UsbPort_OnRxFromIsr(const uint8_t* data, uint32_t len);
void UsbPort_OnTxCpltFromIsr(void);
/* aborted=1 only AFTER endpoints have stopped referencing the TX buffer. */
void UsbPort_OnStateFromIsr(uint8_t ready, uint8_t aborted);
#ifdef __cplusplus
}
#endif

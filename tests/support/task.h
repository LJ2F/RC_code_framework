#pragma once
#ifdef __cplusplus
extern "C" {
#endif
void mock_enter(void);
void mock_exit(void);
#ifdef __cplusplus
}
#endif
#define taskENTER_CRITICAL() mock_enter()
#define taskEXIT_CRITICAL() mock_exit()

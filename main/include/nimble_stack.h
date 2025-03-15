#ifndef NIMBLE_STACK_H
#define NIMBLE_STACK_H

void on_stack_reset(int reason);
void on_stack_sync(void);
void nimble_host_config_init(void);
void nimble_host_task(void *param);

#endif
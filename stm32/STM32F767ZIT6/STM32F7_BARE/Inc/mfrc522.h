#ifndef MFRC522_H
#define MFRC522_H

#include <stdint.h>
#include "stm32f7xx.h"

void mfrc522_version(uint8_t *version);
void mfrc522_check(uint8_t *status);

void mfrc522_init(void);
void mfrc522_antenna_check(uint8_t *tx);
int mfrc522_check_atqa(uint8_t *atqa, uint8_t *atqa_len);
int mfrc522_read_id(uint8_t *uid, uint8_t *uid_len);

#endif // MFRC522_H